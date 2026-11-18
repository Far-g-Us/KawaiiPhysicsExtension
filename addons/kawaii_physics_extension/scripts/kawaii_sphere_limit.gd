class_name KawaiiSphereLimit
extends Resource

enum LimitType { OUTER, INNER }

@export var driving_bone: StringName = &""
@export var offset_location: Vector3 = Vector3.ZERO
@export var offset_rotation_deg: Vector3 = Vector3.ZERO
@export_range(0.0, 10.0) var radius: float = 0.05
@export var limit_type: LimitType = LimitType.OUTER
@export var enabled: bool = true
