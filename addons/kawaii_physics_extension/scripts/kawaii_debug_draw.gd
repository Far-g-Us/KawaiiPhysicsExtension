class_name KawaiiDebugDraw
extends RefCounted

## Отладочная отрисовка цепей, лимитов и скоростей (ImmediateMesh).

var mesh_instance: MeshInstance3D = null
var mesh: ImmediateMesh = null
var material: StandardMaterial3D = null


func update(owner: Node3D, sk: Skeleton3D, bones: Array, spheres: Array, capsules: Array, planes: Array) -> void:
	if mesh_instance == null or not is_instance_valid(mesh_instance):
		_ensure(owner)
	mesh_instance.visible = true
	mesh_instance.global_transform = sk.global_transform
	mesh.clear_surfaces()
	for ci in range(bones.size()):
		var b: Dictionary = bones[ci]
		if b.is_dummy:
			continue
		if b.parent_idx >= 0:
			var p: Dictionary = bones[b.parent_idx]
			_line(p.loc, b.loc, Color(1, 1, 1))
		_line(b.pose_loc, b.loc, Color(1, 1, 1))
		_ring(b.loc, b.pose_rot, b.radius, Color(0, 1, 1))
		var vel: Vector3 = (b.loc as Vector3) - (b.prev_loc as Vector3)
		_line(b.loc, (b.loc as Vector3) + vel * 20.0, Color(0.2, 1.0, 1.0))
	for s in spheres:
		_ring(s.loc, s.rot, s.radius, Color(1, 1, 0))
	for c in capsules:
		_ring(c.loc, c.rot, c.radius, Color(1, 0.5, 0))
		var axis: Vector3 = (c.rot as Quaternion) * Vector3(0, 0, 1) * (float(c.length) * 0.5)
		_line(c.loc - axis, c.loc + axis, Color(1, 0.5, 0))
	for p in planes:
		_plane_quad(p)


func hide() -> void:
	if mesh_instance != null and is_instance_valid(mesh_instance):
		mesh_instance.visible = false
		mesh.clear_surfaces()


func free_nodes() -> void:
	if mesh_instance != null and is_instance_valid(mesh_instance):
		mesh_instance.queue_free()
	mesh_instance = null
	mesh = null
	material = null


func _ensure(owner: Node3D) -> void:
	mesh = ImmediateMesh.new()
	material = StandardMaterial3D.new()
	material.shading_mode = BaseMaterial3D.SHADING_MODE_UNSHADED
	material.no_depth_test = true
	material.vertex_color_use_as_albedo = true
	mesh_instance = MeshInstance3D.new()
	mesh_instance.mesh = mesh
	mesh_instance.material_override = material
	mesh_instance.top_level = true
	owner.add_child(mesh_instance)


func _line(from_v: Vector3, to_v: Vector3, color: Color) -> void:
	mesh.surface_begin(Mesh.PRIMITIVE_LINES, material)
	mesh.surface_set_color(color)
	mesh.surface_add_vertex(from_v)
	mesh.surface_set_color(color)
	mesh.surface_add_vertex(to_v)
	mesh.surface_end()


func _ring(center_v: Vector3, rot: Quaternion, radius: float, color: Color) -> void:
	var axes := [Vector3(1, 0, 0), Vector3(0, 1, 0), Vector3(0, 0, 1)]
	for i in range(3):
		var u: Vector3 = rot * axes[(i + 1) % 3]
		var v: Vector3 = rot * axes[(i + 2) % 3]
		mesh.surface_begin(Mesh.PRIMITIVE_LINES, material)
		for seg in range(16):
			var a0 := TAU * float(seg) / 16.0
			var a1 := TAU * float(seg + 1) / 16.0
			mesh.surface_set_color(color)
			mesh.surface_add_vertex(center_v + (u * cos(a0) + v * sin(a0)) * radius)
			mesh.surface_set_color(color)
			mesh.surface_add_vertex(center_v + (u * cos(a1) + v * sin(a1)) * radius)
		mesh.surface_end()


func _plane_quad(p: Dictionary) -> void:
	var rot: Quaternion = p.rot
	var right: Vector3 = rot * Vector3(1, 0, 0) * 0.1
	var up: Vector3 = rot * Vector3(0, 1, 0) * 0.1
	var c: Vector3 = p.loc
	var corners := [c - right - up, c + right - up, c + right + up, c - right + up]
	mesh.surface_begin(Mesh.PRIMITIVE_LINES, material)
	for i in range(4):
		mesh.surface_set_color(Color(1, 0, 0))
		mesh.surface_add_vertex(corners[i])
		mesh.surface_set_color(Color(1, 0, 0))
		mesh.surface_add_vertex(corners[(i + 1) % 4])
	mesh.surface_end()
