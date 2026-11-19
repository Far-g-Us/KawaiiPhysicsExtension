class_name KawaiiExternalForce
extends Resource

## База внешней силы: возвращает силу в мире для точки. Наследники переопределяют sample().

@export var enabled: bool = true


func sample(_time: float, _world_pos: Vector3) -> Vector3:
	return Vector3.ZERO
