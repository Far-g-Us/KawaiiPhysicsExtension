class_name KawaiiWindZone
extends KawaiiWind

## Объёмная зона ветра: ветер действует только внутри box/sphere и плавно
## затухает к границе (falloff).

enum Shape { BOX, SPHERE }

@export var shape: Shape = Shape.BOX
@export var center: Vector3 = Vector3.ZERO
@export var size: Vector3 = Vector3(4.0, 3.0, 4.0)
@export var radius: float = 3.0
@export_range(0.0, 1.0) var falloff: float = 0.4


func inside_factor(world_pos: Vector3) -> float:
	var local := world_pos - center
	var d := 0.0
	if shape == Shape.SPHERE:
		if radius <= 0.0:
			return 0.0
		d = local.length() / radius
	else:
		var half := size * 0.5
		var nx := absf(local.x) / maxf(half.x, 0.001)
		var ny := absf(local.y) / maxf(half.y, 0.001)
		var nz := absf(local.z) / maxf(half.z, 0.001)
		d = maxf(nx, maxf(ny, nz))
	if d > 1.0:
		return 0.0
	return 1.0 - falloff * smoothstep(1.0 - falloff, 1.0, d) if falloff > 0.0 else 1.0


func sample(time: float, world_pos: Vector3) -> Vector3:
	var f := inside_factor(world_pos)
	if f <= 0.0:
		return Vector3.ZERO
	return super.sample(time, world_pos) * f
