class_name KawaiiConstantForce
extends KawaiiExternalForce

## Постоянная внешняя сила (мировые единицы).

@export var force: Vector3 = Vector3(0.0, 0.0, 0.0)


func sample(_time: float, _world_pos: Vector3) -> Vector3:
	if not enabled:
		return Vector3.ZERO
	return force
