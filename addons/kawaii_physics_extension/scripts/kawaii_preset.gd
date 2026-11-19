class_name KawaiiPreset
extends Resource

## Пресет настроек цепи: можно применять к модификатору (или назначить в preset).

@export var settings: KawaiiSettings = null
@export var bone_subdivision_count: int = 0
@export var bone_subdivision_collision_only: bool = false
@export var world_collision_enabled: bool = false
@export var self_collision_enabled: bool = false
@export var gravity: Vector3 = Vector3(0.0, -9.8, 0.0)
@export var constant_force: Vector3 = Vector3.ZERO
@export var apply_collisions: bool = true
@export var apply_gravity: bool = true
@export var apply_forces: bool = true


func apply_to(mod: Object) -> void:
	if mod == null:
		return
	if settings != null:
		mod.set("settings", settings.duplicate(true))
	mod.set("bone_subdivision_count", bone_subdivision_count)
	mod.set("bone_subdivision_collision_only", bone_subdivision_collision_only)
	if apply_collisions:
		mod.set("world_collision_enabled", world_collision_enabled)
		mod.set("self_collision_enabled", self_collision_enabled)
	if apply_gravity:
		mod.set("gravity", gravity)
	if apply_forces:
		mod.set("constant_force", constant_force)
