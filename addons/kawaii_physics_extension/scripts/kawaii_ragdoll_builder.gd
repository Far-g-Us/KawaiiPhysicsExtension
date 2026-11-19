class_name KawaiiRagdollBuilder
extends RefCounted

## Сборка физического ragdoll (PhysicalBoneSimulator3D) для Skeleton3D:
## PhysicalBone3D на выбранные кости (bone_name, поза кости, сфера-коллайдер),
## pin-joints, запуск с именами костей.

static func build(sk: Skeleton3D, only: Array[StringName] = [], anchor_root: bool = false) -> PhysicalBoneSimulator3D:
	var sim := PhysicalBoneSimulator3D.new()
	sim.name = "PhysicalRagdoll"
	sk.add_child(sim)
	for i in range(sk.get_bone_count()):
		var bname := sk.get_bone_name(i)
		if not only.is_empty() and not only.has(StringName(bname)):
			continue
		var pb := PhysicalBone3D.new()
		pb.name = String(bname)
		pb.bone_name = StringName(bname)
		pb.transform = sk.get_bone_global_pose(i)
		pb.joint_type = PhysicalBone3D.JOINT_TYPE_PIN
		pb.joint_offset = Transform3D.IDENTITY
		pb.joint_rotation = Vector3.ZERO
		sim.add_child(pb)
		var dir := _bone_dir_local(sk, i, pb)
		var length := dir.length()
		var shape := CollisionShape3D.new()
		var cap := CapsuleShape3D.new()
		cap.radius = clampf(length * 0.12, 0.008, 0.03)
		cap.height = maxf(length, cap.radius * 2.0 + 0.005)
		shape.shape = cap
		var dir_n: Vector3 = dir.normalized() if length > 1e-4 else Vector3.DOWN
		shape.transform = Transform3D(Basis(Quaternion(Vector3.UP, dir_n)), dir * 0.5)
		pb.add_child(shape)
	if anchor_root:
		_anchor(sk, sim)
	return sim


static func _bone_dir_local(sk: Skeleton3D, i: int, pb: PhysicalBone3D) -> Vector3:
	for c in range(sk.get_bone_count()):
		if sk.get_bone_parent(c) == i:
			return pb.transform.affine_inverse() * sk.get_bone_global_pose(c).origin
	var pi := sk.get_bone_parent(i)
	if pi >= 0:
		return pb.transform.affine_inverse() * sk.get_bone_global_pose(pi).origin
	return Vector3(0.0, -0.1, 0.0)


static func _anchor(sk: Skeleton3D, sim: PhysicalBoneSimulator3D) -> void:
	var selected: Dictionary = {}
	for c in sim.get_children():
		if c is PhysicalBone3D:
			selected[(c as PhysicalBone3D).bone_name] = c
	var parent_node := sk.get_parent()
	if parent_node == null:
		return
	for bname in selected:
		var bi := sk.find_bone(String(bname))
		var pi := sk.get_bone_parent(bi)
		var is_root := pi < 0 or not selected.has(sk.get_bone_name(pi))
		if not is_root:
			continue
		var pb: PhysicalBone3D = selected[bname]
		var anchor := StaticBody3D.new()
		anchor.name = "RagdollAnchor_" + String(bname)
		anchor.collision_layer = 0
		anchor.collision_mask = 0
		parent_node.add_child(anchor)
		anchor.global_position = pb.global_position
		var joint := PinJoint3D.new()
		joint.name = "RagdollPin_" + String(bname)
		parent_node.add_child(joint)
		joint.global_position = pb.global_position
		joint.node_a = joint.get_path_to(anchor)
		joint.node_b = joint.get_path_to(pb)


static func bone_length(sk: Skeleton3D, i: int) -> float:
	var parent_i := sk.get_bone_parent(i)
	if parent_i < 0:
		return 0.1
	return sk.get_bone_global_pose(i).origin.distance_to(sk.get_bone_global_pose(parent_i).origin)


static func start(sim: PhysicalBoneSimulator3D) -> void:
	var names: Array[StringName] = []
	for c in sim.get_children():
		if c is PhysicalBone3D:
			names.append((c as PhysicalBone3D).bone_name)
	sim.physical_bones_start_simulation(names)


static func count_bones(sim: PhysicalBoneSimulator3D) -> int:
	var n := 0
	for c in sim.get_children():
		if c is PhysicalBone3D:
			n += 1
	return n


static func pair_distances(sim: PhysicalBoneSimulator3D) -> Array:
	var out: Array = []
	var prev: Node3D = null
	for c in sim.get_children():
		if c is PhysicalBone3D:
			if prev != null:
				out.append(prev.global_position.distance_to((c as Node3D).global_position))
			prev = c
	return out
