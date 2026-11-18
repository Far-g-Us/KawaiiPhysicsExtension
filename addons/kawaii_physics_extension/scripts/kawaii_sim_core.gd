class_name KawaiiSimCore


const EPS := 1e-6


static func simulate_step(bones: Array, dt_old: float, dt: float, gravity: Vector3,
		skel_move_loc: Vector3, skel_move_rot: Quaternion, target_fps: float,
		bone_forces: Array = []) -> void:
	if dt <= EPS:
		return
	var exponent := target_fps * dt
	for ci in range(bones.size()):
		var b: Dictionary = bones[ci]
		if b.parent_idx < 0:
			b.prev_prev_loc = b.prev_loc
			b.prev_loc = b.loc
			b.loc = b.pose_loc
			continue
		var vel := Vector3.ZERO
		if dt_old > EPS:
			vel = (b.loc as Vector3) - (b.prev_loc as Vector3)
			vel /= dt_old
		b.prev_prev_loc = b.prev_loc
		b.prev_loc = b.loc
		vel *= (1.0 - b.damping)
		var g: Vector3 = gravity
		if ci < bone_forces.size():
			g = bone_forces[ci]
		vel += g * dt
		b.loc = (b.loc as Vector3) + vel * dt
		b.loc = (b.loc as Vector3) + skel_move_loc * (1.0 - b.world_damp_loc)
		var rotated := skel_move_rot * (b.prev_loc as Vector3)
		b.loc = (b.loc as Vector3) + (rotated - (b.prev_loc as Vector3)) * (1.0 - b.world_damp_rot)
		var parent: Dictionary = bones[b.parent_idx]
		var base: Vector3 = (parent.loc as Vector3) + ((b.pose_loc as Vector3) - (parent.pose_loc as Vector3))
		b.loc = (b.loc as Vector3) + (base - (b.loc as Vector3)) * (1.0 - pow(1.0 - b.stiffness, exponent))


static func restore_lengths(bones: Array) -> void:
	for b in bones:
		if b.parent_idx < 0:
			continue
		var parent: Dictionary = bones[b.parent_idx]
		var pose_len: float = ((b.pose_loc as Vector3) - (parent.pose_loc as Vector3)).length()
		var dir := (b.loc as Vector3) - (parent.loc as Vector3)
		var d := dir.length()
		if d <= EPS:
			continue
		b.loc = (parent.loc as Vector3) + dir * (pose_len / d)


static func adjust_by_angle_limit(bones: Array, i: int) -> void:
	var b: Dictionary = bones[i]
	if b.limit_angle_deg <= 0.0:
		return
	var parent: Dictionary = bones[b.parent_idx]
	var bone_dir := (b.loc as Vector3) - (parent.loc as Vector3)
	var bone_len := bone_dir.length()
	if bone_len <= EPS:
		return
	bone_dir /= bone_len
	var pose_dir := (b.pose_loc as Vector3) - (parent.pose_loc as Vector3)
	var pose_len := pose_dir.length()
	if pose_len <= EPS:
		return
	pose_dir /= pose_len
	var axis := pose_dir.cross(bone_dir)
	var angle := atan2(axis.length(), clampf(pose_dir.dot(bone_dir), -1.0, 1.0))
	var over := angle - deg_to_rad(b.limit_angle_deg)
	if over <= 0.0:
		return
	if axis.length() <= EPS:
		return
	var new_dir := bone_dir.rotated(axis.normalized(), -over)
	b.loc = (parent.loc as Vector3) + new_dir * bone_len


static func safe_between(from: Vector3, to: Vector3) -> Quaternion:
	if from.length_squared() < 1e-12 or to.length_squared() < 1e-12:
		return Quaternion.IDENTITY
	var f := from.normalized()
	var t := to.normalized()
	var dot := clampf(f.dot(t), -1.0, 1.0)
	if dot >= 1.0 - 1e-9:
		return Quaternion.IDENTITY
	if dot <= -1.0 + 1e-9:
		var axis := Vector3.UP.cross(f)
		if axis.length_squared() < 1e-12:
			axis = Vector3.RIGHT.cross(f)
		return Quaternion(axis.normalized(), PI)
	var axis2 := f.cross(t)
	if axis2.length_squared() < 1e-12:
		return Quaternion.IDENTITY
	return Quaternion(axis2.normalized(), acos(dot))


static func adjust_by_sphere_limit(loc: Vector3, bone_radius: float, sphere_loc: Vector3,
		sphere_radius: float, inner: bool) -> Vector3:
	if inner:
		var limit_inner := maxf(sphere_radius - bone_radius, 0.0)
		var delta := loc - sphere_loc
		var dist_sq := delta.length_squared()
		if dist_sq < limit_inner * limit_inner:
			return loc
		var dist := sqrt(dist_sq)
		if dist <= EPS:
			return sphere_loc
		return sphere_loc + limit_inner * (delta / dist)
	var limit_outer := sphere_radius + bone_radius
	var delta := loc - sphere_loc
	var dist_sq := delta.length_squared()
	if dist_sq > limit_outer * limit_outer:
		return loc
	var dist := sqrt(dist_sq)
	if dist <= EPS:
		return loc
	return loc + (limit_outer - dist) * (delta / dist)


static func adjust_by_capsule_limit(loc: Vector3, bone_radius: float, capsule_loc: Vector3,
		capsule_rot: Quaternion, capsule_radius: float, capsule_length: float) -> Vector3:
	var axis := capsule_rot * Vector3(0, 0, 1)
	var start := capsule_loc + axis * (capsule_length * 0.5)
	var end := capsule_loc - axis * (capsule_length * 0.5)
	var seg := end - start
	var closest := start
	if seg.length_squared() > EPS * EPS:
		var t := clampf((loc - start).dot(seg) / seg.length_squared(), 0.0, 1.0)
		closest = start + seg * t
	var limit := bone_radius + capsule_radius
	var delta := loc - closest
	var dist := delta.length()
	if dist < limit and dist > EPS:
		return closest + (delta / dist) * limit
	if dist < limit:
		return closest + (capsule_rot * Vector3(1, 0, 0)) * limit
	return loc


static func adjust_by_plane_limit(loc: Vector3, prev_loc: Vector3, bone_radius: float,
		plane_point: Vector3, plane_normal: Vector3) -> Vector3:
	var plane := Plane(plane_normal, plane_point)
	var on_plane := plane.project(loc)
	if (loc - on_plane).length_squared() < bone_radius * bone_radius:
		return on_plane + plane_normal * bone_radius
	var hit = plane.intersects_segment(loc, prev_loc)
	if hit != null:
		return on_plane + plane_normal * bone_radius
	return loc


static func resolve_self_collision(bones: Array, min_dist: float, push_ratio: float) -> void:
	if min_dist <= EPS:
		return
	var min_sq := min_dist * min_dist
	var n := bones.size()
	for i in range(n):
		var a: Dictionary = bones[i]
		if a.is_dummy:
			continue
		for j in range(i + 1, n):
			var b: Dictionary = bones[j]
			if b.is_dummy:
				continue
			if a.parent_idx == j or b.parent_idx == i:
				continue
			var delta: Vector3 = (b.loc as Vector3) - (a.loc as Vector3)
			var dist_sq := delta.length_squared()
			if dist_sq <= min_sq and dist_sq > EPS * EPS:
				var dist := sqrt(dist_sq)
				var push_dir := delta / dist
				var push := (min_dist - dist) * push_ratio
				a.loc = (a.loc as Vector3) - push_dir * push
				b.loc = (b.loc as Vector3) + push_dir * push
