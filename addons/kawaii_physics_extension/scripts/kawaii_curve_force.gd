class_name KawaiiCurveForce
extends KawaiiExternalForce

## Сила по кривой во времени: направление * strength * curve(фase+time*time_scale).

@export var direction: Vector3 = Vector3(1.0, 0.0, 0.0)
@export_range(0.0, 100.0) var strength: float = 2.0
@export var curve: Curve = null
@export var time_scale: float = 0.5
@export var phase: float = 0.0


func sample(time: float, _world_pos: Vector3) -> Vector3:
	if not enabled or curve == null or direction.length_squared() < 1e-8:
		return Vector3.ZERO
	var t := fposmod(time * time_scale + phase, 1.0)
	return direction.normalized() * strength * curve.sample(t)
