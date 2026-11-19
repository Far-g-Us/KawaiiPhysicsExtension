class_name KawaiiPhysicsPost
extends RefCounted

## Пост-обработка симуляции: бокс-лимиты, синхронизация костей, planar-режим.
## Вынесено из kawaii_physics.gd (лимит gdlint на длину файла).


static func apply_box_limits(bones: Array, core: GDScript, resolved: Array) -> void:
	if resolved.is_empty():
		return
	for b in bones:
		for bx in resolved:
			b.loc = core.adjust_by_box_limit(b.loc, b.radius, bx.loc, bx.rot, bx.half)


static func apply_sync(sk: Skeleton3D, bones: Array, entries: Array, written: Array) -> void:
	if entries.is_empty():
		return
	for entry in entries:
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
		for b in bones:
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
		if not written.has(dst_i):
			written.append(dst_i)


static func apply_planar(bones: Array, sk: Skeleton3D, normal: Vector3) -> void:
	if normal.length_squared() < 1e-6:
		return
	var nb: Vector3 = sk.global_transform.affine_inverse().basis * normal
	if nb.length_squared() < 1e-6:
		return
	nb = nb.normalized()
	for i in range(bones.size()):
		var b: Dictionary = bones[i]
		if b.parent_idx < 0:
			continue
		var parent: Dictionary = bones[b.parent_idx]
		var pose_len: float = ((b.pose_loc as Vector3) - (parent.pose_loc as Vector3)).length()
		if pose_len <= 1e-5:
			continue
		var off: Vector3 = (b.loc as Vector3) - (parent.loc as Vector3)
		off -= nb * off.dot(nb)
		if off.length_squared() <= 1e-10:
			off = (b.pose_loc as Vector3) - (parent.pose_loc as Vector3)
			off -= nb * off.dot(nb)
		if off.length_squared() <= 1e-10:
			continue
		b.loc = (parent.loc as Vector3) + off.normalized() * pose_len


static func apply_bridge(sk: Skeleton3D, space: PhysicsDirectSpaceState3D, bones: Array, ia: int, ib: int, count: int, scale: float, mask: int, margin: float, radius_scale: float) -> void:
	if scale <= 0.0 or count <= 0 or space == null:
		return
	var skel_inv_basis := sk.global_transform.affine_inverse().basis
	var rad: float = maxf((float(bones[ia].radius) + float(bones[ib].radius)) * 0.5 * radius_scale, 0.001)
	var shape := SphereShape3D.new()
	shape.radius = rad
	for k in range(1, count + 1):
		var t: float = float(k) / float(count + 1)
		var p_local: Vector3 = (bones[ia].loc as Vector3).lerp(bones[ib].loc as Vector3, t)
		var p_world: Vector3 = sk.global_transform * p_local
		var params := PhysicsShapeQueryParameters3D.new()
		params.shape = shape
		params.transform = Transform3D(Basis.IDENTITY, p_world)
		params.collision_mask = mask
		params.collide_with_areas = false
		params.collide_with_bodies = true
		var info := space.get_rest_info(params)
		if info.is_empty():
			continue
		var normal: Vector3 = info["normal"]
		var target_w: Vector3 = (info.get("point", p_world) as Vector3) + normal * (rad + margin)
		var push_local: Vector3 = skel_inv_basis * ((target_w - p_world) * scale)
		bones[ia].loc = (bones[ia].loc as Vector3) + push_local * (1.0 - t)
		bones[ib].loc = (bones[ib].loc as Vector3) + push_local * t


static func is_ignored_group(collider: Object, group: StringName) -> bool:
	if group == &"" or collider == null:
		return false
	var node := collider as Node
	while node != null:
		if node.is_in_group(group):
			return true
		node = node.get_parent()
	return false


static func apply_ccd(sk: Skeleton3D, space: PhysicsDirectSpaceState3D, bones: Array, exclude: Array[RID], mask: int, margin: float, radius_scale: float, exclude_group: StringName) -> void:
	if space == null:
		return
	var skel_xform := sk.global_transform
	var skel_inv := skel_xform.affine_inverse()
	for b in bones:
		if b.parent_idx < 0 or b.is_dummy:
			continue
		var to_w: Vector3 = skel_xform * (b.loc as Vector3)
		var prev_w: Vector3 = skel_xform * (b.prev_loc as Vector3)
		if prev_w.distance_squared_to(to_w) < 1e-8:
			continue
		var radius: float = maxf(float(b.radius) * radius_scale, 0.001)
		var shape := SphereShape3D.new()
		shape.radius = radius
		var sp := PhysicsShapeQueryParameters3D.new()
		sp.shape = shape
		sp.transform = Transform3D(Basis.IDENTITY, prev_w)
		sp.motion = to_w - prev_w
		sp.collision_mask = mask
		sp.exclude = exclude
		var frac := space.cast_motion(sp)
		if frac.size() != 2 or float(frac[1]) >= 1.0:
			continue
		sp.transform = Transform3D(Basis.IDENTITY, prev_w + (to_w - prev_w) * float(frac[1]))
		sp.motion = Vector3.ZERO
		var info := space.get_rest_info(sp)
		if info.is_empty() or is_ignored_group(info.get("collider"), exclude_group):
			continue
		var normal: Vector3 = info["normal"]
		var target_w: Vector3 = (info.get("point", prev_w) as Vector3) + normal * (radius + margin)
		b.loc = skel_inv * target_w
		b.prev_loc = b.loc


static func apply_mesh_collisions(sk: Skeleton3D, owner: Node, bones: Array, mesh_collisions: Array) -> void:
	if mesh_collisions.is_empty():
		return
	var skel_basis_inv := sk.global_transform.affine_inverse().basis
	var pushed: Array = []
	for mc in mesh_collisions:
		if mc == null or not is_instance_valid(mc) or not bool(mc.get("enabled")):
			continue
		var path_var: Variant = mc.get("target_path")
		if path_var == null:
			continue
		var target := owner.get_node_or_null(path_var as NodePath)
		if target == null:
			continue
		var target_sk := target as Skeleton3D
		if target_sk == null:
			continue
		var radius: float = float(mc.get("radius"))
		var names_var: Variant = mc.get("bone_names")
		var names: Array = names_var if names_var is Array else []
		var spheres: Array = []
		if names.is_empty():
			for i in range(target_sk.get_bone_count()):
				spheres.append(target_sk.global_transform * target_sk.get_bone_global_pose(i).origin)
		else:
			for n in names:
				var bi := target_sk.find_bone(String(n))
				if bi >= 0:
					spheres.append(target_sk.global_transform * target_sk.get_bone_global_pose(bi).origin)
		if spheres.is_empty():
			continue
		for b in bones:
			if b.is_dummy:
				continue
			var bp: Vector3 = sk.global_transform * (b.loc as Vector3)
			for center in spheres:
				var d: Vector3 = bp - (center as Vector3)
				var dist := d.length()
				var min_dist: float = radius + float(b.radius)
				if dist < min_dist:
					var dir: Vector3 = d / dist if dist > 1e-5 else Vector3.RIGHT
					b.loc = (b.loc as Vector3) + skel_basis_inv * (dir * (min_dist - dist))
					if not pushed.has(b):
						pushed.append(b)
					break
	if pushed.is_empty():
		return
	for _pass in 2:
		_relax_lengths(bones, pushed)
		_relax_dummies(bones)


static func _relax_lengths(bones: Array, pushed: Array) -> void:
	for b in pushed:
		var pi: int = int(b.parent_idx)
		if pi < 0 or pi >= bones.size():
			continue
		var parent: Dictionary = bones[pi]
		var pose_len: float = ((b.pose_loc as Vector3) - (parent.pose_loc as Vector3)).length()
		var off: Vector3 = (b.loc as Vector3) - (parent.loc as Vector3)
		if pose_len > 1e-5 and off.length_squared() > 1e-10:
			b.loc = (parent.loc as Vector3) + off.normalized() * pose_len


static func _relax_dummies(bones: Array) -> void:
	for b in bones:
		if not b.is_dummy or not bool(b.get("collision_only", false)):
			continue
		var ep: int = int(b.get("edge_p_ci", -1))
		var ec: int = int(b.get("edge_c_ci", -1))
		if ep < 0 or ec < 0 or ep >= bones.size() or ec >= bones.size():
			continue
		var t: float = float(b.get("sub_t", 0.5))
		b.loc = (bones[ep].loc as Vector3).lerp(bones[ec].loc as Vector3, t)
		b.prev_loc = b.loc


static func chain_center(sk: Skeleton3D, bones: Array, fallback: Vector3) -> Vector3:
	if sk == null or bones.is_empty():
		return fallback
	var sum := Vector3.ZERO
	var count := 0
	for b in bones:
		if b.is_dummy:
			continue
		sum += sk.global_transform * (b.loc as Vector3)
		count += 1
	if count == 0:
		return fallback
	return sum / float(count)


static func apply_shared(bones: Array, others: Array) -> void:
	var moved: Array = []
	for b in bones:
		if b.is_dummy:
			continue
		for other_bones in others:
			for ob in other_bones:
				if ob.is_dummy:
					continue
				var d: Vector3 = (b.loc as Vector3) - (ob.loc as Vector3)
				var min_d: float = float(b.radius) + float(ob.radius)
				var dist := d.length()
				if dist < min_d and dist > 1e-6:
					var push: Vector3 = d / dist * ((min_d - dist) * 0.5)
					b.loc = (b.loc as Vector3) + push
					if not moved.has(b):
						moved.append(b)
	if moved.is_empty():
		return
	for _pass in 2:
		_relax_lengths(bones, moved)


static func blend_ragdoll(xform: Transform3D, bone_name: String, src: Skeleton3D, weight: float) -> Transform3D:
	if src == null or weight <= 0.0:
		return xform
	var rp := source_bone_xform(src, bone_name)
	if rp == null:
		return xform
	var w: float = clampf(weight, 0.0, 1.0)
	return Transform3D(Basis(xform.basis.slerp((rp as Transform3D).basis, w)).orthonormalized(), xform.origin.lerp((rp as Transform3D).origin, w))


static func source_bone_xform(src: Skeleton3D, bone_name: String) -> Variant:
	for c in src.get_children():
		if c is PhysicalBoneSimulator3D:
			for pb in c.get_children():
				if pb is PhysicalBone3D and String(pb.bone_name) == bone_name:
					return src.global_transform.affine_inverse() * (pb as Node3D).global_transform
	var ri := src.find_bone(bone_name)
	if ri < 0:
		return null
	return src.get_bone_global_pose(ri)


static func update_debug(debug: RefCounted, owner: Node3D, sk: Skeleton3D, bones: Array, spheres: Array, capsules: Array, planes: Array, enabled: bool) -> void:
	if debug == null:
		return
	if not enabled:
		debug.hide()
		return
	debug.update(owner, sk, bones, spheres, capsules, planes)


static func collect_shared(registry: Array, self_ref: Object, skeleton: Skeleton3D) -> Array:
	var others: Array = []
	for m in registry:
		if m == self_ref or m == null or not is_instance_valid(m):
			continue
		if (m as Object).get("_skeleton") != skeleton:
			continue
		var mb: Array = (m as Object).get("_bones")
		if mb.is_empty():
			continue
		others.append(mb)
	return others


static func constraint_depth(name_to_chain: Dictionary, bones: Array, c: Resource) -> int:
	var ia: int = int(name_to_chain.get(String(c.get("bone_a")), -1))
	var ib: int = int(name_to_chain.get(String(c.get("bone_b")), -1))
	return maxi(bone_depth(bones, ia), bone_depth(bones, ib))


static func bone_depth(bones: Array, i: int) -> int:
	var d := 0
	var cur := i
	while cur >= 0 and cur < bones.size() and d < 64:
		cur = int(bones[cur].parent_idx)
		d += 1
	return d
