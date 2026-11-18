class_name KawaiiWind
extends Resource

## Ветер для KawaiiPhysics: направление, сила, турбулентность (детерминированный
## шум по времени и позиции). Возвращает силу в мировом пространстве.

@export var enabled: bool = true
@export var direction: Vector3 = Vector3(1.0, 0.0, 0.3)
@export_range(0.0, 100.0) var strength: float = 2.0
@export_range(0.0, 1.0) var turbulence: float = 0.3
@export var noise_scale: float = 0.6
@export_range(0.0, 5.0) var noise_speed: float = 0.5
@export var noise_seed: int = 1337

var _noise: FastNoiseLite = null


func _init() -> void:
	_noise = FastNoiseLite.new()
	_noise.seed = noise_seed
	_noise.noise_type = FastNoiseLite.TYPE_SIMPLEX_SMOOTH
	_noise.frequency = 1.0


func sample(time: float, world_pos: Vector3) -> Vector3:
	if not enabled:
		return Vector3.ZERO
	var dir := direction.normalized() if direction.length() > 0.001 else Vector3.RIGHT
	if _noise == null:
		_noise = FastNoiseLite.new()
		_noise.seed = noise_seed
		_noise.noise_type = FastNoiseLite.TYPE_SIMPLEX_SMOOTH
	_noise.seed = noise_seed
	var n := _noise.get_noise_3d(world_pos.x * noise_scale, world_pos.y * noise_scale + time * noise_speed, world_pos.z * noise_scale)
	var side := _noise.get_noise_3d(world_pos.z * noise_scale + 13.1, world_pos.x * noise_scale - 7.7, time * noise_speed * 0.7)
	dir = dir.rotated(Vector3.UP, side * 0.6 * turbulence)
	var mag := strength * (1.0 + turbulence * n)
	return dir * mag
