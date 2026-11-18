# KawaiiPhysics for Godot 4.7

Spring-bone physics for `Skeleton3D` — Godot port of the UE **KawaiiPhysicsExtension** (MIT). Every chain is simulated as a set of damped spring bones with angle limits, collision limits, world collision, wind and constraints. Manual stepping is supported (`active = false` + `process_modification_with_delta(delta)`).

## Install

Copy `addons/kawaii_physics_extension/` into your project, enable the plugin (optional; `class_name` scripts work without the editor plugin). Add `KawaiiPhysics` as a child of your `Skeleton3D`, set `root_bone`.

## Parameters

| Parameter | Type | Default | Description |
|---|---|---|---|
| `simulation_space` | enum | `COMPONENT` | `COMPONENT` (skeleton-local + move compensation) or `WORLD` (full inertia; `world_damping_*` ignored) |
| `root_bone` | StringName | — | Root of the chain; children are added automatically |
| `additional_root_bones` | Array[StringName] | `[]` | Extra independent chains |
| `exclude_bones` | Array[StringName] | `[]` | Bones (and their children) excluded from the chain |
| `settings` | KawaiiSettings | — | damping/stiffness/world damping/radius/limit angle + stiffness/damping curves |
| `bone_constraints` | Array[KawaiiBoneConstraint] | `[]` | Horizontal distance constraints between bones of different chains |
| `bone_subdivision_count` | int | 0 | Dummy bones inserted between real bones (0–10) |
| `dummy_bone_length` | float | 0 | Dummy tip bone length |
| `target_framerate` | int | 60 | Simulation reference framerate |
| `gravity` | Vector3 | (0, -9.8, 0) | World gravity |
| `wind` | KawaiiWind | null | Global wind resource |
| `wind_scale` | float | 1.0 | Wind multiplier |
| `wind_zones` | Array[KawaiiWindZone] | `[]` | Box/sphere wind zones |
| `constant_force` | Vector3 | 0 | Constant force |
| `teleport_distance_threshold` | float | 3.0 | Reset threshold for a skeleton jump |
| `teleport_rotation_threshold_deg` | float | 10.0 | Reset threshold for a skeleton rotation |
| `warmup_frames` | int | 0 | Warm-up steps |
| `sphere_limits` / `capsule_limits` / `plane_limits` | Array | [] | Limit resources attached to a driving bone |
| `world_collision_enabled` | bool | false | Bones collide with world physics |
| `world_collision_mask` | int | 1 | Physics layers |
| `world_collision_margin` | float | 0.005 | Push-out margin |
| `world_collision_radius_scale` | float | 1.0 | Radius multiplier |
| `self_collision_enabled` | bool | false | Bone-bone collision |
| `self_collision_min_distance` | float | 0.01 | Min distance |
| `self_collision_radius_multiplier` | float | 1.0 | Distance multiplier |
| `self_collision_push_split_ratio` | float | 0.5 | Push split between the pair |
| `sync_bones` | Array[KawaiiSyncBone] | `[]` | Target bones receive a simulated bone's global pose |
| `debug_draw` | bool | false | Debug lines/rings |

## Step order

simulate → limit collisions → self-collision → angle limits → restore lengths → bone constraints → restore lengths → world collision → write-back → sync bones → debug draw.

## Resources

- `KawaiiSettings` — damping, stiffness, world damping, collision radius, limit angle, optional stiffness/damping curves (multipliers by chain position, 0 = root, 1 = tip).
- `KawaiiBoneConstraint` — `bone_a`, `bone_b`, `stiffness`, `enabled`; pulls the distance toward the animated pose distance.
- `KawaiiWind` — `enabled`, `direction`, `strength`, `turbulence`, `noise_scale`, `noise_speed`, `noise_seed` (deterministic FastNoiseLite); `sample(time, world_pos)`.
- `KawaiiWindZone` (extends `KawaiiWind`) — `shape` (BOX/SPHERE), `center`, `size`, `radius`, `falloff`; zero outside.
- `KawaiiSyncBone` — `source_bone`, `target_bone`, `offset`, `enabled`.
- `KawaiiPhysicsManager` (Node3D) — LOD/quota: `target`, `lod_distance`, `max_active_chains`, `update_interval`, `enabled`, `print_stats`; distance is measured to the chain center (`KawaiiPhysics.get_chain_center()`); built-in stats `get_stats()` → chains/active/step_usec/step_usec_ema.

## World collision

Rays from the nearest real ancestor to each bone (dummy segments included), pushed to `hit.position + normal * (radius + margin)`; ancestor `CollisionObject3D`s are excluded automatically.

## Known limitations

- `simulation_space = WORLD` is an inertia policy (full move compensation), not a coordinate-frame switch: limits/constraints/world collision stay in skeleton space.
- `BASE_BONE` is not implemented: bones are skeleton-local and base-bone animation is already in the pose; it would only matter for external attachments.
- Subdivision dummies are fully integrated; there is no collision-only mode and no horizontal constraint subdivision.
- Editor preview is not implemented (would require `@tool` simulation).
- No box limits, mesh collision, cloth constraints, ragdoll, substeps, shared collision, or physics collision layers.
- `settings` is created in `_ready`, not `_init`.

## Tests

`tests/test_kawaii_*` in the host project: core, node, world collision, multi-root, subdivision, constraints, wind, wind zones, manager, moving skeleton, simulation space, curves, sync bones.

## Godot 4.7 note

The engine calls `_process_modification_with_delta(delta)` directly; with `active = true` the engine steps the modifier (`tests/smoke_kawaii.gd` proves 90/90 calls). Manual stepping requires `active = false`; do not write simulated poses back every frame unless you intend baking.
