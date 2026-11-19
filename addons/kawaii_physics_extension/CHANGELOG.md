# Changelog — KawaiiPhysics (Godot port)

## 1.20.2
- `KawaiiRagdollBuilder`: capsule colliders oriented along the bone (radius from the bone length) instead of spheres; optional `anchor_root` (static body + `PinJoint3D`) for a hanging ragdoll. Verified in the test project: capsules 3/3, anchored root moves 0.0005 m, segment lengths hold.


## 1.20.1
- `KawaiiRagdollBuilder` moved into the addon: pin joints, bone-length colliders, `start()` with bone names, `pair_distances()`; physical ragdoll joints verified on the simple chain and the Sammy subset (segment lengths preserved).


## 1.20.0
- Ragdoll blend prefers `PhysicalBone3D` transforms from a `PhysicalBoneSimulator3D` on the source skeleton (physical ragdoll support), falling back to `get_bone_global_pose`.
- Test project: physical ragdoll builder (`RagdollBuilder`), physical ragdoll probe (simple chain + Sammy subset), manager culling probe (`cull_by_view` near/behind swap).


## 1.19.1
- Critic fixes: null-safe constraint list, debug-draw instance creation restored, editor-only `_tool_settings` fallback (no scene dirtying), warmup runs the full post-step pipeline, manager keeps engine/manual classification stable on re-collect and applies per-LOD presets to manual chains, HangingLamp uses `step_manual`, spawners set `teleport_rotation_threshold_deg = 60` (die() no longer snaps), wind-zone falloff reaches zero at the boundary, impulse uses the frame dt, sleep updates its reference frames.
- Perf probe reworked to measure the full frame; 3 headless runs: Sammy 18 chains sim 1.50–1.65 ms avg (peak 2.4–3.1 ms), Human 0.36–0.38 ms; full frame ~6.8 ms (engine pacing). The sim number excludes the pose bake; treat it as a lower bound.


## 1.19.0
- Ragdoll blend: `ragdoll_blend` + `ragdoll_source` (blend the simulated pose toward a `Skeleton3D` ragdoll pose; verified on the 202-bone Sammy model).


## 1.18.0
- `step_manual(delta)` + `lod_enabled` for manually stepped chains; the manager now manages manual chains too (LOD/quota/budget) instead of ignoring them.
- Manager `cull_by_view` (camera frustum) in addition to distance.
- `KawaiiDebugDraw` is loaded dynamically (no global-class dependency; survives a stale editor class cache).
- Perf probe on the real game scenes: KawaiiSammy 18 chains ≈1.29 ms avg / 2.16 ms peak, KawaiiHuman 6 chains ≈0.33 ms avg (headless CPU).


## 1.17.0
- `gravity_scale` and `prediction_scale` (gravity/compensation multipliers).
- Integration tests for the game scenes: `KawaiiSammy.tscn` (18 chains) and `KawaiiHuman.tscn` (5 ragdoll chains) instantiate, build and step without errors.


## 1.16.0
- `SimSpace.BASE_NODE` + `base_node`: the move compensation is taken from an external node, so the chain lives in that frame and the skeleton's own motion does not drag it.


## 1.15.0
- Shared collision between chains of the same skeleton (`shared_collision_enabled`): a static registry of modifiers, sphere pair push with half penetration, segment lengths relaxed after the push.


## 1.14.0
- `constraint_leaf_first` (default true): horizontal constraints are applied leaf-first (heuristic solve order).
- Manager per-LOD presets: `lod_steps` + `lod_presets` (a `KawaiiPreset` applied when a chain crosses a distance step).
- Editor preview verified in the running editor via AgentBridge (tool script, `editor_preview` steps the simulation).


## 1.13.0
- `@tool` editor preview: `editor_preview` steps the simulation in the editor (manual, `active=false`); no group/debug-node pollution of the edited scene.
- Debug draw: velocity segments (cyan) in addition to lines/rings/plane quads.
- Debug drawing moved to `kawaii_debug_draw.gd` (`KawaiiDebugDraw`).


## 1.11.0
- Editor QoL: exported resource arrays are typed (`Array[KawaiiSyncBone]`, `Array[KawaiiWindZone]`, `Array[KawaiiExternalForce]`, `Array[KawaiiMeshCollision]`, `Array[KawaiiSphereLimit]`, `Array[KawaiiCapsuleLimit]`, `Array[KawaiiPlaneLimit]`, `Array[KawaiiBoxLimit]`), so the Inspector "Add Element" creates the right resource. NB: assigning an untyped `Array` to these properties is now rejected — use a typed array.
- Inspector export groups: Кости / Физика / Внешние силы / Телепорт и сон / Коллизии / Отладка.
- `get_chain_center` logic moved to `KawaiiPhysicsPost`.
- Synced missing `.uid` files (box limit, sync bone): all three addon copies are 17 `.gd` + 17 `.uid`.


## 1.10.1
- Mesh collision: pushed bones are re-normalized to their pose segment length (no chain stretching); collision-only dummies re-lerped; fixed dummy `edge_p_ci` (second dummy referenced the first dummy instead of the real edge parent).
- Verified on the 202-bone Sammy model (hair/tail/boob/ass chains, 11/11 checks).

## 1.10.0
- `KawaiiMeshCollision`: target-skeleton bone spheres push the chain out (own body under clothes).

## 1.9.0
- World collision CCD (swept sphere), `world_collision_exclude_group`, backstop recipe via OUTER sphere limit.

## 1.8.0
- External forces: `KawaiiExternalForce` base, `KawaiiCurveForce`, `KawaiiConstantForce`.

## 1.7.0
- Horizontal constraint subdivision + bridge feedback.

## 1.6.0
- `add_impulse`; post-processing split into `kawaii_physics_post.gd`.

## 1.5.0
- `planar_mode` / `plane_normal` (2.5D for 2D/billboard mobs).

## 1.4.0
- `KawaiiBoxLimit`.

## 1.3.0
- Substepping (`max_substep_dt`, `max_substeps`, `last_substeps`).

## 1.2.0
- `bone_subdivision_collision_only` with endpoint feedback.

## 1.1.0
- Physics sleep (`sleep_enabled/threshold/frames`, `wake()`), manager `frame_budget_usec`, `step_usec_ema` stats.


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
