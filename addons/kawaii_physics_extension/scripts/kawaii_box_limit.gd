class_name KawaiiBoxLimit
extends Resource

## Бокс-лимит на управляющей кости: симулированные кости выталкиваются наружу.

@export var driving_bone: StringName = &""
@export var offset_location: Vector3 = Vector3.ZERO
@export var offset_rotation_deg: Vector3 = Vector3.ZERO
@export var size: Vector3 = Vector3(0.2, 0.2, 0.2)
@export var enabled: bool = true
