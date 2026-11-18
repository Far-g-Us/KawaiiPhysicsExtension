@icon("res://addons/kawaii_physics_extension/icon.svg")
class_name KawaiiPhysics
extends SkeletonModifier3D

var _core: GDScript = null
var _scripts_failed := false
var _elapsed := 0.0
var last_step_usec := 0
var _settings_script: GDScript = null
var _sphere_limit_script: GDScript = null
var _capsule_limit_script: GDScript = null
var _plane_limit_script: GDScript = null


func _ensure_scripts() -> void:
	if _core != null or _scripts_failed:
		return
	var bases: Array[String] = []
	var sc := get_script() as Script
	if sc != null and not sc.resource_path.is_empty():
		bases.append(sc.resource_path.get_base_dir())
	bases.append("res://addons/kawaii_physics_extension/scripts")
	bases.append("res://addons/kawaii_physics/scripts")
	for base in bases:
		var core: GDScript = load(base + "/kawaii_sim_core.gd")
		if core == null:
			continue
		_core = core
		_settings_script = load(base + "/kawaii_settings.gd")
		_sphere_limit_script = load(base + "/kawaii_sphere_limit.gd")
		_capsule_limit_script = load(base + "/kawaii_capsule_limit.gd")
		_plane_limit_script = load(base + "/kawaii_plane_limit.gd")
		break
	if _core == null:
		_scripts_failed = true
		push_error("KawaiiPhysics: scripts not found (addons folder renamed?)")
		return
	if settings == null and _settings_script != null:
		settings = _settings_script.new()
const EPS := 1e-6

enum SimSpace { COMPONENT, WORLD }

@export var simulation_space: SimSpace = SimSpace.COMPONENT
@export var root_bone: StringName = &"":
	set(v):
		root_bone = v
		_chain_dirty = true
@export var exclude_bones: Array[StringName] = []:
	set(v):
		exclude_bones = v
		_chain_dirty = true
@export var additional_root_bones: Array[StringName] = []:
	set(v):
		additional_root_bones = v
		_chain_dirty = true
@export var settings: KawaiiSettings
@export var bone_constraints: Array[KawaiiBoneConstraint] = []
@export var sync_bones: Array = []
@export_range(0, 10) var bone_subdivision_count: int = 0:
	set(v):
		bone_subdivision_count = v
		_chain_dirty = true
@export var dummy_bone_length: float = 0.0:
	set(v):
		dummy_bone_length = v
		_chain_dirty = true
@export_range(1, 240) var target_framerate: int = 60
@export var gravity: Vector3 = Vector3(0, -9.8, 0)
@export var wind: Resource = null
@export_range(0.0, 10.0) var wind_scale: float = 1.0
@export var wind_zones: Array = []
@export var constant_force: Vector3 = Vector3.ZERO
@export var teleport_distance_threshold: float = 3.0
@export var teleport_rotation_threshold_deg: float = 10.0
@export var warmup_frames: int = 0
@export var sphere_limits: Array = []
@export var capsule_limits: Array = []
@export var plane_limits: Array = []
@export var world_collision_enabled: bool = false
@export_flags_3d_physics var world_collision_mask: int = 1
@export_range(0.0, 0.2) var world_collision_margin: float = 0.005
@export_range(0.0, 4.0) var world_collision_radius_scale: float = 1.0
@export var self_collision_enabled: bool = false
@export_range(0.0, 10.0) var self_collision_min_distance: float = 0.01
@export_range(0.0, 10.0) var self_collision_radius_multiplier: float = 1.0
@export_range(0.0, 1.0) var self_collision_push_split_ratio: float = 0.5
@export var debug_draw: bool = false

var _bones: Array = []
var _name_to_chain: Dictionary = {}
var _anim_cache: Dictionary = {}
var _anim_bones: Dictionary = {}
var _chain_dirty: bool = false
var _skeleton: Skeleton3D = null
var _written_bones: Array[int] = []
var _written_sk: Skeleton3D = null
var _prev_origin: Vector3 = Vector3.ZERO
var _prev_rot: Quaternion = Quaternion.IDENTITY
var _has_prev: bool = false
var _prev_delta: float = 0.0
var _pending_warmup: int = 0
var _in_step: bool = false
var _resolved_spheres: Array = []
var _resolved_capsules: Array = []
var _resolved_planes: Array = []
var _debug_mesh_instance: MeshInstance3D = null
var _debug_mesh: ImmediateMesh = null
var _debug_material: StandardMaterial3D = null


func _init() -> void:
	pass


func _ready() -> void:
	add_to_group("kawaii_physics")
	_ensure_scripts()


func _process_modification_with_delta(delta: float) -> void:
	if _in_step:
		return
	_ensure_scripts()
	_in_step = true
	last_step_usec = 0
	var t0: int = Time.get_ticks_usec()
	if _core == null:
		_in_step = false
		return
	var sk := _resolve_skeleton()
	var no_roots: bool = root_bone == &"" and additional_root_bones.is_empty()
	if sk == null or no_roots or delta <= 0.0:
		if sk != null and no_roots:
			_unwrite_override()
		_in_step = false
		return
	sk.force_update_all_bone_transforms()
	if sk != _skeleton or _chain_dirty:
		_skeleton = sk
		_chain_dirty = false
		_unwrite_override()
		_rebuild_chain(sk)
	if _bones.is_empty():
		_unwrite_override()
		_in_step = false
		return
	_sync_settings()
	if simulation_space == SimSpace.WORLD:
		for b in _bones:
			b.world_damp_loc = 0.0
			b.world_damp_rot = 0.0
	var now_origin := sk.global_position
	var now_rot := Quaternion(sk.global_basis)
	var teleported := false
	var move_loc := Vector3.ZERO
	var move_rot := Quaternion.IDENTITY
	if _has_prev:
		move_loc = sk.global_transform.affine_inverse() * _prev_origin
		move_rot = now_rot.inverse() * _prev_rot
		if teleport_distance_threshold > 0.0 and move_loc.length() > teleport_distance_threshold:
			teleported = true
		if not teleported and teleport_rotation_threshold_deg > 0.0:
			if absf(move_rot.get_angle()) > deg_to_rad(teleport_rotation_threshold_deg):
				teleported = true
		if teleported:
			move_loc = Vector3.ZERO
			move_rot = Quaternion.IDENTITY
	if teleported:
		_reset_to_pose(sk)
		_has_prev = false
	var skel_basis_inv := sk.global_transform.affine_inverse().basis
	var base_force: Vector3 = gravity + constant_force
	var has_air: bool = false
	if wind != null and is_instance_valid(wind) and wind.has_method("sample") and bool(wind.get("enabled")):
		has_air = true
	if not wind_zones.is_empty():
		has_air = true
	var bone_forces: Array = []
	if has_air and not is_zero_approx(wind_scale):
		_elapsed += delta
		for b in _bones:
			var wpos: Vector3 = sk.global_transform * (b.loc as Vector3)
			var wf := Vector3.ZERO
			if wind != null and is_instance_valid(wind) and wind.has_method("sample") and bool(wind.get("enabled")):
				wf += (wind.call("sample", _elapsed, wpos) as Vector3) * wind_scale
			for z in wind_zones:
				if z != null and is_instance_valid(z) and z.has_method("sample") and bool(z.get("enabled")):
					wf += (z.call("sample", _elapsed, wpos) as Vector3) * wind_scale
			bone_forces.append(skel_basis_inv * (base_force + wf))
	var gravity_sim := skel_basis_inv * base_force
	var dt := delta
	var dt_old := _prev_delta
	if dt_old <= EPS:
		dt_old = 1.0 / float(target_framerate)
	_snapshot_poses(sk)
	if _pending_warmup > 0:
		var wdt := 1.0 / float(target_framerate)
		for i in range(_pending_warmup):
			_core.simulate_step(_bones, wdt, wdt, gravity_sim, Vector3.ZERO, Quaternion.IDENTITY, float(target_framerate), bone_forces)
			_apply_collision_limits(sk)
			_apply_self_collision()
			_apply_angle_limits()
			_core.restore_lengths(_bones)
			_apply_bone_constraints()
			_core.restore_lengths(_bones)
			_apply_world_collision(sk)
		_pending_warmup = 0
	_core.simulate_step(_bones, dt_old, dt, gravity_sim, move_loc, move_rot, float(target_framerate), bone_forces)
	_apply_collision_limits(sk)
	_apply_self_collision()
	_apply_angle_limits()
	_core.restore_lengths(_bones)
	_apply_bone_constraints()
	_core.restore_lengths(_bones)
	_apply_world_collision(sk)
	_write_back(sk, get_influence())
	_apply_sync_bones(sk)
	_update_debug_draw(sk)
	_prev_origin = now_origin
	_prev_rot = now_rot
	_prev_delta = dt
	_has_prev = true
	_in_step = false
	last_step_usec = Time.get_ticks_usec() - t0


func _skeleton_changed(old_skeleton: Skeleton3D, new_skeleton: Skeleton3D) -> void:
	_unwrite_override()
	_skeleton = null
	_bones = []
	_name_to_chain = {}
	_has_prev = false
	_prev_delta = 0.0
	_chain_dirty = false


func _exit_tree() -> void:
	_unwrite_override()
	if _debug_mesh_instance != null and is_instance_valid(_debug_mesh_instance):
		_debug_mesh_instance.free()
	_debug_mesh_instance = null
	_debug_mesh = null
	_debug_material = null
	_skeleton = null
	_bones = []


func reset_dynamics() -> void:
	_has_prev = false
	_prev_delta = 0.0
	_pending_warmup = warmup_frames
	if _skeleton != null and is_instance_valid(_skeleton):
		_reset_to_pose(_skeleton)


func get_chain_center() -> Vector3:
	var sk := _resolve_skeleton()
	if sk == null or _bones.is_empty():
		return global_position
	var sum := Vector3.ZERO
	var count := 0
	for b in _bones:
		if b.is_dummy:
			continue
		sum += sk.global_transform * (b.loc as Vector3)
		count += 1
	if count == 0:
		return global_position
	return sum / float(count)


func _resolve_skeleton() -> Skeleton3D:
	var sk := get_skeleton()
	if sk != null:
		return sk
	return get_parent() as Skeleton3D


func _rebuild_chain(sk: Skeleton3D) -> void:
	_bones = []
	_name_to_chain = {}
	var roots: Array[StringName] = []
	if root_bone != &"":
		roots.append(root_bone)
	for r in additional_root_bones:
		if r != &"" and not roots.has(r):
			roots.append(r)
	if roots.is_empty():
		return
	var excluded := {}
	for e in exclude_bones:
		var ei := sk.find_bone(String(e))
		if ei >= 0:
			_collect_excluded(sk, ei, excluded)
	for r in roots:
		var root_i := sk.find_bone(String(r))
		if root_i < 0:
			continue
		_dfs_chain(sk, root_i, -1, excluded)
	if _bones.is_empty():
		return
	_append_dummy_tips()
	_subdivide_bones()
	if _bones.is_empty():
		return
	_anim_bones = {}
	for nb in _bones:
		if not nb.is_dummy and int(nb.index) >= 0:
			_anim_bones[int(nb.index)] = true
	_snapshot_poses(sk, true)
	_pending_warmup = warmup_frames


func _collect_excluded(sk: Skeleton3D, bone_i: int, excluded: Dictionary) -> void:
	excluded[bone_i] = true
	for c in sk.get_bone_children(bone_i):
		_collect_excluded(sk, c, excluded)


func _dfs_chain(sk: Skeleton3D, bone_i: int, parent_ci: int, excluded: Dictionary) -> void:
	if excluded.has(bone_i):
		return
	if _name_to_chain.has(String(sk.get_bone_name(bone_i))):
		return
	var ci := _bones.size()
	_bones.append({
		name = String(sk.get_bone_name(bone_i)),
		index = bone_i,
		parent_idx = parent_ci,
		pose_loc = Vector3.ZERO,
		pose_rot = Quaternion.IDENTITY,
		loc = Vector3.ZERO,
		prev_loc = Vector3.ZERO,
		prev_prev_loc = Vector3.ZERO,
		damping = 0.0,
		stiffness = 0.0,
		world_damp_loc = 0.8,
		world_damp_rot = 0.8,
		limit_angle_deg = 0.0,
		radius = 0.03,
		is_dummy = false,
		seeded = false,
	})
	_name_to_chain[String(sk.get_bone_name(bone_i))] = ci
	for c in sk.get_bone_children(bone_i):
		_dfs_chain(sk, c, ci, excluded)


func _append_dummy_tips() -> void:
	if dummy_bone_length <= 0.0:
		return
	var child_count := {}
	for b in _bones:
		if b.parent_idx >= 0:
			child_count[b.parent_idx] = int(child_count.get(b.parent_idx, 0)) + 1
	for ci in range(_bones.size()):
		var b: Dictionary = _bones[ci]
		if int(child_count.get(ci, 0)) > 0:
			continue
		_bones.append({
			name = b.name + "_dummy",
			index = -1,
			sub_t = 0.0,
			parent_idx = ci,
			pose_loc = Vector3.ZERO,
			pose_rot = Quaternion.IDENTITY,
			loc = Vector3.ZERO,
			prev_loc = Vector3.ZERO,
			prev_prev_loc = Vector3.ZERO,
			damping = 0.0,
			stiffness = 0.0,
			world_damp_loc = 0.8,
			world_damp_rot = 0.8,
			limit_angle_deg = 0.0,
			radius = 0.03,
			is_dummy = true,
			seeded = false,
		})


func _subdivide_bones() -> void:
	var count: int = bone_subdivision_count
	if count <= 0 or _bones.is_empty():
		return
	var children: Dictionary = {}
	for ci in range(_bones.size()):
		var b: Dictionary = _bones[ci]
		if b.is_dummy:
			continue
		var p: int = int(b.parent_idx)
		if p < 0:
			continue
		if children.has(p):
			children[p] = -1
		else:
			children[p] = ci
	var new_bones: Array = []
	var remap: Array[int] = []
	remap.resize(_bones.size())
	var pending: Dictionary = {}
	for ci in range(_bones.size()):
		var b: Dictionary = _bones[ci]
		var nb: Dictionary = b.duplicate(true)
		if b.parent_idx < 0:
			nb.parent_idx = -1
		elif pending.has(ci):
			nb.parent_idx = int(pending[ci])
		else:
			nb.parent_idx = remap[int(b.parent_idx)]
		remap[ci] = new_bones.size()
		new_bones.append(nb)
		var child_i: int = int(children.get(ci, -1))
		if child_i < 0:
			continue
		var child: Dictionary = _bones[child_i]
		var prev_new: int = remap[ci]
		for k in range(1, count + 1):
			var t := float(k) / float(count + 1)
			var d: Dictionary = b.duplicate(true)
			d.name = String(b.name) + "_sub%d" % k
			d.index = -1
			d.is_dummy = true
			d.sub_t = t
			d.edge_a = int(b.index)
			d.edge_b = int(child.index)
			d.parent_idx = prev_new
			d.pose_loc = (b.pose_loc as Vector3).lerp(child.pose_loc as Vector3, t)
			d.pose_rot = b.pose_rot
			d.loc = d.pose_loc
			d.prev_loc = d.pose_loc
			d.prev_prev_loc = d.pose_loc
			d.seeded = true
			prev_new = new_bones.size()
			new_bones.append(d)
		pending[child_i] = prev_new
	_bones = new_bones
	_name_to_chain = {}
	for ci in range(_bones.size()):
		var nb: Dictionary = _bones[ci]
		if not nb.is_dummy and int(nb.index) >= 0:
			_name_to_chain[String(nb.name)] = ci


func _snapshot_poses(sk: Skeleton3D, seed_all: bool = false) -> void:
	_anim_cache.clear()
	for b in _bones:
		if b.is_dummy:
			continue
		if b.index < 0:
			continue
		var g := _anim_global(sk, int(b.index))
		b.pose_loc = g.origin
		b.pose_rot = g.basis.get_rotation_quaternion()
		if seed_all or not b.seeded:
			b.loc = b.pose_loc
			b.prev_loc = b.pose_loc
			b.prev_prev_loc = b.pose_loc
			b.seeded = true
	for b in _bones:
		if not b.is_dummy:
			continue
		var sub_t: float = float(b.get("sub_t", 0.0))
		if sub_t > 0.0:
			var ea: int = int(b.get("edge_a", -1))
			var eb: int = int(b.get("edge_b", -1))
			var ga := _anim_global(sk, ea) if ea >= 0 else Transform3D.IDENTITY
			var gb := _anim_global(sk, eb) if eb >= 0 else ga
			b.pose_loc = ga.origin.lerp(gb.origin, sub_t)
			b.pose_rot = ga.basis.get_rotation_quaternion()
		else:
			var leaf: Dictionary = _bones[b.parent_idx]
			var dir := Vector3.ZERO
			if leaf.parent_idx >= 0:
				dir = (leaf.pose_loc as Vector3) - (_bones[leaf.parent_idx].pose_loc as Vector3)
			if dir.length() <= EPS:
				dir = (leaf.pose_rot as Quaternion) * Vector3(0, 0, 1)
			b.pose_loc = (leaf.pose_loc as Vector3) + dir.normalized() * dummy_bone_length
			b.pose_rot = leaf.pose_rot
		if seed_all or not b.seeded:
			b.loc = b.pose_loc
			b.prev_loc = b.pose_loc
			b.prev_prev_loc = b.pose_loc
			b.seeded = true


func _sync_settings() -> void:
	if settings == null or not (settings is KawaiiSettings):
		return
	var count: int = _bones.size()
	for i in range(count):
		var b: Dictionary = _bones[i]
		var t: float = 0.0 if count <= 1 else float(i) / float(count - 1)
		var stiff: float = float(settings.stiffness)
		var damp: float = float(settings.damping)
		if settings.stiffness_curve != null:
			stiff *= settings.stiffness_curve.sample(t)
		if settings.damping_curve != null:
			damp *= settings.damping_curve.sample(t)
		b.damping = damp
		b.stiffness = stiff
		b.world_damp_loc = settings.world_damping_location
		b.world_damp_rot = settings.world_damping_rotation
		b.radius = settings.collision_radius
		b.limit_angle_deg = settings.limit_angle_deg


func _apply_angle_limits() -> void:
	for i in range(_bones.size()):
		if _bones[i].parent_idx >= 0:
			_core.adjust_by_angle_limit(_bones, i)


func _anim_global(sk: Skeleton3D, bone_i: int) -> Transform3D:
	if _anim_cache.has(bone_i):
		return _anim_cache[bone_i]
	var local := sk.get_bone_pose(bone_i)
	var parent_i := sk.get_bone_parent(bone_i)
	var g: Transform3D
	if parent_i < 0:
		g = local
	elif _anim_bones.has(parent_i):
		g = _anim_global(sk, parent_i) * local
	else:
		g = sk.get_bone_global_pose(parent_i) * local
	_anim_cache[bone_i] = g
	return g


func _clean_global_pose(sk: Skeleton3D, bone_i: int) -> Transform3D:
	var stack: Array[int] = []
	var cur := bone_i
	while cur >= 0:
		stack.push_back(cur)
		cur = sk.get_bone_parent(cur)
	var g := Transform3D.IDENTITY
	for i in range(stack.size() - 1, -1, -1):
		g = g * sk.get_bone_pose(stack[i])
	return g


func _resolve_limit(sk: Skeleton3D, lim: Resource, script: GDScript) -> Dictionary:
	if lim == null or not is_instance_valid(lim):
		return {}
	if lim.get_script() != script:
		return {}
	if not bool(lim.enabled):
		return {}
	var bone_i := sk.find_bone(String(lim.driving_bone))
	if bone_i < 0:
		return {}
	var dg := _clean_global_pose(sk, bone_i)
	var rot := dg.basis.get_rotation_quaternion()
	return {loc = dg.origin + rot * (lim.offset_location as Vector3), rot = rot}


func _limit_rot(base_rot: Quaternion, offset_deg: Vector3) -> Quaternion:
	return base_rot * Quaternion.from_euler(Vector3(
		deg_to_rad(offset_deg.x), deg_to_rad(offset_deg.y), deg_to_rad(offset_deg.z)))


func _apply_bone_constraints() -> void:
	if bone_constraints.is_empty():
		return
	for c in bone_constraints:
		if c == null or not is_instance_valid(c) or not bool(c.enabled):
			continue
		var ia: int = int(_name_to_chain.get(String(c.bone_a), -1))
		var ib: int = int(_name_to_chain.get(String(c.bone_b), -1))
		if ia < 0 or ib < 0 or ia == ib:
			continue
		var a: Dictionary = _bones[ia]
		var b: Dictionary = _bones[ib]
		var target: float = (a.pose_loc as Vector3).distance_to(b.pose_loc as Vector3)
		var la: Vector3 = a.loc
		var lb: Vector3 = b.loc
		var cur: float = la.distance_to(lb)
		if target <= EPS or cur <= EPS:
			continue
		var err: float = cur - target
		var dir: Vector3 = (lb - la) / cur
		var k: float = clampf(float(c.stiffness), 0.0, 1.0) * 0.5
		a.loc = la + dir * err * k
		b.loc = lb - dir * err * k
		a.prev_loc = (a.prev_loc as Vector3).lerp(a.loc, 0.2)
		b.prev_loc = (b.prev_loc as Vector3).lerp(b.loc, 0.2)


func _apply_world_collision(sk: Skeleton3D) -> void:
	if not world_collision_enabled or _bones.is_empty():
		return
	var world := sk.get_world_3d()
	if world == null:
		return
	var space := world.direct_space_state
	if space == null:
		return
	var exclude: Array[RID] = []
	var owner_node := sk.get_parent()
	while owner_node != null:
		if owner_node is CollisionObject3D:
			exclude.append((owner_node as CollisionObject3D).get_rid())
		owner_node = owner_node.get_parent()
	var skel_xform := sk.global_transform
	var skel_inv := skel_xform.affine_inverse()
	for b in _bones:
		if b.parent_idx < 0:
			continue
		var start_ci: int = int(b.parent_idx)
		while start_ci >= 0 and bool(_bones[start_ci].is_dummy):
			start_ci = int(_bones[start_ci].parent_idx)
		var from_w: Vector3 = skel_xform * (b.pose_loc as Vector3)
		if start_ci >= 0:
			from_w = skel_xform * (_bones[start_ci].loc as Vector3)
		var to_w: Vector3 = skel_xform * (b.loc as Vector3)
		if from_w.distance_squared_to(to_w) < 1e-8:
			continue
		var query := PhysicsRayQueryParameters3D.create(from_w, to_w)
		query.collision_mask = world_collision_mask
		query.exclude = exclude
		query.hit_from_inside = false
		var hit := space.intersect_ray(query)
		if hit.is_empty():
			continue
		var normal: Vector3 = hit["normal"]
		var radius: float = maxf(float(b.radius) * world_collision_radius_scale, 0.001)
		var target_w: Vector3 = (hit["position"] as Vector3) + normal * (radius + world_collision_margin)
		var local_target := skel_inv * target_w
		b.loc = local_target
		b.prev_loc = (b.prev_loc as Vector3).lerp(local_target, 0.5)


func _apply_collision_limits(sk: Skeleton3D) -> void:
	_resolved_spheres = []
	_resolved_capsules = []
	_resolved_planes = []
	for lim in sphere_limits:
		var res := _resolve_limit(sk, lim, _sphere_limit_script)
		if res.is_empty() or float(lim.radius) <= 0.0:
			continue
		_resolved_spheres.append({
			loc = res.loc,
			rot = _limit_rot(res.rot, lim.offset_rotation_deg),
			radius = float(lim.radius),
			inner = int(lim.limit_type) == 1,
		})
	for lim in capsule_limits:
		var res := _resolve_limit(sk, lim, _capsule_limit_script)
		if res.is_empty() or float(lim.radius) <= 0.0 or float(lim.length) <= 0.0:
			continue
		_resolved_capsules.append({
			loc = res.loc,
			rot = _limit_rot(res.rot, lim.offset_rotation_deg),
			radius = float(lim.radius),
			length = float(lim.length),
		})
	for lim in plane_limits:
		var res := _resolve_limit(sk, lim, _plane_limit_script)
		if res.is_empty():
			continue
		var rot := _limit_rot(res.rot, lim.offset_rotation_deg)
		_resolved_planes.append({
			loc = res.loc,
			rot = rot,
			normal = rot * Vector3(0, 1, 0),
		})
	for b in _bones:
		for s in _resolved_spheres:
			b.loc = _core.adjust_by_sphere_limit(b.loc, b.radius, s.loc, s.radius, s.inner)
		for c in _resolved_capsules:
			b.loc = _core.adjust_by_capsule_limit(b.loc, b.radius, c.loc, c.rot, c.radius, c.length)
		for p in _resolved_planes:
			b.loc = _core.adjust_by_plane_limit(b.loc, b.prev_loc, b.radius, p.loc, p.normal)


func _apply_self_collision() -> void:
	if not self_collision_enabled or _bones.size() < 2:
		return
	_core.resolve_self_collision(_bones,
		self_collision_min_distance * self_collision_radius_multiplier,
		self_collision_push_split_ratio)


func _apply_sync_bones(sk: Skeleton3D) -> void:
	if sync_bones.is_empty():
		return
	for entry in sync_bones:
		if entry == null or not is_instance_valid(entry):
			continue
		if not bool(entry.get("enabled")):
			continue
		var src_name: String = String(entry.get("source_bone"))
		var dst_name: String = String(entry.get("target_bone"))
		var dst_i := sk.find_bone(dst_name)
		if dst_i < 0 or src_name.is_empty():
			continue
		var src_xform := Transform3D.IDENTITY
		var found := false
		for b in _bones:
			if int(b.index) >= 0 and String(b.name) == src_name:
				src_xform = Transform3D(Basis(b.get("final_rot", b.pose_rot)), b.loc)
				found = true
				break
		if not found:
			var src_i := sk.find_bone(src_name)
			if src_i < 0:
				continue
			src_xform = sk.get_bone_global_pose(src_i)
		var offset: Variant = entry.get("offset")
		var off: Transform3D = offset if offset is Transform3D else Transform3D.IDENTITY
		sk.set_bone_global_pose_override(dst_i, src_xform * off, 1.0, false)
		if not _written_bones.has(dst_i):
			_written_bones.append(dst_i)


func _update_debug_draw(sk: Skeleton3D) -> void:
	if not debug_draw:
		if _debug_mesh_instance != null and is_instance_valid(_debug_mesh_instance):
			_debug_mesh_instance.visible = false
			_debug_mesh.clear_surfaces()
		return
	_ensure_debug_nodes()
	_debug_mesh_instance.visible = true
	_debug_mesh_instance.global_transform = sk.global_transform
	_debug_mesh.clear_surfaces()
	for ci in range(_bones.size()):
		var b: Dictionary = _bones[ci]
		if b.is_dummy:
			continue
		if b.parent_idx >= 0:
			var p: Dictionary = _bones[b.parent_idx]
			_draw_line(p.loc, b.loc, Color(1, 1, 1))
		_draw_line(b.pose_loc, b.loc, Color(1, 1, 1))
		_draw_ring(b.loc, b.pose_rot, b.radius, Color(0, 1, 1))
	for s in _resolved_spheres:
		_draw_ring(s.loc, s.rot, s.radius, Color(1, 1, 0))
	for c in _resolved_capsules:
		_draw_ring(c.loc, c.rot, c.radius, Color(1, 0.5, 0))
		var axis: Vector3 = (c.rot as Quaternion) * Vector3(0, 0, 1) * (float(c.length) * 0.5)
		_draw_line(c.loc - axis, c.loc + axis, Color(1, 0.5, 0))
	for p in _resolved_planes:
		_draw_plane_quad(p)


func _ensure_debug_nodes() -> void:
	if _debug_mesh_instance != null and is_instance_valid(_debug_mesh_instance):
		return
	_debug_mesh = ImmediateMesh.new()
	_debug_material = StandardMaterial3D.new()
	_debug_material.shading_mode = BaseMaterial3D.SHADING_MODE_UNSHADED
	_debug_material.no_depth_test = true
	_debug_material.vertex_color_use_as_albedo = true
	_debug_mesh_instance = MeshInstance3D.new()
	_debug_mesh_instance.mesh = _debug_mesh
	_debug_mesh_instance.material_override = _debug_material
	_debug_mesh_instance.top_level = true
	add_child(_debug_mesh_instance)


func _draw_line(from_v: Vector3, to_v: Vector3, color: Color) -> void:
	_debug_mesh.surface_begin(Mesh.PRIMITIVE_LINES, _debug_material)
	_debug_mesh.surface_set_color(color)
	_debug_mesh.surface_add_vertex(from_v)
	_debug_mesh.surface_set_color(color)
	_debug_mesh.surface_add_vertex(to_v)
	_debug_mesh.surface_end()


func _draw_ring(center_v: Vector3, rot: Quaternion, radius: float, color: Color) -> void:
	var axes := [Vector3(1, 0, 0), Vector3(0, 1, 0), Vector3(0, 0, 1)]
	for i in range(3):
		var u: Vector3 = rot * axes[(i + 1) % 3]
		var v: Vector3 = rot * axes[(i + 2) % 3]
		_debug_mesh.surface_begin(Mesh.PRIMITIVE_LINES, _debug_material)
		for seg in range(16):
			var a0 := TAU * float(seg) / 16.0
			var a1 := TAU * float(seg + 1) / 16.0
			_debug_mesh.surface_set_color(color)
			_debug_mesh.surface_add_vertex(center_v + (u * cos(a0) + v * sin(a0)) * radius)
			_debug_mesh.surface_set_color(color)
			_debug_mesh.surface_add_vertex(center_v + (u * cos(a1) + v * sin(a1)) * radius)
		_debug_mesh.surface_end()


func _draw_plane_quad(p: Dictionary) -> void:
	var rot: Quaternion = p.rot
	var right: Vector3 = rot * Vector3(1, 0, 0) * 0.1
	var up: Vector3 = rot * Vector3(0, 1, 0) * 0.1
	var c: Vector3 = p.loc
	var corners := [c - right - up, c + right - up, c + right + up, c - right + up]
	_debug_mesh.surface_begin(Mesh.PRIMITIVE_LINES, _debug_material)
	for i in range(4):
		_debug_mesh.surface_set_color(Color(1, 0, 0))
		_debug_mesh.surface_add_vertex(corners[i])
		_debug_mesh.surface_set_color(Color(1, 0, 0))
		_debug_mesh.surface_add_vertex(corners[(i + 1) % 4])
	_debug_mesh.surface_end()


func _reset_to_pose(sk: Skeleton3D) -> void:
	if _bones.is_empty():
		_unwrite_override()
		return
	sk.force_update_all_bone_transforms()
	_snapshot_poses(sk, true)
	for b in _bones:
		b.loc = b.pose_loc
		b.prev_loc = b.pose_loc
		b.prev_prev_loc = b.pose_loc
	_unwrite_override()


func _unwrite_override() -> void:
	if _written_sk != null and is_instance_valid(_written_sk):
		var count := _written_sk.get_bone_count()
		for i in _written_bones:
			if i >= 0 and i < count:
				_written_sk.set_bone_global_pose_override(i, Transform3D(), 0.0, false)
	_written_bones = []
	_written_sk = null


func _write_back(sk: Skeleton3D, influence: float) -> void:
	var single_child := {}
	var written: Array[int] = []
	for ci in range(_bones.size()):
		var b: Dictionary = _bones[ci]
		if b.parent_idx >= 0:
			var p: Dictionary = _bones[b.parent_idx]
			if single_child.has(b.parent_idx):
				single_child[b.parent_idx] = -1
			else:
				single_child[b.parent_idx] = ci
	for ci in range(_bones.size()):
		var b: Dictionary = _bones[ci]
		if b.index < 0:
			continue
		var final_rot: Quaternion = b.pose_rot
		var child_ci: int = single_child.get(ci, -1)
		if child_ci >= 0:
			var child: Dictionary = _bones[child_ci]
			var cv: Vector3 = (child.pose_loc as Vector3) - (b.pose_loc as Vector3)
			var sv: Vector3 = (child.loc as Vector3) - (b.loc as Vector3)
			if cv.length() > EPS and sv.length() > EPS:
				var ndiff := cv.normalized() - sv.normalized()
				if ndiff.length() > 0.01:
					final_rot = _core.safe_between(cv, sv) * (b.pose_rot as Quaternion)
		b.final_rot = final_rot
		var xform := Transform3D(Basis(final_rot), b.loc)
		var pose_xform := Transform3D(Basis(b.pose_rot), b.pose_loc)
		var blended := pose_xform.interpolate_with(xform, influence)
		written.append(b.index)
		sk.set_bone_global_pose_override(b.index, blended, 1.0, false)
	_written_bones = written
	_written_sk = sk
