# KawaiiPhysics for Godot 4.7

Spring-bone physics for `Skeleton3D` — Godot port of the UE **KawaiiPhysicsExtension** (MIT). Every chain is simulated as a set of damped spring bones with angle limits, collision limits, world collision, wind and constraints. Manual stepping is supported (`active = false` + `process_modification_with_delta(delta)`).

## Install

Copy `addons/kawaii_physics_extension/` into your project, enable the plugin (optional; `class_name` scripts work without the editor plugin). Add `KawaiiPhysics` as a child of your `Skeleton3D`, set `root_bone`.

## Parameters

| Parameter | Type | Default | Description |
|---|---|---|---|
| `simulation_space` | enum | `COMPONENT` | `COMPONENT` / `WORLD` / `BASE_NODE` (external base node frame) |
| `ragdoll_blend` | float | 0.0 | Blend weight toward the ragdoll source pose (0–1) |
| `ragdoll_source` | NodePath | &"" | `Skeleton3D` ragdoll-pose source (e.g., a skeleton under `PhysicalBoneSimulator3D`) |
| `base_node` | NodePath | &"" | External base node for `BASE_NODE` (relative to the modifier) |
| `root_bone` | StringName | — | Root of the chain; children are added automatically |
| `additional_root_bones` | Array[StringName] | `[]` | Extra independent chains |
| `exclude_bones` | Array[StringName] | `[]` | Bones (and their children) excluded from the chain |
| `settings` | KawaiiSettings | — | damping/stiffness/world damping/radius/limit angle + stiffness/damping curves |
| `preset` | KawaiiPreset | null | Preset: settings + subdivision + collisions + gravity; applied in `_ready` (or `apply_preset()`) |
| `bone_constraints` | Array[KawaiiBoneConstraint] | `[]` | Horizontal distance constraints between bones of different chains |
| `constraint_leaf_first` | bool | true | Solve constraints leaf-first (heuristic order) |
| `bone_subdivision_count` | int | 0 | Dummy bones inserted between real bones (0–10) |
| `bone_subdivision_collision_only` | bool | false | Dummies do not integrate, stay on the endpoint line and push the displacement back to endpoints |
| `dummy_bone_length` | float | 0 | Dummy tip bone length |
| `target_framerate` | int | 60 | Simulation reference framerate |
| `max_substep_dt` | float | 0.0 | Substepping: max dt per substep (0 = off) |
| `max_substeps` | int | 4 | Substeps per frame cap |
| `gravity` | Vector3 | (0, -9.8, 0) | World gravity |
| `gravity_scale` | float | 1.0 | Gravity multiplier |
| `prediction_scale` | float | 0.0 | Motion-compensation boost (prediction) |
| `wind` | KawaiiWind | null | Global wind resource |
| `planar_mode` | bool | false | 2.5D: keep the chain in a plane (2D/billboard mobs) |
| `plane_normal` | Vector3 | (0,0,1) | Plane normal (world) for planar_mode |
| `wind_scale` | float | 1.0 | Wind multiplier |
| `wind_zones` | Array[KawaiiWindZone] | `[]` | Box/sphere wind zones |
| `external_forces` | Array[KawaiiExternalForce] | `[]` | External forces (curve/constant), sampled per bone |
| `mesh_collisions` | Array[KawaiiMeshCollision] | `[]` | Target-skeleton bone spheres push the chain out (own body under clothes) |
| `constant_force` | Vector3 | 0 | Constant force |
| `teleport_distance_threshold` | float | 3.0 | Reset threshold for a skeleton jump |
| `teleport_rotation_threshold_deg` | float | 10.0 | Reset threshold for a skeleton rotation |
| `warmup_frames` | int | 0 | Warm-up steps |
| `sphere_limits` / `capsule_limits` / `plane_limits` / `box_limits` | Array | [] | Limit resources attached to a driving bone |
| `world_collision_enabled` | bool | false | Bones collide with world physics |
| `world_collision_mask` | int | 1 | Physics layers |
| `world_collision_margin` | float | 0.005 | Push-out margin |
| `world_collision_radius_scale` | float | 1.0 | Radius multiplier |
| `world_collision_ccd` | bool | false | Swept-sphere CCD (no tunneling through thin colliders) |
| `world_collision_exclude_group` | StringName | &"" | Bodies (and children) in this group are ignored |
| `self_collision_enabled` | bool | false | Bone-bone collision |
| `shared_collision_enabled` | bool | false | Collision with other chains of the same skeleton |
| `self_collision_min_distance` | float | 0.01 | Min distance |
| `self_collision_radius_multiplier` | float | 1.0 | Distance multiplier |
| `self_collision_push_split_ratio` | float | 0.5 | Push split between the pair |
| `sync_bones` | Array[KawaiiSyncBone] | `[]` | Target bones receive a simulated bone's global pose |
| `sleep_enabled` | bool | false | Auto-sleep when motionless |
| `sleep_motion_threshold` | float | 0.002 | Per-step bone motion threshold for sleep |
| `sleep_frames` | int | 5 | Steps below the threshold before sleeping |
| `editor_preview` | bool | false | @tool preview of the simulation in the editor (keep `active=false`) |
| `debug_draw` | bool | false | Debug lines/rings/velocities |

## Step order

simulate → limit collisions → self-collision → angle limits → restore lengths → bone constraints → restore lengths → world collision → write-back → sync bones → debug draw.

## Resources

- `KawaiiSettings` — damping, stiffness, world damping, collision radius, limit angle, optional stiffness/damping curves (multipliers by chain position, 0 = root, 1 = tip).
- `KawaiiBoneConstraint` — `bone_a`, `bone_b`, `stiffness`, `enabled`; pulls the distance toward the animated pose distance; `subdivision_count` + `feedback_scale` add bridge probes that push colliders out of the cloth gaps.
- `KawaiiWind` — `enabled`, `direction`, `strength`, `turbulence`, `noise_scale`, `noise_speed`, `noise_seed` (deterministic FastNoiseLite); `sample(time, world_pos)`.
- `KawaiiMeshCollision` — `target_path` (relative to the modifier), `bone_names` (empty = all), `radius`, `enabled`: bone spheres of the target skeleton push chain bones out; use it for a 3D mob's own body, which is excluded from world collision as an ancestor.
- `KawaiiPreset` — `settings` (duplicated on apply), `bone_subdivision_count`, `bone_subdivision_collision_only`, `world_collision_enabled`, `self_collision_enabled`, `gravity`, `constant_force`.
- `KawaiiExternalForce` base: `sample(time, world_pos)`; `KawaiiCurveForce` (direction/strength/curve/time_scale/phase — animated gusts) and `KawaiiConstantForce` (`force`).
- `KawaiiWindZone` (extends `KawaiiWind`) — `shape` (BOX/SPHERE/**CAPSULE**), `center`, `size`, `radius`, `height`, `axis`, `falloff`; zero outside.
- `KawaiiBoxLimit` — `driving_bone`, `offset_location`, `offset_rotation_deg`, `size`, `enabled`; pushes bones out along the minimal-penetration axis (after length restore, before world collision).
- `KawaiiSyncBone` — `source_bone`, `target_bone`, `offset`, `enabled`.
- `KawaiiPhysicsManager` (Node3D) — LOD/quota: `target`, `lod_distance`, `max_active_chains`, `update_interval`, `enabled`, `print_stats`; distance is measured to the chain center (`KawaiiPhysics.get_chain_center()`); built-in stats `get_stats()` → chains/active/step_usec/step_usec_ema/budget_skipped/sleeping; `frame_budget_usec` disables the farthest active chains until the EMA total fits (the nearest is always kept). Per-LOD: `lod_steps` + `lod_presets` apply a `KawaiiPreset` when a chain crosses a distance step.

## World collision

Rays from the nearest real ancestor to each bone (dummy segments included), pushed to `hit.position + normal * (radius + margin)`; ancestor `CollisionObject3D`s are excluded automatically. `world_collision_ccd` adds a swept-sphere test from `prev_loc` to `loc`; `world_collision_exclude_group` skips colliders in a group (own body). Backstop: an OUTER `KawaiiSphereLimit` on a body bone keeps chain bones out of the body.

## Impulses (hit reaction)

`add_impulse(impulse: Vector3, bone := &"", falloff := true)` — one-shot velocity change (m/s) applied through `prev_loc` (Verlet). Without `bone` it affects all real bones with falloff toward the tip; with `bone` it hits exactly that bone. Wakes a sleeping chain.

## 2D/3D planar mode

For 2D/billboard mobs: `planar_mode = true` with `plane_normal` (world, usually (0,0,1) for a side-scroller). After all collisions, bone offsets are projected onto the plane with preserved segment lengths, so the chain sways only in the screen plane. 3D mobs keep the mode off. The setting is per modifier, i.e. per chain/mob.

## KawaiiRagdollBuilder (physical ragdoll)

Static utility: `build(sk, only)` creates a `PhysicalBoneSimulator3D` with `PhysicalBone3D` nodes (bone_name, bone pose, pin joint, sphere collider sized from the bone length); `start(sim)` starts the simulation **with bone names** (without them the physical bones do not start); `pair_distances(sim)` checks segment lengths. Verified on Sammy: joints hold the lengths (0.3/0.3; tail 0.1428; hair 0.08–0.09) and the ragdoll blend to the physical source matches 4/4 bones.

## Ragdoll blend

`ragdoll_blend` + `ragdoll_source`: after the simulation each chain bone's pose is blended with the same-named bone's global pose in the source `Skeleton3D` (origin lerp, basis slerp). The source can be a skeleton driven by `PhysicalBoneSimulator3D` (a separate instance), so the Kawaii chain hands control to a physical ragdoll. With `ragdoll_blend = 1` the output equals the source pose; with 0 it is the simulation. Poses are taken in the source skeleton's space (rigs must match by bone names). If the source skeleton has a child `PhysicalBoneSimulator3D`, the blend reads the `PhysicalBone3D` transforms by `bone_name` — this is how a real physical ragdoll is used (the skeleton poses there are overrides and `get_bone_global_pose` does not see them).

## Manual stepping and the manager

When meshes hang on `BoneAttachment3D` and the engine path does not move them, step manually: `mod.active = false`, call `mod.step_manual(delta)` from your `_process` (respects `lod_enabled`) and write poses back. `KawaiiPhysicsManager` manages both engine-stepped (`active=true`) and manual chains: for manual ones it toggles `lod_enabled` (LOD/quota/budget/`cull_by_view`), and `step_manual` skips disabled chains. `debug_draw` in the editor works after Reload Project (`KawaiiDebugDraw` is loaded dynamically).

## Known limitations

- `simulation_space = WORLD` is an inertia policy; `BASE_NODE` takes the compensation from an external node.
- Subdivision: collision-only mode exists; edges to dummy tips and branches (2+ children) are not subdivided; horizontal constraints support subdivision + bridge feedback.
- `_anim_global` ignores overrides of other engine-driven modifiers on the same chain (outside ancestors are seen).
- Ragdoll: `ragdoll_blend` + `ragdoll_source` blend toward a separate skeleton's pose (e.g., under `PhysicalBoneSimulator3D`); the addon does not create a physical ragdoll skeleton.
- Twist is not expressible by the point solver (`limit_angle_deg` is bend/swing); pose multipliers only `gravity_scale`; warping replaced by teleport thresholds; bone scaling is already in pose lengths; no GPU/deterministic.
- No editor gizmos for limits (limits show in `debug_draw`/preview).
- `settings` is created in `_ready`; in the editor preview an internal `_tool_settings` is used and the scene is not marked dirty.
- The manager is not added to scenes by default; manual chains (`active=false`) are managed via `lod_enabled`, engine chains via `active`; the group is rescanned in `update_lod`.


## Tests

`tests/test_kawaii_*` in the host project: core, node, world collision, multi-root, subdivision, collision-only subdivision, constraints, wind, wind zones, manager, moving skeleton, simulation space, curves, sync bones, sleep/budget.

## Godot 4.7 note

The engine calls `_process_modification_with_delta(delta)` directly; with `active = true` the engine steps the modifier (`tests/smoke_kawaii.gd` proves 90/90 calls). Manual stepping requires `active = false`; do not write simulated poses back every frame unless you intend baking.
