# KawaiiPhysicsExtension

Fork of [KawaiiPhysics](https://github.com/pafuhana1213/KawaiiPhysics) for **Unreal Engine 5.5**. Extended with character-focused features: body physics (HDT-SMP + UBE), clothing, weapons, and accessories.

> **Original plugin**: [pafuhana1213/KawaiiPhysics](https://github.com/pafuhana1213/KawaiiPhysics)  
> **This fork**: [Far-g-Us/KawaiiPhysicsExtension](https://github.com/Far-g-Us/KawaiiPhysicsExtension)  
> **Version**: 1.31.1 | **UE 5.5** | **Author: Far-g-Us**

---

## What's New in KawaiiPhysicsExtension

### UE 5.5 Compatibility
Full build in UE 5.5. All compilation errors from engine API changes fixed (18 fixes):

- `FPlatformTime::Microseconds()` → `FPlatformTime::Seconds() * 1000000`
- `FBoneReference::BoneIndex.IsValid()` → `HasValidSetup()`
- `GetLinkedAnimInstances()` (non-const) is now private in UE 5.5
- `FAnimInstanceProxy::AddCurveValue()` is now protected
- `GRHISupportsComputeShaders` removed
- Module dependencies: added UMG, RHI, RenderCore

### Bug Fixes (108 fixes)
- **Critical/High**: PredictedMove 2x double-counting, LODSettings cache, DrivingBone validity, Impulse primary AnimInstance, WindZone Verlet spike, Distance-LOD (Camera cache), Budget Allocator inverted logic, Bone Scaling not applied, Pose Gravity multipliers dead code, Self-Collision inverted distance condition
- **Medium**: RuntimeDebugDraw, EditMode ChildIndex bounds, RagdollBlend uninitialized rotation, Magic numbers, RemoveAt(0), Wind noise, NaN from zero axis, Substep drift, FrameCount never incremented
- **Low**: Teleport WorldSpace PrevLocation, Substep accumulator drift, Null SkelComp warnings, GPU fallback gravity, Log categories, deprecated arrays
- **Race conditions (3)**: Impulse queue deferred, Collision mutex, TSharedPtr FCriticalSection
- **API misuse (7)**: ComponentTransform Transient, FORCEINLINE, deprecated arrays, BlueprintType, int32 types, default values
- **Batch 5 (Claude review)**: Transfer Motion SourceBone init, DT²→DT, Camera culling GetPawn→GetViewTarget, Camera culling threshold→UPROPERTY

### 10 New Features

| # | Feature | UPROPERTY | Description |
|---|---------|-----------|-------------|
| 1 | **Heuristic Solve Order** | `bHeuristicSolveOrder` | XPBD solver: sort bones from leaves to root for fast convergence |
| 2 | **Physics Sleep** | `bEnablePhysicsSleep`, `SleepVelocityThreshold`, `SleepFrameCount` | Auto-freeze simulation when bones are still (CPU savings) |
| 3 | **Per-Chain Sim Space** | `SimulationSpaceOverride` in ChainPreset | Different spaces for different chains (hair=World, skirt=Component) |
| 4 | **Collision Groups** | `CollisionLayer` on node + each primitive | Bitmask filtering: hair=head, skirt=legs |
| 5 | **CCD (Edge Collision)** | `bEnableCCD`, `CCDSphereRadius` | Swept sphere between frames. Prevents tunneling |
| 6 | **Backstop Collision** | `bEnableBackstopCollision`, `BackstopRadius`, `BackstopStrength` | Per-bone hemisphere: prevents bones from penetrating the body |
| 7 | **Camera Culling** | `bEnableCameraFrustumCulling`, `CameraCullingDistance` | Skip simulation for invisible characters |
| 8 | **Pose Warp Smoothing** | `bEnablePoseWarpSmoothing`, `PoseWarpAngleThreshold`, `PoseWarpSmoothingSpeed` | Smooths sudden animation changes (hit reactions) |
| 9 | **2.5D Anime Mode** | `bEnable2DAnimeMode`, `AnimePlaneBlend`, `AnimePlaneNormal` | Constrain movement to a plane for anime-style physics |
| 10 | **Transfer Motion** | `bEnableTransferMotion`, `TransferMotionSources[]` | Angular velocity of source bone → external force |

### Character Focus
All new physics features are **disabled by default** — enable via boolean as needed:

| Feature | Description |
|---------|-------------|
| Body Physics (HDT-SMP + UBE) | Body physics: breast, abdomen, buttocks |
| Clothing Physics | Clothing physics with shape preservation |
| Weapon/Accessory Physics | Pendulum for weapons and accessories |
| Cloth-to-Body Collision | Clothing collision with body mesh |
| Character Physics Profile | DataAsset for character physics setup |
| Deterministic Mode | Fixed timestep (disabled by default) |

---

## Installation

### Option 1: Git Clone
```bash
cd YourProject/Plugins/
git clone https://github.com/Far-g-Us/KawaiiPhysicsExtension.git
```

### Option 2: Manual Installation
1. Download archive from [GitHub Releases](https://github.com/Far-g-Us/KawaiiPhysicsExtension/releases)
2. Extract to `YourProject/Plugins/KawaiiPhysicsExtension/`
3. Rebuild project

### Requirements
- **Unreal Engine 5.5** (5.4 and below not supported in this version)
- Visual Studio 2022+ or Rider
- Windows x64

---

## Plugin Structure

```
KawaiiPhysicsExtension/
├── Source/
│   ├── KawaiiPhysicsExtension/          # Runtime module
│   │   ├── Public/
│   │   │   ├── AnimNode_KawaiiPhysics.h
│   │   │   ├── KawaiiPhysics.h
│   │   │   ├── KawaiiPhysicsLibrary.h
│   │   │   ├── KawaiiPhysicsAnimDynamicsBridge.h
│   │   │   ├── KawaiiPhysicsGPUCompute.h
│   │   │   ├── KawaiiPhysicsCharacterProfile.h
│   │   │   ├── KawaiiPhysicsCollisionLimits.h
│   │   │   ├── KawaiiPhysicsTypes.h
│   │   │   └── ExternalForces/
│   │   │       ├── KawaiiPhysicsExternalForce.h
│   │   │       ├── KawaiiPhysicsExternalForce_Wind.h
│   │   │       ├── KawaiiPhysicsExternalForce_WindZone.h
│   │   │       └── KawaiiPhysicsExternalForce_Gravity.h
│   │   └── Private/
│   │       ├── AnimNode_KawaiiPhysics.cpp
│   │       ├── AnimNode_KawaiiPhysicsSimulation.cpp
│   │       ├── AnimNode_KawaiiPhysicsCollision.cpp
│   │       ├── AnimNode_KawaiiPhysicsModifyBones.cpp
│   │       ├── AnimNode_KawaiiPhysicsSyncBone.cpp
│   │       ├── KawaiiPhysics.cpp
│   │       ├── KawaiiPhysicsLibrary.cpp
│   │       ├── KawaiiPhysicsGPUCompute.cpp
│   │       └── ...
│   └── KawaiiPhysicsExtensionEd/        # Editor module
│       ├── Public/
│       └── Private/
│           ├── AnimGraphNode_KawaiiPhysics.cpp
│           ├── KawaiiPhysicsEd.cpp
│           └── KawaiiPhysicsEditMode.cpp
├── Content/
│   └── KawaiiPhysics/                   # Base assets
├── KawaiiPhysicsExtension.uplugin
└── README.md
```

---

## Core Features

### Anim Graph Node
**KawaiiPhysics** — main node in AnimGraph. Supports:
- Multiple bone chains with auto-detection
- World / BaseBone / Component simulation space + **Per-Chain Simulation Space**
- LOD-aware physics (auto-disable at far LODs)
- Substepping and WarmUp for stability
- Deterministic Mode (accumulator-based fixed timestep)

### Physics Features
- **Self-Collision** — bone-to-bone collision (Spatial Hash Grid O(n·k))
- **External Collision** — collision with meshes, capsules, spheres
- **Bone Constraints** — XPBD constraints between bones (Heuristic Solve Order)
- **Bone Subdivision** — automatic dummy bone insertion for flexibility
- **Planar Limits** — planar constraints
- **Twist/Swing Limits** — rotation constraints
- **CCD (Edge Collision)** — swept sphere prevents tunneling
- **Backstop Collision** — per-bone hemisphere prevents body penetration
- **Collision Groups** — bitmask layer filtering
- **Physics Sleep** — auto-freeze when bones are still
- **2.5D Anime Mode** — constrain movement to a plane for anime style
- **Transfer Motion** — angular velocity of bone → external force

### External Forces
Pluggable external force system:
- **Basic Force** — constant force (gravity, wind)
- **Curve Force** — animated force via CurveFloat
- **Gravity** — realistic gravity with configurable direction
- **Wind** — wind with noise and turbulence
- **Wind Zone** — wind zone with shape (Box/Sphere/Capsule)

### Performance
- **Camera Culling** — skip simulation for invisible characters
- **Distance LOD** — reduce quality by distance
- **Budget Allocator** — rate limiting simulation
- **Physics Sleep** — freeze when idle

### Character Focus
- **Pose Warp Smoothing** — smooth sudden animation changes
- **Warping Smoothing** — smooth actor teleports
- **Pose Blending** — pose-based multipliers (gravity, damping, stiffness)
- **Physics Material** — per-bone physics settings
- **Bone Scaling** — dynamic bone scaling during simulation

### Debug & Visualization
- Runtime debug draw: bones, collision shapes, velocity arrows
- Editor-only: bone hierarchy, constraint visualization
- Blueprint API for runtime parameter changes

---

## Blueprint API

```cpp
// Core functions
UKawaiiPhysicsLibrary::SetStiffness(Node, Value)
UKawaiiPhysicsLibrary::SetDamping(Node, Value)
UKawaiiPhysicsLibrary::SetWindStrength(Node, Value)
UKawaiiPhysicsLibrary::SetGravity(Node, FVector)

// Body Physics
UKawaiiPhysicsLibrary::SetBodyPhysicsEnabled(Node, bool)
UKawaiiPhysicsLibrary::SetBreastPhysics(Node, Stiffness, Damping)
UKawaiiPhysicsLibrary::SetButtPhysics(Node, Stiffness, Damping)

// Collision
UKawaiiPhysicsLibrary::SetExternalCollisionsEnabled(Node, bool)
UKawaiiPhysicsLibrary::AddExternalCollisionSphere(Node, BoneName, Radius, Offset)
UKawaiiPhysicsLibrary::SetSelfCollisionEnabled(Node, bool)

// New Features
UKawaiiPhysicsLibrary::SetPhysicsSleepEnabled(Node, bool)
UKawaiiPhysicsLibrary::SetCCDEnabled(Node, bool)
UKawaiiPhysicsLibrary::SetBackstopEnabled(Node, bool)
```

---

## Modules

| Module | Type | Description |
|--------|------|-------------|
| `KawaiiPhysicsExtension` | Runtime | Core physics, AnimGraph node |
| `KawaiiPhysicsExtensionEd` | UncookedOnly | Editor UI, debugging, preview |

### Module Dependencies

**KawaiiPhysicsExtension** (Public):
- Core, AnimGraphRuntime, CoreUObject, Engine, GameplayTags, DeveloperSettings

**KawaiiPhysicsExtension** (Private):
- Slate, SlateCore, UMG, RHI, RenderCore

**KawaiiPhysicsExtensionEd** (Private):
- Core, CoreUObject, Engine, InputCore, KawaiiPhysicsExtension
- AnimGraph, BlueprintGraph, Persona, UnrealEd, AnimGraphRuntime
- Slate, SlateCore, EditorFramework, AnimationEditMode

---

## Building

```bash
# Windows
"E:\Epic Games\UE_5.5\Engine\Build\BatchFiles\Build.bat" ^
    YourProjectEditor Win64 Development ^
    -Project="YourProject\YourProject.uproject" ^
    -WaitMutex
```

Or via Visual Studio / Rider:
1. Open `YourProject.uproject`
2. Build → Build Solution (Ctrl+Shift+B)

---

## Known Limitations

1. **GPU Simulation** — experimental feature. Auto-disables below 50 bones. Collision and constraints always run on CPU.
2. **GetLinkedAnimInstances()** — non-const version is private in UE 5.5. const_cast workaround used.
3. **FAnimInstanceProxy::AddCurveValue()** — protected in UE 5.5. Public UAnimInstance::AddCurveValue() used instead.
4. **Log Categories** — Runtime and Editor modules use different log categories due to DLL symbol export limitations.
5. **Camera Culling** — uses distance-based check (not precise frustum). Precise frustum check requires camera access from animation thread.

---

## Roadmap

- [x] GPU Simulation: experimental support (auto-disables below 50 bones)
- [x] Cloth Simulation: basic cloth physics (shear/bend/stretch constraints)
- [ ] Cosserat Rod: twist constraint for hair
- [ ] VRM SpringBone Import Bridge
- [ ] Batched Processing: World Subsystem for 20+ characters
- [ ] Platform-specific LOD Profiles

---

## Credits

- [pafuhana1213](https://github.com/pafuhana1213) — author of original KawaiiPhysics
- [Unreal Engine](https://www.unrealengine.com/) — Epic Games
- UE 5.5 API reference and community

---

## License

MIT License — see [LICENSE](LICENSE) for details.

---

*Last updated: v1.31.1 — UE 5.5 Compatible + 10 New Features*
