class_name KawaiiCapsuleLimit
extends Resource

@export var driving_bone: StringName = &""
@export var offset_location: Vector3 = Vector3.ZERO
@export var offset_rotation_deg: Vector3 = Vector3.ZERO
@export_range(0.0, 10.0) var radius: float = 0.05
@export_range(0.0, 10.0) var length: float = 0.10
@export var enabled: bool = true
