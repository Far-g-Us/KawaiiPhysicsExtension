class_name KawaiiPhysicsManager
extends Node3D

## LOD и квота активных цепей: далёкие/лишние модификаторы выключаются через
## `active = false` (движок их не шагает), при возврате — reset_dynamics().
## Не autoload: кладётся нодой в сцену.

@export var target: Node3D = null
@export_range(0.0, 200.0) var lod_distance: float = 30.0
@export_range(0, 512) var max_active_chains: int = 24
@export_range(0.02, 5.0) var update_interval: float = 0.25
@export var enabled: bool = true
@export var print_stats: bool = false

var _mods: Array = []
var _was_active: Dictionary = {}
var _timer := 0.0
var active_count := 0
var step_usec_total := 0
var step_usec_ema := 0.0


func _ready() -> void:
	add_to_group("kawaii_physics_manager")
	collect()


func collect() -> void:
	_mods.clear()
	_was_active.clear()
	if not is_inside_tree():
		return
	for m in get_tree().get_nodes_in_group("kawaii_physics"):
		if m is KawaiiPhysics and is_instance_valid(m):
			_mods.append(m)
			_was_active[m] = m.active


func _process(delta: float) -> void:
	if not enabled:
		return
	_timer -= delta
	if _timer > 0.0:
		return
	_timer = update_interval
	update_lod()


func _origin() -> Vector3:
	if target != null and is_instance_valid(target):
		return target.global_position
	var vp := get_viewport()
	if vp != null:
		var cam := vp.get_camera_3d()
		if cam != null:
			return cam.global_position
	return Vector3(INF, INF, INF)


func update_lod() -> void:
	var origin := _origin()
	if not origin.is_finite():
		return
	var arr: Array = []
	for m in _mods:
		if m == null or not is_instance_valid(m) or not m.is_inside_tree():
			continue
		if not bool(_was_active.get(m, false)):
			continue
		arr.append([m, (m as KawaiiPhysics).get_chain_center().distance_to(origin)])
	arr.sort_custom(func(a: Array, b: Array) -> bool: return float(a[1]) < float(b[1]))
	for i in range(arr.size()):
		var m: KawaiiPhysics = arr[i][0]
		var d: float = float(arr[i][1])
		var want: bool = i < max_active_chains and d <= lod_distance
		if want and not m.active:
			m.reset_dynamics()
			m.active = true
		elif not want and m.active:
			m.active = false
	active_count = 0
	step_usec_total = 0
	for m in _mods:
		if m != null and is_instance_valid(m) and m.active:
			active_count += 1
			step_usec_total += int(m.last_step_usec)
	step_usec_ema = lerpf(step_usec_ema, float(step_usec_total), 0.2) if step_usec_ema > 0.0 else float(step_usec_total)
	if print_stats:
		print("KawaiiPhysicsManager chains=%d active=%d step_us=%d ema=%.1f" % [_mods.size(), active_count, step_usec_total, step_usec_ema])


func get_stats() -> Dictionary:
	return {
		"chains": _mods.size(),
		"active": active_count,
		"step_usec": step_usec_total,
		"step_usec_ema": step_usec_ema,
	}
