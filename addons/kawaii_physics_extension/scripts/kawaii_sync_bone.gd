class_name KawaiiSyncBone
extends Resource

## Синхронизация кости: целевая кость получает глобальную позу исходной
## (симулированной) с опциональным offset.

@export var source_bone: StringName = &""
@export var target_bone: StringName = &""
@export var offset: Transform3D = Transform3D.IDENTITY
@export var enabled: bool = true
