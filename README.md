# KawaiiPhysicsExtension

Fork of [KawaiiPhysics](https://github.com/pafuhana1213/KawaiiPhysics) for **Unreal Engine 5.5**. Extended with character-focused features: body physics (HDT-SMP + UBE), clothing, weapons, and accessories.

> **Original plugin**: [pafuhana1213/KawaiiPhysics](https://github.com/pafuhana1213/KawaiiPhysics)  
> **This fork**: [Far-g-Us/KawaiiPhysicsExtension](https://github.com/Far-g-Us/KawaiiPhysicsExtension)  
> **Version**: 1.31.1 | **UE 5.5** | **Author: Far-g-Us**

---

## Что нового в KawaiiPhysicsExtension

### UE 5.5 Совместимость
Полная сборка в UE 5.5. Исправлены все ошибки компиляции, вызванные изменениями API движка (18 исправлений):

- `FPlatformTime::Microseconds()` → `FPlatformTime::Seconds() * 1000000`
- `FBoneReference::BoneIndex.IsValid()` → `HasValidSetup()`
- `GetLinkedAnimInstances()` (non-const) теперь приватный в UE 5.5
- `FAnimInstanceProxy::AddCurveValue()` теперь protected
- `GRHISupportsComputeShaders` удалён
- Модульные зависимости: добавлены UMG, RHI, RenderCore

### Исправления багов (108 исправлений)
- **Критические/высокие**: PredictedMove 2x double-counting, LODSettings кэш, DrivingBone валидность, Impulse primary AnimInstance, WindZone Verlet spike, Distance-LOD (Camera cache), Budget Allocator inverted logic, Bone Scaling not applied, Pose Gravity multipliers dead code, Self-Collision inverted distance condition
- **Средние**: RuntimeDebugDraw, EditMode ChildIndex bounds, RagdollBlend uninitialized rotation, Magic numbers, RemoveAt(0), Wind noise, NaN from zero axis, Substep drift, FrameCount never incremented
- **Низкие**: Teleport WorldSpace PrevLocation, Substep accumulator drift, Null SkelComp warnings, GPU fallback gravity, Log categories, deprecated arrays
- **Race conditions (3)**: Impulse queue deferred, Collision mutex, TSharedPtr FCriticalSection
- **API misuse (7)**: ComponentTransform Transient, FORCEINLINE, deprecated arrays, BlueprintType, int32 types, default values
- **Batch 5 (Claude review)**: Transfer Motion SourceBone init, DT²→DT, Camera culling GetPawn→GetViewTarget, Camera culling threshold→UPROPERTY

### 10 Новых фич

| # | Фича | UPROPERTY | Описание |
|---|------|-----------|----------|
| 1 | **Heuristic Solve Order** | `bHeuristicSolveOrder` | XPBD solver: сортировка костей от листьев к корню для быстрой сходимости |
| 2 | **Physics Sleep** | `bEnablePhysicsSleep`, `SleepVelocityThreshold`, `SleepFrameCount` | Автозаморозка симуляции при неподвижных костях (CPU экономия) |
| 3 | **Per-Chain Sim Space** | `SimulationSpaceOverride` в ChainPreset | Разные пространства для разных цепочек (волосы=World, юбка=Component) |
| 4 | **Collision Groups** | `CollisionLayer` на ноде + каждом primitive | Bitmask фильтрация: волосы=голова, юбка=ноги |
| 5 | **CCD (Edge Collision)** | `bEnableCCD`, `CCDSphereRadius` | Swept sphere между кадрами. Предотвращает tunneling |
| 6 | **Backstop Collision** | `bEnableBackstopCollision`, `BackstopRadius`, `BackstopStrength` | Per-bone hemisphere: не даёт костям провалиться в тело |
| 7 | **Camera Culling** | `bEnableCameraFrustumCulling`, `CameraCullingDistance` | Пропуск симуляции для невидимых персонажей |
| 8 | **Pose Warp Smoothing** | `bEnablePoseWarpSmoothing`, `PoseWarpAngleThreshold`, `PoseWarpSmoothingSpeed` | Сглаживает резкие смены анимации (hit reactions) |
| 9 | **2.5D Anime Mode** | `bEnable2DAnimeMode`, `AnimePlaneBlend`, `AnimePlaneNormal` | Ограничение движения плоскостью для аниме-стиля |
| 10 | **Transfer Motion** | `bEnableTransferMotion`, `TransferMotionSources[]` | Угловая скорость кости-источника → внешняя сила |

### Фокус на персонаже
Все новые физические фичи **выключены по умолчанию** — включаются по необходимости через boolean:

| Фича | Описание |
|-------|----------|
| Body Physics (HDT-SMP + UBE) | Физика тела: грудь, живот, ягодицы |
| Clothing Physics | Физика одежды с сохранением формы |
| Weapon/Accessory Physics | Пендулум для оружия и аксессуаров |
| Cloth-to-Body Collision | Столкновение одежды с телом |
| Character Physics Profile | DataAsset для настройки профиля |
| Deterministic Mode | Фиксированный timestep (выкл по умолчанию) |

---

## Установка

### Вариант 1: Git Clone
```bash
cd YourProject/Plugins/
git clone https://github.com/Far-g-Us/KawaiiPhysicsExtension.git
```

### Вариант 2: Ручная установка
1. Скачайте архив с [GitHub Releases](https://github.com/Far-g-Us/KawaiiPhysicsExtension/releases)
2. Распакуйте в `YourProject/Plugins/KawaiiPhysicsExtension/`
3. Пересоберите проект

### Требования
- **Unreal Engine 5.5** (5.4 и ниже не поддерживаются в этой версии)
- Visual Studio 2022+ или Rider
- Windows x64

---

## Структура плагина

```
KawaiiPhysicsExtension/
├── Source/
│   ├── KawaiiPhysicsExtension/          # Runtime модуль
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
│   └── KawaiiPhysicsExtensionEd/        # Editor модуль
│       ├── Public/
│       └── Private/
│           ├── AnimGraphNode_KawaiiPhysics.cpp
│           ├── KawaiiPhysicsEd.cpp
│           └── KawaiiPhysicsEditMode.cpp
├── Content/
│   └── KawaiiPhysics/                   # Базовые ассеты
├── KawaiiPhysicsExtension.uplugin
└── README.md
```

---

## Основные фичи

### Anim Graph Node
**KawaiiPhysics** — основной нод в AnimGraph. Поддерживает:
- Multiple bone chains with auto-detection
- World / BaseBone / Component simulation space + **Per-Chain Simulation Space**
- LOD-aware physics (авто-отключение на дальних LOD)
- Substepping и WarmUp для стабильности
- Deterministic Mode (accumulator-based fixed timestep)

### Physics Features
- **Self-Collision** — столкновение костей друг с другом (Spatial Hash Grid O(n·k))
- **External Collision** — столкновение с meshes, capsules, spheres
- **Bone Constraints** — XPBD constraints между костями (Heuristic Solve Order)
- **Bone Subdivision** — автоматическое добавление dummy-костей для гибкости
- **Planar Limits** — ограничения по плоскостям
- **Twist/Swing Limits** — ограничения вращения
- **CCD (Edge Collision)** — swept sphere предотвращает tunneling
- **Backstop Collision** — per-bone hemisphere предотвращает penetration в тело
- **Collision Groups** — bitmask фильтрация по слоям
- **Physics Sleep** — автозаморозка при неподвижных костях
- **2.5D Anime Mode** — ограничение движения плоскостью для аниме-стиля
- **Transfer Motion** — angular velocity кости → внешняя сила

### External Forces
Плагинная система внешних сил:
- **Basic Force** — постоянная сила (гравитация, ветер)
- **Curve Force** — анимированная сила через CurveFloat
- **Gravity** — реалистичная гравитация с настраиваемым направлением
- **Wind** — ветер с шумом и turbulence
- **Wind Zone** — зона ветра с формой (Box/Sphere/Capsule)

### Performance
- **Camera Culling** — пропуск симуляции для невидимых персонажей
- **Distance LOD** — снижение качества по расстоянию
- **Budget Allocator** — rate limiting симуляции
- **Physics Sleep** — freeze при неподвижности

### Character Focus
- **Pose Warp Smoothing** — сглаживание резких смен анимации
- **Warping Smoothing** — сглаживание телепортов актора
- **Pose Blending** — pose-based multipliers (gravity, damping, stiffness)
- **Physics Material** — per-bone настройки физики
- **Bone Scaling** — dynamic scaling костей при симуляции

### Debug & Visualization
- Runtime debug draw: bones, collision shapes, velocity arrows
- Editor-only: bone hierarchy, constraint visualization
- Blueprint API для runtime изменения параметров

---

## Blueprint API

```cpp
// Основные функции
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

## Модули

| Модуль | Тип | Описание |
|--------|-----|----------|
| `KawaiiPhysicsExtension` | Runtime | Основная физика, AnimGraph node |
| `KawaiiPhysicsExtensionEd` | UncookedOnly | Editor UI, отладка, превью |

### Зависимости модулей

**KawaiiPhysicsExtension** (Public):
- Core, AnimGraphRuntime, CoreUObject, Engine, GameplayTags, DeveloperSettings

**KawaiiPhysicsExtension** (Private):
- Slate, SlateCore, UMG, RHI, RenderCore

**KawaiiPhysicsExtensionEd** (Private):
- Core, CoreUObject, Engine, InputCore, KawaiiPhysicsExtension
- AnimGraph, BlueprintGraph, Persona, UnrealEd, AnimGraphRuntime
- Slate, SlateCore, EditorFramework, AnimationEditMode

---

## Сборка

```bash
# Windows
"E:\Epic Games\UE_5.5\Engine\Build\BatchFiles\Build.bat" ^
    YourProjectEditor Win64 Development ^
    -Project="YourProject\YourProject.uproject" ^
    -WaitMutex
```

Или через Visual Studio / Rider:
1. Откройте `YourProject.uproject`
2. Build → Build Solution (Ctrl+Shift+B)

---

## Известные ограничения

1. **GPU Simulation** — экспериментальная фича. Авто-отключается при < 50 костях. Collision и constraints всегда на CPU.
2. **GetLinkedAnimInstances()** — в UE 5.5 non-const версия приватная. Используется const_cast workaround.
3. **FAnimInstanceProxy::AddCurveValue()** — protected в UE 5.5. Используется публичный UAnimInstance::AddCurveValue() вместо этого.
4. **Log Categories** — Runtime и Editor модули используют разные лог-категории из-за ограничений экспорта символов между DLL.
5. **Camera Culling** — использует distance-based check (не точный frustum). Точный frustum check требует доступа к камере из animation thread.

---

## Планы

- [x] GPU Simulation: экспериментальная поддержка (авто-отключение при < 50 костях)
- [x] Cloth Simulation: базовая тканевая физика (shear/bend/stretch constraints)
- [ ] Cosserat Rod: twist constraint для волос
- [ ] VRM SpringBone Import Bridge
- [ ] Batched Processing: World Subsystem для 20+ персонажей
- [ ] Platform-specific LOD Profiles

---

## Спасибо

- [pafuhana1213](https://github.com/pafuhana1213) — автор оригинального KawaiiPhysics
- [Unreal Engine](https://www.unrealengine.com/) — Epic Games
- UE 5.5 API reference and community

---

## Лицензия

MIT License — см. [LICENSE](LICENSE) для подробностей.

---

*Последнее обновление: v1.31.1 — UE 5.5 Compatible + 10 New Features*
