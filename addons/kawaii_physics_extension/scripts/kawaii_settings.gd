class_name KawaiiSettings
extends Resource

@export_range(0.0, 1.0) var damping: float = 0.1
@export_range(0.0, 1.0) var stiffness: float = 0.05
@export_range(0.0, 1.0) var world_damping_location: float = 0.8
@export_range(0.0, 1.0) var world_damping_rotation: float = 0.8
@export_range(0.0, 10.0) var collision_radius: float = 0.03
@export_range(0.0, 180.0) var limit_angle_deg: float = 0.0
@export var stiffness_curve: Curve = null
@export var damping_curve: Curve = null
