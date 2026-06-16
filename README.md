# KawaiiPhysicsExtension

Fork of [KawaiiPhysics](https://github.com/pafuhana1213/KawaiiPhysics) for **Unreal Engine 5.5**. Extended with character-focused features: body physics (HDT-SMP + UBE), clothing, weapons, and accessories.

> **Original plugin**: [pafuhana1213/KawaiiPhysics](https://github.com/pafuhana1213/KawaiiPhysics)  
> **This fork**: [Far-g-Us/KawaiiPhysicsExtension](https://github.com/Far-g-Us/KawaiiPhysicsExtension)  
> **Version**: 1.31.1 | **UE 5.5** | **作者: A1ZZM**

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

### Исправления багов (59 исправлений)
- **10 критических/высоких**: PredictedMove 2x double-counting, LODSettings кэш, DrivingBone валидность, Impulse primary AnimInstance, WindZone Verlet spike
- **8 средних**: RuntimeDebugDraw,EditMode ChildIndex bounds, RagdollBlend uninitialized rotation
- **8 низких**: Teleport WorldSpace PrevLocation, Substep accumulator drift, Null SkelComp warnings
- **11 дополнительных (Batch 2)**: GPU fallback gravity, Bone velocity access, и другие

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

### Вариант 2:手动 копирование
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
- Multiple bone chains с自动 обнаружением
- World / BaseBone / Component simulation space
- LOD-aware physics (авто-отключение на дальних LOD)
- Substepping и WarmUp для стабильности
- Deterministic Mode (accumulator-based fixed timestep)

### External Forces
Плагинная система внешних сил:
- **Basic Force** — постоянная сила (гравитация, ветер)
- **Curve Force** — анимированная сила через CurveFloat
- **Gravity** — реалистичная гравитация с настраиваемым направлением
- **Wind** — ветер с шумом и turbulence
- **Wind Zone** — зона ветра с формой (Box/Sphere/Capsule)

### Physics Features
- **Self-Collision** — столкновение костей друг с другом
- **External Collision** — столкновение с meshes, capsules, spheres
- **Bone Constraints** — XPBD-.constraints между костями
- **Bone Subdivision** — автоматическое добавление dummy-костей для гибкости
- **Planar Limits** — ограничения по плоскостям
- **Twist/Swing Limits** — ограничения вращения
- **Windy** — глобальный ветер для всех нодов
- **Presets** — наборы настроек для разных типов физики

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

---

## Планы

- [ ] GPU Simulation: полная поддержка collision на GPU
- [ ] Cloth Simulation: полноценная тканевая физика
- [ ] Skeletal Mesh Collision: runtime добавление collision через blueprint
- [ ] Performance Profiler: встроенные метрики для бюджетирования
- [ ] Multi-platform: Android, iOS, Linux support

---

## Спасибо

- [pafuhana1213](https://github.com/pafuhana1213) — автор оригинального KawaiiPhysics
- [Unreal Engine](https://www.unrealengine.com/) — Epic Games
- UE 5.5 API reference и社區

---

## Лицензия

MIT License — см. [LICENSE](LICENSE) для подробностей.

---

*Последнее обновление: v1.31.1 — UE 5.5 Compatible*
