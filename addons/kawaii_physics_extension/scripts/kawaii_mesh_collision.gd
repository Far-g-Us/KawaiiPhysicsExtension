class_name KawaiiMeshCollision
extends Resource

## Коллизия с костями другого скелета (тело моба под одеждой/волосами):
## сферы в позициях костей target-скелета выталкивают кости цепи наружу.

@export var target_path: NodePath = NodePath("")
@export var bone_names: Array[StringName] = []
@export_range(0.0, 1.0) var radius: float = 0.05
@export var enabled: bool = true


