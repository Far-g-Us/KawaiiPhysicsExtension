@tool
@icon("res://addons/kawaii_physics_extension/icon.svg")
class_name KawaiiPhysics
extends SkeletonModifier3D

var _core: GDScript = null
var _scripts_failed := false
var sleeping := false
var lod_enabled := true
var _sleep_counter := 0
var step_usec_ema := 0.0
var _elapsed := 0.0
var last_substeps := 1
var last_step_usec := 0
var _settings_script: GDScript = null
var _sphere_limit_script: GDScript = null
var _capsule_limit_script: GDScript = null
var _plane_limit_script: GDScript = null
var _box_limit_script: GDScript = null
var _post_script: GDScript = null


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
		_box_limit_script = load(base + "/kawaii_box_limit.gd")
		_post_script = load(base + "/kawaii_physics_post.gd")
		_debug_draw_script = load(base + "/kawaii_debug_draw.gd")
		break
	if _core == null:
		_scripts_failed = true
		push_error("KawaiiPhysics: scripts not found (addons folder renamed?)")
		return
	if settings == null and _settings_script != null and not Engine.is_editor_hint():
		settings = _settings_script.new()
const EPS := 1e-6

enum SimSpace { COMPONENT, WORLD, BASE_NODE }

@export var simulation_space: SimSpace = SimSpace.COMPONENT
@export var ragdoll_blend: float = 0.0
@export var ragdoll_source: NodePath = NodePath("")
@export var base_node: NodePath = NodePath("")
@export_group("Кости")
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
@export_group("Физика")
@export var preset: KawaiiPreset = null
@export var settings: KawaiiSettings
@export var constraint_leaf_first: bool = true
@export var bone_constraints: Array[KawaiiBoneConstraint] = []
@export var sync_bones: Array[KawaiiSyncBone] = []
@export var bone_subdivision_collision_only: bool = false
@export_range(0, 10) var bone_subdivision_count: int = 0:
	set(v):
		bone_subdivision_count = v
		_chain_dirty = true
@export var dummy_bone_length: float = 0.0:
	set(v):
		dummy_bone_length = v
		_chain_dirty = true
@export_range(1, 240) var target_framerate: int = 60
@export_range(0.0, 0.2) var max_substep_dt: float = 0.0
@export_range(1, 8) var max_substeps: int = 4
@export_range(0.0, 2.0) var gravity_scale: float = 1.0
@export_range(0.0, 2.0) var prediction_scale: float = 0.0
@export var gravity: Vector3 = Vector3(0, -9.8, 0)
@export_group("Внешние силы")
@export var wind: Resource = null
@export_range(0.0, 10.0) var wind_scale: float = 1.0
@export var wind_zones: Array[KawaiiWindZone] = []
@export var external_forces: Array[KawaiiExternalForce] = []
@export var mesh_collisions: Array[KawaiiMeshCollision] = []
@export var planar_mode: bool = false
@export var plane_normal: Vector3 = Vector3(0.0, 0.0, 1.0)
@export var constant_force: Vector3 = Vector3.ZERO
@export_group("Телепорт и сон")
@export var teleport_distance_threshold: float = 3.0
@export var teleport_rotation_threshold_deg: float = 10.0
@export var warmup_frames: int = 0
@export_group("Коллизии")
@export_subgroup("Лимиты")
@export var sphere_limits: Array[KawaiiSphereLimit] = []
@export var capsule_limits: Array[KawaiiCapsuleLimit] = []
@export var plane_limits: Array[KawaiiPlaneLimit] = []
@export var box_limits: Array[KawaiiBoxLimit] = []
@export_subgroup("Мир")
@export var world_collision_enabled: bool = false
@export_flags_3d_physics var world_collision_mask: int = 1
@export_range(0.0, 0.2) var world_collision_margin: float = 0.005
@export_range(0.0, 4.0) var world_collision_radius_scale: float = 1.0
@export var world_collision_ccd: bool = false
@export var world_collision_exclude_group: StringName = &""
@export_subgroup("Self-collision")
@export var shared_collision_enabled: bool = false
@export var self_collision_enabled: bool = false
@export_range(0.0, 10.0) var self_collision_min_distance: float = 0.01
@export_range(0.0, 10.0) var self_collision_radius_multiplier: float = 1.0
@export_range(0.0, 1.0) var self_collision_push_split_ratio: float = 0.5
@export_subgroup("Сон")
@export var sleep_enabled: bool = false
@export var sleep_motion_threshold: float = 0.002
@export_range(1, 120) var sleep_frames: int = 5
@export_group("Отладка")
@export var editor_preview: bool = false
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
var _prev_base_origin: Vector3 = Vector3.ZERO
var _prev_base_rot: Quaternion = Quaternion.IDENTITY
var _has_prev_base: bool = false
var _pending_warmup: int = 0
var _in_step: bool = false
var _resolved_spheres: Array = []
var _resolved_capsules: Array = []
var _resolved_planes: Array = []
var _resolved_boxes: Array = []
var _debug: RefCounted = null
var _tool_settings: KawaiiSettings = null
var _prev_frame_dt: float = 0.0
var _debug_draw_script: GDScript = null

static var _shared_registry: Array = []


func _init() -> void:
	pass


func _ready() -> void:
	if Engine.is_editor_hint():
		if editor_preview:
			_ensure_scripts()
		return
	add_to_group("kawaii_physics")
	if not _shared_registry.has(self):
		_shared_registry.append(self)
	_ensure_scripts()
	if preset != null:
		apply_preset(preset)


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
	if simulation_space != SimSpace.COMPONENT:
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
		elif prediction_scale > 0.0:
			move_loc *= (1.0 + prediction_scale)
	if teleported:
		_reset_to_pose(sk)
		_has_prev = false
		_has_prev_base = false
	if simulation_space == SimSpace.BASE_NODE and _has_prev and _has_prev_base:
		var base := get_node_or_null(base_node) as Node3D
		if base != null:
			var bnow := base.global_transform
			move_loc = bnow.affine_inverse() * _prev_base_origin
			move_rot = Quaternion(bnow.basis).inverse() * _prev_base_rot
	var skel_basis_inv := sk.global_transform.affine_inverse().basis
	var base_force: Vector3 = gravity * gravity_scale + constant_force
	var has_air: bool = false
	if wind != null and is_instance_valid(wind) and wind.has_method("sample") and bool(wind.get("enabled")):
		has_air = true
	if not wind_zones.is_empty():
		has_air = true
	if not external_forces.is_empty():
		has_air = true
	var bone_forces: Array = []
	if has_air:
		_elapsed += delta
		for b in _bones:
			var wpos: Vector3 = sk.global_transform * (b.loc as Vector3)
			var wf := Vector3.ZERO
			if wind != null and is_instance_valid(wind) and wind.has_method("sample") and bool(wind.get("enabled")):
				wf += (wind.call("sample", _elapsed, wpos) as Vector3) * wind_scale
			for z in wind_zones:
				if z != null and is_instance_valid(z) and z.has_method("sample") and bool(z.get("enabled")):
					wf += (z.call("sample", _elapsed, wpos) as Vector3) * wind_scale
			for f in external_forces:
				if f != null and is_instance_valid(f) and f.has_method("sample") and bool(f.get("enabled")):
					wf += f.call("sample", _elapsed, wpos) as Vector3
			bone_forces.append(skel_basis_inv * (base_force + wf))
	var gravity_sim := skel_basis_inv * base_force
	var dt := delta
	var dt_old := _prev_delta
	if dt_old <= EPS:
		dt_old = 1.0 / float(target_framerate)
	_snapshot_poses(sk)
	if sleeping:
		var skel_moved: bool = move_loc.length() > sleep_motion_threshold or absf(move_rot.get_angle()) > 0.002
		var pose_moved := false
		for b in _bones:
			if b.is_dummy:
				continue
			if (b.pose_loc as Vector3).distance_to(b.loc as Vector3) > sleep_motion_threshold * 4.0:
				pose_moved = true
				break
		if not skel_moved and not pose_moved:
			_prev_origin = now_origin
			_prev_rot = now_rot
			_prev_delta = delta
			last_step_usec = 0
			_in_step = false
			return
		sleeping = false
		_sleep_counter = 0
	if _pending_warmup > 0:
		var wdt := 1.0 / float(target_framerate)
		for i in range(_pending_warmup):
			_core.simulate_step(_bones, wdt, wdt, gravity_sim, Vector3.ZERO, Quaternion.IDENTITY, float(target_framerate), bone_forces)
			_apply_collision_limits(sk)
			_apply_self_collision()
			_apply_angle_limits()
			_core.restore_lengths(_bones)
			_apply_bone_constraints(sk)
			_core.restore_lengths(_bones)
			_apply_post_steps(sk)
		_pending_warmup = 0
	var substeps := 1
	if max_substep_dt > 0.0 and dt > max_substep_dt:
		substeps = clampi(int(ceil(dt / max_substep_dt)), 1, max_substeps)
	last_substeps = substeps
	var sdt := dt / float(substeps)
	var sdt_old := dt_old if substeps == 1 else sdt
	var sub_move: Vector3 = move_loc / float(substeps)
	var sub_rot: Quaternion = move_rot if substeps == 1 else Quaternion.IDENTITY.slerp(move_rot, 1.0 / float(substeps))
	for si in range(substeps):
		var old_dt := sdt_old if si == 0 else sdt
		_core.simulate_step(_bones, old_dt, sdt, gravity_sim, sub_move, sub_rot, float(target_framerate), bone_forces)
		_apply_collision_limits(sk)
		_apply_self_collision()
		_apply_angle_limits()
		_core.restore_lengths(_bones)
		_apply_bone_constraints(sk)
		_core.restore_lengths(_bones)
		_apply_post_steps(sk)
	_write_back(sk, get_influence())
	if _post_script != null:
		_post_script.apply_sync(sk, _bones, sync_bones, _written_bones)
	_update_debug_draw(sk)
	_prev_origin = now_origin
	_prev_rot = now_rot
	_prev_delta = sdt
	_prev_frame_dt = dt
	_has_prev = true
	if simulation_space == SimSpace.BASE_NODE:
		var base_end := get_node_or_null(base_node) as Node3D
		if base_end != null:
			_prev_base_origin = base_end.global_position
			_prev_base_rot = Quaternion(base_end.global_basis)
			_has_prev_base = true
	if sleep_enabled:
		var max_motion := 0.0
		for b in _bones:
			max_motion = maxf(max_motion, (b.loc as Vector3).distance_to(b.prev_loc as Vector3))
		if max_motion < sleep_motion_threshold:
			_sleep_counter += 1
			if _sleep_counter >= sleep_frames:
				sleeping = true
		else:
			_sleep_counter = 0
	_in_step = false
	last_step_usec = Time.get_ticks_usec() - t0
	step_usec_ema = lerpf(step_usec_ema, float(last_step_usec), 0.2) if step_usec_ema > 0.0 else float(last_step_usec)


func step_manual(delta: float) -> void:
	if not lod_enabled or _in_step or delta <= 0.0:
		return
	if settings == null and _settings_script != null:
		if _tool_settings == null:
			_tool_settings = _settings_script.new()
	_process_modification_with_delta(delta)


func _process(delta: float) -> void:
	if Engine.is_editor_hint() and editor_preview:
		step_manual(delta)


func _skeleton_changed(old_skeleton: Skeleton3D, new_skeleton: Skeleton3D) -> void:
	_unwrite_override()
	_skeleton = null
	_bones = []
	_name_to_chain = {}
	_has_prev = false
	_prev_delta = 0.0
	_chain_dirty = false


func _exit_tree() -> void:
	_shared_registry.erase(self)
	_unwrite_override()
	if _debug != null:
		_debug.free_nodes()
	_debug = null
	_skeleton = null
	_bones = []


func add_impulse(impulse: Vector3, bone: StringName = &"", falloff: bool = true) -> void:
	if impulse.length_squared() <= 1e-12:
		return
	sleeping = false
	_sleep_counter = 0
	var dt: float = 1.0 / float(target_framerate)
	if _prev_frame_dt > 1e-5:
		dt = _prev_frame_dt
	if bone != &"":
		var ci: int = int(_name_to_chain.get(String(bone), -1))
		if ci >= 0 and ci < _bones.size():
			_bones[ci].prev_loc = (_bones[ci].prev_loc as Vector3) - impulse * dt
		return
	var n: int = _bones.size()
	for i in range(n):
		var b: Dictionary = _bones[i]
		if b.is_dummy:
			continue
		var w := 1.0
		if falloff and n > 1:
			w = 1.0 - 0.7 * float(i) / float(n - 1)
		b.prev_loc = (b.prev_loc as Vector3) - impulse * dt * w


func apply_preset(p: Resource) -> void: if p is KawaiiPreset: (p as KawaiiPreset).apply_to(self)


func wake() -> void: sleeping = false; _sleep_counter = 0


func reset_dynamics() -> void:
	sleeping = false
	_sleep_counter = 0
	_has_prev = false
	_prev_delta = 0.0
	_pending_warmup = warmup_frames
	if _skeleton != null and is_instance_valid(_skeleton):
		_reset_to_pose(_skeleton)


func get_chain_center() -> Vector3: return _post_script.chain_center(_resolve_skeleton(), _bones, global_position) if _post_script != null else global_position


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
		var real_parent_new: int = remap[ci]
		var prev_new: int = real_parent_new
		for k in range(1, count + 1):
			var t := float(k) / float(count + 1)
			var d: Dictionary = b.duplicate(true)
			d.name = String(b.name) + "_sub%d" % k
			d.index = -1
			d.is_dummy = true
			d.sub_t = t
			d.edge_a = int(b.index)
			d.edge_b = int(child.index)
			d.collision_only = bone_subdivision_collision_only
			d.edge_p_ci = real_parent_new
			d.edge_child_old = child_i
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
	for b in _bones:
		if b.is_dummy and b.has("edge_child_old"):
			b.edge_c_ci = remap[int(b.edge_child_old)]
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
	var eff: KawaiiSettings = settings if settings != null else _tool_settings
	if eff == null and _settings_script != null and Engine.is_editor_hint():
		_tool_settings = _settings_script.new()
		eff = _tool_settings
	if eff == null or not (eff is KawaiiSettings):
		return
	var count: int = _bones.size()
	for i in range(count):
		var b: Dictionary = _bones[i]
		var t: float = 0.0 if count <= 1 else float(i) / float(count - 1)
		var stiff: float = float(eff.stiffness)
		var damp: float = float(eff.damping)
		if eff.stiffness_curve != null:
			stiff *= eff.stiffness_curve.sample(t)
		if eff.damping_curve != null:
			damp *= eff.damping_curve.sample(t)
		b.damping = damp
		b.stiffness = stiff
		b.world_damp_loc = eff.world_damping_location
		b.world_damp_rot = eff.world_damping_rotation
		b.radius = eff.collision_radius
		b.limit_angle_deg = eff.limit_angle_deg


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


func _apply_bone_constraints(sk: Skeleton3D) -> void:
	if bone_constraints.is_empty():
		return
	var order: Array = []
	for c in bone_constraints:
		if c != null and is_instance_valid(c):
			order.append(c)
	if constraint_leaf_first:
		order.sort_custom(func(a: Resource, b: Resource) -> bool:
			return _post_script.constraint_depth(_name_to_chain, _bones, a) > _post_script.constraint_depth(_name_to_chain, _bones, b))
	for c in order:
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
		if world_collision_enabled and int(c.subdivision_count) > 0 and is_inside_tree():
			var space := get_world_3d().direct_space_state
			if space != null and _post_script != null:
				_post_script.apply_bridge(sk, space, _bones, ia, ib, int(c.subdivision_count), float(c.feedback_scale), world_collision_mask, world_collision_margin, world_collision_radius_scale)


func _apply_box_limits() -> void: if _post_script != null: _post_script.apply_box_limits(_bones, _core, _resolved_boxes)


func _update_collision_only_dummies() -> void:
	for b in _bones:
		if not b.is_dummy or not bool(b.get("collision_only", false)):
			continue
		var ep: int = int(b.get("edge_p_ci", -1))
		var ec: int = int(b.get("edge_c_ci", -1))
		if ep < 0 or ec < 0 or ep >= _bones.size() or ec >= _bones.size():
			continue
		var t: float = float(b.get("sub_t", 0.5))
		var pa: Vector3 = _bones[ep].loc
		var pb: Vector3 = _bones[ec].loc
		b.loc = pa.lerp(pb, t)
		b.prev_loc = b.loc


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
	if world_collision_ccd and _post_script != null:
		_post_script.apply_ccd(sk, space, _bones, exclude, world_collision_mask, world_collision_margin, world_collision_radius_scale, world_collision_exclude_group)
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
		if _post_script != null and _post_script.is_ignored_group(hit.get("collider"), world_collision_exclude_group):
			continue
		var normal: Vector3 = hit["normal"]
		var radius: float = maxf(float(b.radius) * world_collision_radius_scale, 0.001)
		var target_w: Vector3 = (hit["position"] as Vector3) + normal * (radius + world_collision_margin)
		var local_target := skel_inv * target_w
		if b.is_dummy and bool(b.get("collision_only", false)):
			var ep: int = int(b.get("edge_p_ci", -1))
			var ec: int = int(b.get("edge_c_ci", -1))
			if ep >= 0 and ec >= 0 and ep < _bones.size() and ec < _bones.size():
				var t: float = float(b.get("sub_t", 0.5))
				var push_local: Vector3 = local_target - (b.loc as Vector3)
				_bones[ep].loc = (_bones[ep].loc as Vector3) + push_local * (1.0 - t)
				_bones[ec].loc = (_bones[ec].loc as Vector3) + push_local * t
				b.loc = (_bones[ep].loc as Vector3).lerp(_bones[ec].loc as Vector3, t)
				b.prev_loc = b.loc
				continue
		b.loc = local_target
		b.prev_loc = (b.prev_loc as Vector3).lerp(local_target, 0.5)


func _apply_collision_limits(sk: Skeleton3D) -> void:
	_resolved_spheres = []
	_resolved_capsules = []
	_resolved_planes = []
	_resolved_boxes = []
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
	for lim in box_limits:
		var res := _resolve_limit(sk, lim, _box_limit_script)
		if res.is_empty():
			continue
		var half: Vector3 = (lim.size as Vector3) * 0.5
		if half.x <= 0.0 or half.y <= 0.0 or half.z <= 0.0:
			continue
		_resolved_boxes.append({
			loc = res.loc,
			rot = Basis(_limit_rot(res.rot, lim.offset_rotation_deg)),
			half = half,
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


func _update_debug_draw(sk: Skeleton3D) -> void:
	if _debug == null and _debug_draw_script != null:
		_debug = _debug_draw_script.new()
	_post_script.update_debug(_debug, self, sk, _bones, _resolved_spheres, _resolved_capsules, _resolved_planes, debug_draw)


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


func _apply_post_steps(sk: Skeleton3D) -> void:
	_apply_box_limits()
	_update_collision_only_dummies()
	_apply_world_collision(sk)
	if _post_script != null:
		_post_script.apply_mesh_collisions(sk, self, _bones, mesh_collisions)
	_apply_shared_collision()
	_update_collision_only_dummies()
	if planar_mode and _post_script != null:
		_post_script.apply_planar(_bones, sk, plane_normal)


func _apply_shared_collision() -> void:
	if shared_collision_enabled and _post_script != null:
		var others: Array = _post_script.collect_shared(_shared_registry, self, _skeleton)
		_post_script.apply_shared(_bones, others)


func _write_back(sk: Skeleton3D, influence: float) -> void:
	var rag_src: Skeleton3D = (get_node_or_null(ragdoll_source) as Skeleton3D) if ragdoll_blend > 0.0 and ragdoll_source != NodePath("") else null
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
		if rag_src != null and _post_script != null:
			xform = _post_script.blend_ragdoll(xform, String(b.name), rag_src, ragdoll_blend)
		b.write_xform = xform
		var pose_xform := Transform3D(Basis(b.pose_rot), b.pose_loc)
		var blended := pose_xform.interpolate_with(xform, influence)
		written.append(b.index)
		sk.set_bone_global_pose_override(b.index, blended, 1.0, false)
	_written_bones = written
	_written_sk = sk
