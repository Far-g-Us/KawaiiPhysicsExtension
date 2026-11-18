class_name KawaiiBoneConstraint
extends Resource

## Горизонтальная связь между двумя костями разных цепочек (например, соседние
## полосы ткани): расстояние между ними держится близким к анимированному.

@export var bone_a: StringName = &""
@export var bone_b: StringName = &""
@export_range(0.0, 1.0) var stiffness: float = 0.5
@export var enabled: bool = true
