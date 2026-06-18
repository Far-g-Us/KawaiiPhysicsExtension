# KawaiiPhysicsExtension — Full API Documentation

## Contents

1. [Getting Started](#getting-started)
2. [Anim Graph Node — Parameters](#anim-graph-node)
3. [Blueprint API — Functions](#blueprint-api)
4. [Collision System](#collision-system)
5. [External Forces](#external-forces)
6. [Character Profile](#character-profile)
7. [Performance Features](#performance-features)
8. [Debug & Visualization](#debug)

---

## Getting Started

### Adding to AnimGraph

1. Open your character's AnimBP
2. Add a **KawaiiPhysics** node in the AnimGraph
3. Connect input pose to the node
4. Set `RootBone` — the chain's root bone (child bones are auto-detected)

### Quick Start via Blueprint

```
Event BeginPlay
  → Get Skeletal Mesh Component
  → ConvertToKawaiiPhysics (AnimNodeReference)
  → SetRootBoneName("hair_01")
  → SetPhysicsSettings(...)
```

---

## Anim Graph Node

### Core Parameters

| Parameter | Type | Default | Description |
|-----------|------|---------|-------------|
| `RootBone` | FBoneReference | — | Root bone. All child bones are auto-added to simulation |
| `ExcludeBones` | TArray<FBoneReference> | [] | Bones to exclude from simulation |
| `AdditionalRootBones` | TArray | [] | Additional root bones (for multi-chain setups) |
| `DummyBoneLength` | float | 0.0 | Length of dummy bones for physics at chain ends |
| `BoneForwardAxis` | Enum | X_Positive | Forward axis for bones |

### Physics Settings

| Parameter | Type | Default | Clamp | Description |
|-----------|------|---------|-------|-------------|
| `PhysicsSettings.Stiffness` | float | 0.08 | 0-1 | Spring stiffness (0=soft, 1=rigid) |
| `PhysicsSettings.Damping` | float | 0.58 | 0-1 | Damping (0=none, 1=instant stop) |
| `PhysicsSettings.Radius` | float | 1.0 | 0+ | Bone radius for collisions |
| `SimulationSpace` | Enum | ComponentSpace | — | WorldSpace / ComponentSpace / BaseBoneSpace |
| `TargetFramerate` | int32 | 60 | 1+ | Target framerate |

### Heuristic Solver

| Parameter | Type | Default | Description |
|-----------|------|---------|-------------|
| `bHeuristicSolveOrder` | bool | false | Sort constraints leaves→root. One iteration ≈ 2-3 regular iterations |

### Bone Subdivision

| Parameter | Type | Default | Clamp | Description |
|-----------|------|---------|-------|-------------|
| `BoneSubdivisionCount` | int32 | 0 | 0-10 | Number of dummy bones between real bones |
| `bBoneSubdivisionCollisionOnly` | bool | true | — | Collision-only (no integration) |
| `BoneConstraintSubdivisionCount` | int32 | 0 | 0-10 | Horizontal constraints between chains |

### Collision

| Parameter | Type | Default | Description |
|-----------|------|---------|-------------|
| `SphericalLimits` | TArray | [] | Spherical colliders |
| `CapsuleLimits` | TArray | [] | Capsule colliders |
| `BoxLimits` | TArray | [] | Box colliders |
| `PlanarLimits` | TArray | [] | Planar constraints |
| `bAllowWorldCollision` | bool | true | World collision |

### Self-Collision

| Parameter | Type | Default | Clamp | Description |
|-----------|------|---------|-------|-------------|
| `bEnableSelfCollision` | bool | false | — | Bone-to-bone collision |
| `SelfCollisionMinDistance` | float | 1.0 | 0.1-10 | Minimum distance between bones |
| `SelfCollisionRadiusMultiplier` | float | 1.0 | 0.5-2 | Radius multiplier |
| `SelfCollisionMaxCheckDistMultiplier` | float | 4.0 | 1-20 | Early culling distance multiplier |
| `SelfCollisionPushSplitRatio` | float | 0.5 | 0-1 | Push distribution (0=all to BoneB) |

### CCD (Continuous Collision Detection)

| Parameter | Type | Default | Clamp | Description |
|-----------|------|---------|-------|-------------|
| `bEnableCCD` | bool | false | — | Prevent tunneling via swept sphere |
| `CCDSphereRadius` | float | 0.5 | 0.1-10 | Swept sphere radius |

### Backstop Collision

| Parameter | Type | Default | Clamp | Description |
|-----------|------|---------|-------|-------------|
| `bEnableBackstopCollision` | bool | false | — | Per-bone hemisphere prevents body penetration |
| `BackstopRadius` | float | 2.0 | 0.1-20 | Hemisphere radius |
| `BackstopStrength` | float | 0.8 | 0-1 | Push strength |

### Collision Groups

| Parameter | Type | Default | Description |
|-----------|------|---------|-------------|
| `CollisionLayer` | int32 | 0 | Bitmask layer (0=collides with all) |

### Performance Features

| Parameter | Type | Default | Clamp | Description |
|-----------|------|---------|-------|-------------|
| `bEnablePhysicsSleep` | bool | false | — | Auto-freeze when motionless |
| `SleepVelocityThreshold` | float | 1.0 | 0.1-50 | Velocity threshold (cm/s) |
| `SleepFrameCount` | int32 | 5 | 1-60 | Consecutive frames below threshold |
| `bEnableCameraFrustumCulling` | bool | false | — | Skip invisible characters |
| `CameraCullingDistance` | float | 5000 | 100-50000 | Max distance from camera |
| `bEnableBudgetAllocator` | bool | false | — | Rate limiting simulation |
| `BudgetMicroSeconds` | float | 0 | 0+ | Budget (μs/frame) |

### Pose Blending

| Parameter | Type | Default | Clamp | Description |
|-----------|------|---------|-------|-------------|
| `bEnablePoseBlending` | bool | false | — | Pose-based multipliers |
| `PoseGravityStanding` | float | 1.0 | 0-2 | Gravity (standing) |
| `PoseGravitySitting` | float | 0.7 | 0-2 | Gravity (sitting) |
| `PoseGravityLying` | float | 0.3 | 0-2 | Gravity (lying) |
| `PoseDampingSitting` | float | 1.2 | 0.5-2 | Damping (sitting) |
| `PoseStiffnessLying` | float | 1.5 | 0.5-2 | Stiffness (lying) |

### Pose Warp Smoothing

| Parameter | Type | Default | Clamp | Description |
|-----------|------|---------|-------|-------------|
| `bEnablePoseWarpSmoothing` | bool | false | — | Smooth abrupt animation transitions |
| `PoseWarpAngleThreshold` | float | 45 | 5-180 | Angle threshold (degrees) |
| `PoseWarpSmoothingSpeed` | float | 10 | 1-50 | Smoothing speed |

### 2.5D Anime Mode

| Parameter | Type | Default | Clamp | Description |
|-----------|------|---------|-------|-------------|
| `bEnable2DAnimeMode` | bool | false | — | Constrain movement to plane |
| `AnimePlaneBlend` | float | 0.5 | 0-1 | 3D↔2D blend |
| `AnimePlaneNormal` | FVector | ForwardVector | — | Plane normal |

### Transfer Motion

| Parameter | Type | Default | Description |
|-----------|------|---------|-------------|
| `bEnableTransferMotion` | bool | false | Bone angular velocity → external force |
| `TransferMotionSources` | TArray | [] | Force sources |

---

## Blueprint API

### Basic Functions

```cpp
// Node conversion
FKawaiiPhysicsReference Ref = ConvertToKawaiiPhysics(AnimNodeReference);

// Reset dynamics (on teleport)
ResetDynamics(Ref);

// Enable/disable
SetPhysicsEnabled(Ref, true/false);
```

### Core Properties

```cpp
// Root bone
SetRootBoneName(Ref, "hair_01");
FName Root = GetRootBoneName(Ref);

// Excluded bones
SetExcludeBoneNames(Ref, {"spine_01", "pelvis"});
TArray<FName> Excluded = GetExcludeBoneNames(Ref);

// Physics settings
FKawaiiPhysicsSettings Settings;
Settings.Stiffness = 0.1f;
Settings.Damping = 0.6f;
Settings.Radius = 2.0f;
SetPhysicsSettings(Ref, Settings);

// Dummy bone length
SetDummyBoneLength(Ref, 5.0f);

// Bone subdivision
SetBoneSubdivisionCount(Ref, 3);
```

### Environment

```cpp
// Gravity
SetGravity(Ref, FVector(0, 0, -980));

// Wind
SetEnableWind(Ref, true);
SetWindScale(Ref, 1.5f);

// Collision
SetAllowWorldCollision(Ref, true);
```

### Twist/Swing Limits

```cpp
SetUseTwistSwingLimits(Ref, true);
SetTwistLimitAngle(Ref, 30.0f);      // degrees
SetSwingLimitYAngle(Ref, 45.0f);     // degrees
SetSwingLimitZAngle(Ref, 45.0f);     // degrees
```

### Per-Bone Control

```cpp
// Damping
SetBoneDamping(Ref, "hair_01", 0.7f);

// Stiffness
SetBoneStiffness(Ref, "hair_01", 0.15f);

// Collision radius
SetBoneCollisionRadius(Ref, "hair_01", 2.0f);

// Limit angle
SetBoneLimitAngle(Ref, "hair_01", 60.0f);
```

### Runtime Collision

```cpp
// Add sphere
AddRuntimeSphereCollision(Ref, "head", FVector(0,0,10), 15.0f, true);

// Add capsule
AddRuntimeCapsuleCollision(Ref, "hip", FVector(0,0,0), 10.0f, 20.0f);

// Add box
AddRuntimeBoxCollision(Ref, "chest", FVector(0,0,0), FVector(15,15,10));

// Add plane
AddRuntimePlaneCollision(Ref, "feet", FVector(0,0,0), FRotator::ZeroRotator);

// Add mesh collision
TArray<FVector> Vertices = GetConvexHullFromMesh(MyMesh);
AddRuntimeMeshCollision(Ref, "body", Vertices);

// Clear all
ClearRuntimeCollisions(Ref);
```

### External Forces

```cpp
// Add force
FInstancedStruct ForceStruct;
FBasicForce Force;
Force.ForceDirection = FVector(0, 0, -980);
Force.ForceStrength = 1.0f;
ForceStruct.InitializeAs<FBasicForce>(Force);
AddExternalForce(Ref, ForceStruct, OwnerObject);

// Remove forces
RemoveExternalForcesFromComponent(MeshComp, OwnerObject);
```

### Self-Collision

```cpp
SetSelfCollision(Ref, true);
```

### Spring Volume

```cpp
SetSpringVolume(Ref, true);
```

### Pose Blending

```cpp
SetPoseBlending(Ref, true);
EKawaiiPhysicsDetectedPose Pose = GetDetectedPose(Ref);
// Standing / Sitting / Lying

SetCustomPose(Ref, EKawaiiPhysicsDetectedPose::Standing);
```

### State-Driven Physics

```cpp
SetAnimationState(Ref, "Run");
AddStatePhysicsSettings(Ref, "Run", RunSettings);
AddStatePhysicsSettings(Ref, "Idle", IdleSettings);
```

### Morph Targets

```cpp
SetMorphTargetDriving(Ref, true);
AddMorphTargetMapping(Ref, "breath_chest", "chest_01");
AddMorphTargetMapping(Ref, "breath_belly", "belly_01");
ClearMorphTargetMappings(Ref);
```

### Bone Velocity

```cpp
FVector Velocity;
GetBoneVelocity(Ref, "hair_01", Velocity);

float Speed = GetBoneSpeed(Ref, "hair_01");

FVector AngVel;
GetBoneAngularVelocity(Ref, "hair_01", AngVel);
```

### Character Profile

```cpp
SetCharacterProfile(Ref, MyProfileAsset);
SetAutoDetectBoneChains(Ref, true);
AddSkipBones(Ref, {"spine_01", "pelvis", "spine_02"});
SetWeaponPhysics(Ref, true, 0.3f, 0.5f);
AddWeaponChainRoots(Ref, {"weapon_01", "amulet_01"});
SetBodyMeshCollision(Ref, true);
```

### Presets

```cpp
UKawaiiPhysicsPresetAsset* Presets = CreateDefaultJigglePresets(this);
ApplyPreset(Ref, Presets, "Breast");
ApplyPreset(Ref, Presets, "Hair");
ApplyPreset(Ref, Presets, "Clothing");

TArray<FName> Names = GetPresetNames(Presets);
```

### Physics Material

```cpp
SetPhysicsMaterial(Ref, MyPhysicsMaterial);
```

### Volume Preservation

```cpp
SetBoneVolumePreservation(Ref, "chest_01", 0.8f);
float Value;
GetBoneVolumePreservation(Ref, "chest_01", Value);

SetBonePressure(Ref, "chest_01", 2.0f);
GetBonePressure(Ref, "chest_01", Value);
```

### Bone Scaling

```cpp
SetBoneScaleCompensation(Ref, "chest_01", 1.0f);
float Comp;
GetBoneScaleCompensation(Ref, "chest_01", Comp);

FVector Scale;
GetBoneScale(Ref, "chest_01", Scale);
```

### Ragdoll Blend

```cpp
SetRagdollBlend(Ref, 0.5f, 5.0f); // Weight, Speed
```

### Spring/Damper

```cpp
SetBoneSpringDamper(Ref, "hair_01", 3.0f, 0.5f); // Frequency, DampingRatio
```

### Cloth Constraints

```cpp
SetBoneClothConstraints(Ref, "skirt_01", 0.1f, 0.1f, 1.2f, 0.8f);
// Shear, Bend, StretchLimit, CompressLimit
```

### Budget Allocator

```cpp
SetBudgetAllocator(Ref, true, 5000.0f, 5); // Enable, MicroSeconds, Priority
bool Skipped = WasBudgetSkipped(Ref);
```

### Physics State

```cpp
FKawaiiPhysicsSavedState State = SavePhysicsState(Ref);
// ... later ...
RestorePhysicsState(Ref, State);
```

### Shared Collision

```cpp
SetSharedCollisionSource(Ref, true);
SetSharedCollisionGroupTag(Ref, FGameplayTag::RequestGameplayTag("Hair"));
SetUseSharedCollision(Ref, true);
```

### Debug

```cpp
TArray<FKawaiiPhysicsDebugInfo> Info = GetAllBonesDebugInfo(Ref);
// For UMG visualization
```

---

## Collision System

### Collision Layer System

Every primitive and node has a `CollisionLayer` (int32 bitmask).

**Rule:** A primitive is checked only if `(Node.CollisionLayer & Primitive.CollisionLayer) != 0`. If both are 0, collision always occurs.

**Example:**
```
Node (hair):     CollisionLayer = 0x01
Node (skirt):    CollisionLayer = 0x02
Sphere (head):   CollisionLayer = 0x01   → collides with hair
Capsule (hip):   CollisionLayer = 0x02   → collides with skirt
```

### Runtime Collision Types

| Type | Function | Parameters |
|------|----------|------------|
| Sphere | `AddRuntimeSphereCollision` | BoneName, Offset, Radius, bOuter |
| Capsule | `AddRuntimeCapsuleCollision` | BoneName, Offset, Radius, Length |
| Box | `AddRuntimeBoxCollision` | BoneName, Offset, Extent |
| Plane | `AddRuntimePlaneCollision` | BoneName, Offset, Rotation |
| Mesh (Convex) | `AddRuntimeMeshCollision` | BoneName, Vertices, MeshScale |
| Mesh (Static) | `AddRuntimeStaticMeshCollision` | BoneName, StaticMesh, MeshScale |
| Mesh (Physics) | `AddRuntimePhysicsAssetCollision` | BoneName, PhysicsAsset, BodyName, MeshScale |

---

## External Forces

### Force Types

| Type | Class | Description |
|------|-------|-------------|
| Basic Force | `FKawaiiPhysicsExternalForce_Basic` | Constant acceleration |
| Curve Force | `FKawaiiPhysicsExternalForce_Curve` | Animated via CurveFloat |
| Gravity | `FKawaiiPhysicsExternalForce_Gravity` | Realistic gravity |
| Wind | `FKawaiiPhysicsExternalForce_Wind` | Wind with noise |
| Wind Zone | `FKawaiiPhysicsExternalForce_WindZone` | Wind zone (Box/Sphere/Capsule) |
| Custom | `UKawaiiPhysicsCustomExternalForce` | Custom class via C++ |

---

## Character Profile

### FKawaiiPhysicsChainPreset

| Field | Type | Description |
|-------|------|-------------|
| `ChainType` | EBoneChainType | Hair / Skirt / Tail / Breast / Weapon / Custom |
| `RootBoneName` | FName | Root bone name |
| `PhysicsSettings` | FKawaiiPhysicsSettings | Physics settings for this chain |
| `ChainCollisions` | TArray<FSphericalLimit> | Colliders for this chain |
| `bUseBodyMeshCollision` | bool | Use body mesh for collisions |
| `SimulationSpaceOverride` | EKawaiiPhysicsSimulationSpace | Override simulation space |

### Auto-Detection

Works via `BoneChainDetectionPatterns`:

```
Hair:     hair*, *hair*, twintail*, odango*
Skirt:    skirt*, *skirt*
Tail:     tail*, *tail*
Breast:   breast*, *breast*
Weapon:   weapon*, *weapon*, amulet*, *amulet*
```

---

## Performance Features

### Physics Sleep

When `bEnablePhysicsSleep = true`:
1. Each frame checks velocity of all bones
2. If all bones < `SleepVelocityThreshold` for `SleepFrameCount` frames → `bIsAsleep = true`
3. Simulation is skipped
4. Wake-up: component movement > `SleepVelocityThreshold * 10`

### Camera Culling

When `bEnableCameraFrustumCulling = true`:
1. In PreUpdate (GameThread) camera distance is cached
2. If distance > `CameraCullingDistance` → simulation is skipped

### Heuristic Solve Order

When `bHeuristicSolveOrder = true`:
1. XPBD constraints are sorted leaves→root
2. One iteration ≈ 2-3 iterations of regular order
3. Recommended: `BoneConstraintIterationCountBeforeCollision = 1`

### Budget Allocator

When `bEnableBudgetAllocator = true`:
1. Time between frames is measured
2. If elapsed < `BudgetMicroSeconds` → simulation is skipped
3. `WasBudgetSkipped()` returns true if skipped

---

## Debug

### Runtime Debug Overlay

```cpp
// Enable via console:
a.AnimNode.KawaiiPhysics.RuntimeDebug true

// Or via UPROPERTY:
bEnableRuntimeDebugDraw = true
```

### Blueprint Debug Info

```cpp
TArray<FKawaiiPhysicsDebugInfo> Info = GetAllBonesDebugInfo(Ref);
// Each element contains:
// - BoneName
// - Location, PrevLocation
// - Velocity
// - PhysicsSettings
// - bSkipSimulate
```

---

*Documentation: KawaiiPhysicsExtension v1.31.1 — UE 5.5*
