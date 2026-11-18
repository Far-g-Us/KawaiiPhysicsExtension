# Changelog — KawaiiPhysics (Godot port)

## 1.0.0
- API documentation in English (`README_EN.md`) and changelog.
- Stable API: world collision, additional roots, bone subdivision, bone constraints, wind and wind zones, physics manager (LOD/quota/stats), simulation spaces, curves, sync bones.

## 0.9.0
- `KawaiiSyncBone`: target bones receive a simulated bone's global pose (+offset).

## 0.8.0
- `stiffness_curve` / `damping_curve` in `KawaiiSettings` (multipliers by chain position).

## 0.7.0
- `simulation_space`: `COMPONENT` / `WORLD` (full inertia; `world_damping_*` ignored in WORLD).

## 0.6.1
- Manager: built-in step timing (`last_step_usec`, `get_stats()`, `print_stats`), distance to the chain center.

## 0.6.0
- `KawaiiPhysicsManager`: LOD distance, active-chain quota, reset on return, manual modifiers untouched.

## 0.5.0
- Per-bone wind sampling, `KawaiiWindZone` (box/sphere with falloff), `simulate_step` per-bone forces.

## 0.4.0
- `KawaiiWind` resource (direction, strength, turbulence, deterministic noise).

## 0.3.0
- Additional root bones, bone subdivision (dummy bones between real bones), bone constraints.

## 0.2.0
- World collision (rays from the nearest real ancestor, push-out along the normal), dynamic script resolution (folder-rename proof), `.uid`/class-cache fixes.

## 0.1.0
- MVP: spring chains, sphere/capsule/plane limits, self-collision, gravity/constant force, teleport handling, warm-up, manual stepping, debug draw.
