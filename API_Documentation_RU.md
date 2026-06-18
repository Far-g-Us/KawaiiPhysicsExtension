# KawaiiPhysicsExtension — Полная документация API

## Содержание

1. [Начало работы](#начало-работы)
2. [Anim Graph Node — параметры](#anim-graph-node)
3. [Blueprint API — функции](#blueprint-api)
4. [Collision System](#collision-system)
5. [External Forces](#external-forces)
6. [Character Profile](#character-profile)
7. [Performance Features](#performance-features)
8. [Debug & Visualization](#debug)

---

## Начало работы

### Подключение к AnimGraph

1. Откройте AnimBP вашего персонажа
2. В AnimGraph добавьте нод **KawaiiPhysics**
3. Подключите input pose к ноду
4. Настройте `RootBone` — корневую кость цепочки (автоматически определяет все дочерние кости)

### Быстрый старт через Blueprint

```
Event BeginPlay
  → Get Skeletal Mesh Component
  → ConvertToKawaiiPhysics (AnimNodeReference)
  → SetRootBoneName("hair_01")
  → SetPhysicsSettings(...)
```

---

## Anim Graph Node

### Основные параметры

| Параметр | Тип | По умолчанию | Описание |
|----------|-----|-------------|----------|
| `RootBone` | FBoneReference | — | Корневая кость. Все дочерние кости автоматически добавляются в симуляцию |
| `ExcludeBones` | TArray<FBoneReference> | [] | Кости для исключения из симуляции |
| `AdditionalRootBones` | TArray | [] | Дополнительные корневые кости (для мультицепочек) |
| `DummyBoneLength` | float | 0.0 | Длина dummy-костей для физики на концах цепочек |
| `BoneForwardAxis` | Enum | X_Positive | Ось вперёд для кости |

### Physics Settings

| Параметр | Тип | По умолчанию | Clamp | Описание |
|----------|-----|-------------|-------|----------|
| `PhysicsSettings.Stiffness` | float | 0.08 | 0-1 | Жёсткость пружины (0=мягко, 1=жёстко) |
| `PhysicsSettings.Damping` | float | 0.58 | 0-1 | Затухание (0=нет, 1=мгновенная остановка) |
| `PhysicsSettings.Radius` | float | 1.0 | 0+ | Радиус кости для коллизий |
| `SimulationSpace` | Enum | ComponentSpace | — | WorldSpace / ComponentSpace / BaseBoneSpace |
| `TargetFramerate` | int32 | 60 | 1+ | Целевой framerate |

### Heuristic Solver

| Параметр | Тип | По умолчанию | Описание |
|----------|-----|-------------|----------|
| `bHeuristicSolveOrder` | bool | false | Сортировка constraint от листьев к корню. Одна итерация ≈ 2-3 обычных |

### Bone Subdivision

| Параметр | Тип | По умолчанию | Clamp | Описание |
|----------|-----|-------------|-------|----------|
| `BoneSubdivisionCount` | int32 | 0 | 0-10 | Количество dummy-костей между реальными |
| `bBoneSubdivisionCollisionOnly` | bool | true | — | Только для коллизий (без интеграции) |
| `BoneConstraintSubdivisionCount` | int32 | 0 | 0-10 | Горизонтальные constraints между цепочками |

### Collision

| Параметр | Тип | По умолчанию | Описание |
|----------|-----|-------------|----------|
| `SphericalLimits` | TArray | [] | Сферические коллайдеры |
| `CapsuleLimits` | TArray | [] | Капсульные коллайдеры |
| `BoxLimits` | TArray | [] | Боксовые коллайдеры |
| `PlanarLimits` | TArray | [] | Планарные ограничения |
| `bAllowWorldCollision` | bool | true | Столкновение с миром |

### Self-Collision

| Параметр | Тип | По умолчанию | Clamp | Описание |
|----------|-----|-------------|-------|----------|
| `bEnableSelfCollision` | bool | false | — | Столкновение костей друг с другом |
| `SelfCollisionMinDistance` | float | 1.0 | 0.1-10 | Минимальная дистанция между костями |
| `SelfCollisionRadiusMultiplier` | float | 1.0 | 0.5-2 | Множитель радиуса |
| `SelfCollisionMaxCheckDistMultiplier` | float | 4.0 | 1-20 | Множитель дистанции раннего отсечения |
| `SelfCollisionPushSplitRatio` | float | 0.5 | 0-1 | Распределение отталкивания (0=всё в BoneB) |

### CCD (Continuous Collision Detection)

| Параметр | Тип | По умолчанию | Clamp | Описание |
|----------|-----|-------------|-------|----------|
| `bEnableCCD` | bool | false | — | Предотвращает tunneling через swept sphere |
| `CCDSphereRadius` | float | 0.5 | 0.1-10 | Радиус swept sphere |

### Backstop Collision

| Параметр | Тип | По умолчанию | Clamp | Описание |
|----------|-----|-------------|-------|----------|
| `bEnableBackstopCollision` | bool | false | — | Per-bone hemisphere предотвращает penetration в тело |
| `BackstopRadius` | float | 2.0 | 0.1-20 | Радиус hemisphere |
| `BackstopStrength` | float | 0.8 | 0-1 | Сила отталкивания |

### Collision Groups

| Параметр | Тип | По умолчанию | Описание |
|----------|-----|-------------|----------|
| `CollisionLayer` | int32 | 0 | Bitmask слой (0=со всеми) |

### Performance Features

| Параметр | Тип | По умолчанию | Clamp | Описание |
|----------|-----|-------------|-------|----------|
| `bEnablePhysicsSleep` | bool | false | — | Автозаморозка при неподвижности |
| `SleepVelocityThreshold` | float | 1.0 | 0.1-50 | Порог скорости (cm/s) |
| `SleepFrameCount` | int32 | 5 | 1-60 | Кадров подряд ниже порога |
| `bEnableCameraFrustumCulling` | bool | false | — | Пропуск для невидимых персонажей |
| `CameraCullingDistance` | float | 5000 | 100-50000 | Макс. дистанция от камеры |
| `bEnableBudgetAllocator` | bool | false | — | Rate limiting симуляции |
| `BudgetMicroSeconds` | float | 0 | 0+ | Бюджет (μs/кадр) |

### Pose Blending

| Параметр | Тип | По умолчанию | Clamp | Описание |
|----------|-----|-------------|-------|----------|
| `bEnablePoseBlending` | bool | false | — | Pose-based multipliers |
| `PoseGravityStanding` | float | 1.0 | 0-2 | Гравитация (стоя) |
| `PoseGravitySitting` | float | 0.7 | 0-2 | Гравитация (сидя) |
| `PoseGravityLying` | float | 0.3 | 0-2 | Гравитация (лёжа) |
| `PoseDampingSitting` | float | 1.2 | 0.5-2 | Damping (сидя) |
| `PoseStiffnessLying` | float | 1.5 | 0.5-2 | Stiffness (лёжа) |

### Pose Warp Smoothing

| Параметр | Тип | По умолчанию | Clamp | Описание |
|----------|-----|-------------|-------|----------|
| `bEnablePoseWarpSmoothing` | bool | false | — | Сглаживание резких смен анимации |
| `PoseWarpAngleThreshold` | float | 45 | 5-180 | Порог угла (градусы) |
| `PoseWarpSmoothingSpeed` | float | 10 | 1-50 | Скорость сглаживания |

### 2.5D Anime Mode

| Параметр | Тип | По умолчанию | Clamp | Описание |
|----------|-----|-------------|-------|----------|
| `bEnable2DAnimeMode` | bool | false | — | Ограничение движения плоскостью |
| `AnimePlaneBlend` | float | 0.5 | 0-1 | Смешивание 3D↔2D |
| `AnimePlaneNormal` | FVector | ForwardVector | — | Нормаль плоскости |

### Transfer Motion

| Параметр | Тип | По умолчанию | Описание |
|----------|-----|-------------|----------|
| `bEnableTransferMotion` | bool | false | Angular velocity кости → внешняя сила |
| `TransferMotionSources` | TArray | [] | Источники силы |

---

## Blueprint API

### Базовые функции

```cpp
// Конвертация ноды
FKawaiiPhysicsReference Ref = ConvertToKawaiiPhysics(AnimNodeReference);

// Сброс динамики (при телепорте)
ResetDynamics(Ref);

// Управление включением
SetPhysicsEnabled(Ref, true/false);
```

### Core Properties

```cpp
// Корневая кость
SetRootBoneName(Ref, "hair_01");
FName Root = GetRootBoneName(Ref);

// Исключённые кости
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
// Гравитация
SetGravity(Ref, FVector(0, 0, -980));

// Ветер
SetEnableWind(Ref, true);
SetWindScale(Ref, 1.5f);

// Collision
SetAllowWorldCollision(Ref, true);
```

### Twist/Swing Limits

```cpp
SetUseTwistSwingLimits(Ref, true);
SetTwistLimitAngle(Ref, 30.0f);      // градусы
SetSwingLimitYAngle(Ref, 45.0f);     // градусы
SetSwingLimitZAngle(Ref, 45.0f);     // градусы
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
// Добавить сферу
AddRuntimeSphereCollision(Ref, "head", FVector(0,0,10), 15.0f, true);

// Добавить капсулу
AddRuntimeCapsuleCollision(Ref, "hip", FVector(0,0,0), 10.0f, 20.0f);

// Добавить бокс
AddRuntimeBoxCollision(Ref, "chest", FVector(0,0,0), FVector(15,15,10));

// Добавить plane
AddRuntimePlaneCollision(Ref, "feet", FVector(0,0,0), FRotator::ZeroRotator);

// Добавить mesh collision
TArray<FVector> Vertices = GetConvexHullFromMesh(MyMesh);
AddRuntimeMeshCollision(Ref, "body", Vertices);

// Очистить
ClearRuntimeCollisions(Ref);
```

### External Forces

```cpp
// Добавить силу
FInstancedStruct ForceStruct;
FBasicForce Force;
Force.ForceDirection = FVector(0, 0, -980);
Force.ForceStrength = 1.0f;
ForceStruct.InitializeAs<FBasicForce>(Force);
AddExternalForce(Ref, ForceStruct, OwnerObject);

// Удалить силы
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
// Для UMG визуализации
```

---

## Collision System

### Collision Layer System

Каждый primitive и нод имеет `CollisionLayer` (int32 bitmask).

**Правило:** Primitive проверяется только если `(Node.CollisionLayer & Primitive.CollisionLayer) != 0`. Если оба = 0 — столкновение всегда происходит.

**Пример:**
```
Нод (волосы):    CollisionLayer = 0x01
Нод (юбка):      CollisionLayer = 0x02
Сфера (голова):  CollisionLayer = 0x01   → сталкивается с волосами
Капсула (бедро): CollisionLayer = 0x02   → сталкивается с юбкой
```

### Runtime Collision Types

| Тип | Функция | Параметры |
|-----|---------|-----------|
| Sphere | `AddRuntimeSphereCollision` | BoneName, Offset, Radius, bOuter |
| Capsule | `AddRuntimeCapsuleCollision` | BoneName, Offset, Radius, Length |
| Box | `AddRuntimeBoxCollision` | BoneName, Offset, Extent |
| Plane | `AddRuntimePlaneCollision` | BoneName, Offset, Rotation |
| Mesh (Convex) | `AddRuntimeMeshCollision` | BoneName, Vertices, MeshScale |
| Mesh (Static) | `AddRuntimeStaticMeshCollision` | BoneName, StaticMesh, MeshScale |
| Mesh (Physics) | `AddRuntimePhysicsAssetCollision` | BoneName, PhysicsAsset, BodyName, MeshScale |

---

## External Forces

### Типы сил

| Тип | Класс | Описание |
|-----|-------|----------|
| Basic Force | `FKawaiiPhysicsExternalForce_Basic` | Постоянное ускорение |
| Curve Force | `FKawaiiPhysicsExternalForce_Curve` | Анимация через CurveFloat |
| Gravity | `FKawaiiPhysicsExternalForce_Gravity` | Реалистичная гравитация |
| Wind | `FKawaiiPhysicsExternalForce_Wind` | Ветер с шумом |
| Wind Zone | `FKawaiiPhysicsExternalForce_WindZone` | Зона ветра (Box/Sphere/Capsule) |
| Custom | `UKawaiiPhysicsCustomExternalForce` | Свой класс через C++ |

---

## Character Profile

### FKawaiiPhysicsChainPreset

| Поле | Тип | Описание |
|------|-----|----------|
| `ChainType` | EBoneChainType | Hair / Skirt / Tail / Breast / Weapon / Custom |
| `RootBoneName` | FName | Имя корневой кости |
| `PhysicsSettings` | FKawaiiPhysicsSettings | Настройки физики для цепочки |
| `ChainCollisions` | TArray<FSphericalLimit> | Коллайдеры для этой цепочки |
| `bUseBodyMeshCollision` | bool | Использовать body mesh для коллизий |
| `SimulationSpaceOverride` | EKawaiiPhysicsSimulationSpace | Override пространства симуляции |

### Auto-Detection

Работает через `BoneChainDetectionPatterns`:

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

Когда `bEnablePhysicsSleep = true`:
1. Каждый кадр проверяется скорость всех костей
2. Если все кости < `SleepVelocityThreshold` в течение `SleepFrameCount` кадров → `bIsAsleep = true`
3. Симуляция пропускается
4. Пробуждение: движение компонента > `SleepVelocityThreshold * 10`

### Camera Culling

Когда `bEnableCameraFrustumCulling = true`:
1. В PreUpdate (GameThread) кэшируется расстояние до камеры
2. Если расстояние > `CameraCullingDistance` → симуляция пропускается

### Heuristic Solve Order

Когда `bHeuristicSolveOrder = true`:
1. XPBD constraints сортируются от листьев к корню
2. Одна итерация ≈ 2-3 итераций обычного порядка
3. Рекомендуется: `BoneConstraintIterationCountBeforeCollision = 1`

### Budget Allocator

Когда `bEnableBudgetAllocator = true`:
1. Замеряется время между кадрами
2. Если прошло < `BudgetMicroSeconds` → симуляция пропускается
3. `WasBudgetSkipped()` возвращает true если пропущено

---

## Debug

### Runtime Debug Overlay

```cpp
// Включить через консоль:
a.AnimNode.KawaiiPhysics.RuntimeDebug true

// Или через UPROPERTY:
bEnableRuntimeDebugDraw = true
```

### Blueprint Debug Info

```cpp
TArray<FKawaiiPhysicsDebugInfo> Info = GetAllBonesDebugInfo(Ref);
// Каждый элемент содержит:
// - BoneName
// - Location, PrevLocation
// - Velocity
// - PhysicsSettings
// - bSkipSimulate
```

---

*Документация: KawaiiPhysicsExtension v1.31.1 — UE 5.5*
