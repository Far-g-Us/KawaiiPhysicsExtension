# KawaiiPhysics — Полный мануал для разработчиков и ИИ
> UE 5.3–5.7 · MIT License · v1.20.0 (январь 2026)

---

## 📌 Что такое KawaiiPhysics

KawaiiPhysics — это плагин для Unreal Engine, реализующий **псевдо-физику вторичного движения** (secondary motion / soft body) для костей скелета без использования PhysX. Он работает как **AnimGraph Node** внутри Animation Blueprint и обеспечивает движение волос, юбок, аксессуаров, хвостов и любых других "болтающихся" объектов.

**Ключевые преимущества:**
- Не зависит от PhysX/Chaos — собственный лёгкий алгоритм
- Скелет никогда не "разваливается" — встроенная защита от нестабильности
- Один нод в AnimGraph охватывает целую цепочку костей
- DataAsset для переиспользования настроек между персонажами
- Поддержка ветра (WindDirectionalSource), гравитации и кастомных внешних сил
- Экспериментальная система Bone Constraints (для юбок, сеток)

---

## 🗂️ Структура репозитория

```
KawaiiPhysics/
├── Plugins/KawaiiPhysics/
│   ├── Source/KawaiiPhysics/
│   │   ├── Public/
│   │   │   ├── AnimNode_KawaiiPhysics.h        ← Главный нод
│   │   │   ├── KawaiiPhysicsLibrary.h          ← Blueprint Function Library
│   │   │   ├── KawaiiPhysicsExternalForce.h    ← Базовый класс внешних сил
│   │   │   └── KawaiiPhysicsLimitsDataAsset.h  ← DataAsset для коллизий
│   │   └── Private/
│   │       └── AnimNode_KawaiiPhysics.cpp      ← Вся физическая логика
│   └── KawaiiPhysics.uplugin
├── Content/KawaiiPhysicsSample/               ← Сэмплы
└── KawaiiPhysicsSample.uproject
```

**Главный файл для изменений:** `AnimNode_KawaiiPhysics.cpp` — здесь происходит симуляция. Метод `EvaluateSkeletalControl_AnyThread` — точка входа в физику.

---

## ⚙️ Все параметры узла (с пояснениями)

### БЛОК: Modify Target (Цель)

| Параметр | Тип | Описание |
|---|---|---|
| `Root Bone` | FBoneReference | Корневая кость цепочки. Все дочерние кости автоматически захватываются. |
| `Exclude Bones` | TArray\<FBoneReference\> | Кости, которые нужно **исключить** из симуляции (и их потомков). |
| `Dummy Bone Length` | float | Длина фиктивной кости на конце цепи. Улучшает вращение последней кости. 0 = выкл. |
| `Bone Forward Axis` | EAxis | Ось, считающаяся "вперёд" для кости. Влияет на расположение dummy bone и ориентацию физики. |

**Рекомендации:**
- Всегда задавай `Dummy Bone Length > 0` для концевых костей — иначе последняя кость ведёт себя неестественно.
- Используй `Exclude Bones` для пальцев или костей, которыми управляет IK.

---

### БЛОК: Physics Settings (Физика)

| Параметр | Диапазон | Описание |
|---|---|---|
| `Damping` | 0.0–1.0 | Затухание колебаний. Чем **меньше** — тем дольше раскачивается, тем более "жидкое" поведение. |
| `Stiffness` | 0.0–1.0 | Жёсткость. Чем **больше** — тем сильнее тянет к исходной позе. |
| `World Damping Location` | 0.0–1.0 | Влияние **перемещения** меша в мировых координатах на кости. |
| `World Damping Rotation` | 0.0–1.0 | Влияние **вращения** меша на кости. |
| `Collision Radius` | float | Радиус сферы коллизии каждой кости. |
| `Limit Angle` | 0–180° | Максимальный угол отклонения кости от исходного положения. Предотвращает "бешенство". |
| `Target Frame Rate` | int | Целевой FPS для симуляции. Стабилизирует поведение при разных FPS. |
| `Warm Up Frames` | int | Число "холостых" шагов физики при старте. Полезно чтобы избежать дёргания при появлении. |
| `Teleport Distance Threshold` | float | Если меш переместился на больше этого значения за кадр — перемещение **игнорируется** физикой (телепорт). |
| `Teleport Rotation Threshold` | float | Аналогично для вращения. |
| `Planar Constraint` | FVector | Ограничить движение костей плоскостью. Полезно для юбок. |
| `Update Physics Settings In Game` | bool | Обновлять параметры каждый кадр. Выкл. = небольшой прирост производительности. |
| `Reset Bone Transform When Bone Not Found` | bool | Сбросить трансформ кости если она не найдена. Обычно должно быть **выкл.** |

#### Curve-параметры (коррекция по длине цепочки)

Каждый из основных параметров имеет версию `*_Curve_Data`. Ось X кривой — нормализованная длина от корневой до концевой кости (0.0 = корень, 1.0 = конец). Значение кривой **умножается** на базовый параметр.

Пример: `Stiffness = 0.5`, кривая в точке 0.0 = 1.0, в точке 1.0 = 0.1 → корневые кости жёсткие, концевые — мягкие.

| Curve | Применение |
|---|---|
| `Damping Curve Data` | Разное затухание вдоль цепочки |
| `Stiffness Curve Data` | Разная жёсткость (корень жёсткий, конец мягкий) |
| `World Damping Location Curve Data` | Избирательная реакция на движение |
| `World Damping Rotation Curve Data` | Избирательная реакция на вращение |
| `Radius Curve Data` | Переменный радиус коллизии |
| `Limit Angle Curve Data` | Разные углы ограничения |

---

### БЛОК: Limits / Collision (Коллизии)

KawaiiPhysics использует собственные примитивы коллизии, **не связанные** с Chaos/PhysicsAsset персонажа.

| Примитив | Параметры | Использование |
|---|---|---|
| `Spherical Collision` | Bone, Offset, Radius, Limit Type (Inner/Outer) | Грудь, плечи, голова |
| `Capsule Collision` | Bone, Offset, Radius, Length, Limit Type | Торс, руки, ноги |
| `Planar Collision` | Bone, Offset, Rotation, Limit Type | Пол, стол (не дать прорваться сквозь) |

**Limit Type:**
- `Outer` — кости не могут **войти** внутрь примитива (стандартный отпружинивающий коллайдер)
- `Inner` — кости остаются **внутри** примитива (клетка / ограничивающий объём)

**DataAsset версия:**
Используй `Limits Data Asset` вместо встроенного массива, если хочешь переиспользовать коллизии между разными ABP или персонажами. Создаётся как `KawaiiPhysicsLimitsDataAsset`.

---

### БЛОК: Bone Constraint (Экспериментально)

Система ограничений расстояния между парами костей. Аналог XPBD-constrained simulation.

| Параметр | Описание |
|---|---|
| `Global Compliance Type` | Тип жёсткости: Hard (абсолютное), Soft (пружинное), None |
| `Constraint Iteration Count (Before Collision)` | Итерации до обработки коллизий |
| `Constraint Iteration Count (After Collision)` | Итерации после обработки коллизий |
| `Auto Add Child Dummy Bone Constraint` | Автоматически добавлять dummy bone в список ограничений |
| `Bone Constraints` | Пары костей с ограничением расстояния |
| `Bone Constraints Data Asset` | DataAsset версия (рекомендуется) |

**Применение:** Юбки — задаёшь пары "левый борт / правый борт" чтобы ткань не складывалась насквозь. Более 2–4 итераций заметно влияют на производительность.

---

### БЛОК: External Force (Внешние силы)

| Параметр | Описание |
|---|---|
| `Gravity` | Гравитация в единицах UE (не WorldGravity). Vector. |
| `Enable Wind` | Подключить WindDirectionalSource из уровня |
| `Wind Scale` | Множитель силы ветра |
| `External Forces` | Встроенные пресеты (Basic, Gravity, Curve). Расширяются через C++. |
| `Custom External Forces` | ЭКСПЕРИМЕНТАЛЬНО. Пресеты через BP/C++ (Instanced Property). Требует клик по ноду или компиляцию ABP. |

**Встроенные типы External Force:**

- `Basic` — постоянная сила в заданном направлении (ветер, постоянное ускорение)
- `Gravity` — физически корректная гравитация с поддержкой `CustomGravityDirection` персонажа
- `Curve` — сила, изменяющаяся по кривой во времени (порывы ветра, вибрация)

**Общие параметры всех External Force:**

| Параметр | Описание |
|---|---|
| `Apply Bone Filter` | Применить силу только к этим костям |
| `Ignore Bone Filter` | Не применять к этим костям |
| `External Force Space` | World / Local / Component |
| `Random Force Scale` | Диапазон случайного масштаба силы |
| `Force Rate By Bone Length Rate` | Кривая: изменить силу в зависимости от позиции в цепочке |

---

### БЛОК: World Collision (Коллизии с уровнем)

| Параметр | Описание |
|---|---|
| `Allow World Collision` | Включить проверку коллизий с объектами уровня. **Дорого!** |
| `Override Collision Params` | Использовать кастомные collision channel вместо дефолтных |
| `Ignore Self Component` | Игнорировать PhysicsAsset самого персонажа |
| `Ignore Bones` | Список костей PhysicsAsset, которые игнорировать |
| `Ignore Bone Name Prefix` | Игнорировать кости по префиксу имени |

**Предупреждение:** `Allow World Collision` добавляет sphere sweeps для каждой кости каждый кадр. На персонажах с длинными волосами (20+ костей) это заметно влияет на CPU.

---

## 🧩 Архитектура кода

### Главная структура данных: `FKawaiiPhysicsBone`

```cpp
struct FKawaiiPhysicsBone
{
    FBoneReference BoneRef;
    FTransform PoseTransform;          // Анимационная поза
    FTransform SimulatedTransform;     // Результат симуляции
    FVector Location;                  // Текущая позиция
    FVector PrevLocation;              // Позиция прошлого кадра (Verlet)
    float LengthFromRoot;              // Расстояние от корня
    float PhysicsSettings_Damping;    // Применённые параметры
    float PhysicsSettings_Stiffness;
    // ... и остальные параметры
};
```

### Алгоритм симуляции (Verlet Integration)

Каждый кадр в `EvaluateSkeletalControl_AnyThread`:

1. **Collect Bones** — собрать все кости от Root вниз по иерархии
2. **Update Pose** — применить анимационную позу как "целевую"
3. **Simulate**:
   - Интегрирование Верле: `NewPos = Pos + (Pos - PrevPos) * (1 - Damping) + Force * DeltaTime²`
   - Приближение к анимационной позе через Stiffness
   - Применение WorldDamping (убрать влияние движения актора)
4. **Constraints** — применить Bone Constraints (XPBD-подобные итерации)
5. **Collision** — проверить и разрешить коллизии с примитивами
6. **World Collision** — если включено, sweep против мира
7. **Apply** — записать результат в OutBoneTransforms

---

## 🚀 Идеи по расширению (с техническими деталями)

### 1. Многоосевые ограничения (Twist / Swing Limit)

**Что сейчас:** `Limit Angle` — единое скалярное ограничение угла.

**Что улучшить:** Раздельные ограничения по осям: `TwistLimitAngle`, `SwingLimitAngle1`, `SwingLimitAngle2`. Аналогично Twist/Swing в Physics Constraint UE.

**Как реализовать:**
```cpp
// В FKawaiiPhysicsSettings добавить:
UPROPERTY(EditAnywhere, BlueprintReadWrite)
float TwistLimitAngle = 30.f;
UPROPERTY(EditAnywhere, BlueprintReadWrite)
float SwingLimitYAngle = 45.f;
UPROPERTY(EditAnywhere, BlueprintReadWrite)
float SwingLimitZAngle = 45.f;

// В Simulate:
FQuat CurrentRot = SimulatedTransform.GetRotation();
FQuat ParentRot = ParentBone.SimulatedTransform.GetRotation();
FQuat LocalRot = ParentRot.Inverse() * CurrentRot;
// Clamp LocalRot по каждой оси отдельно
```

---

### 2. Поддержка Control Rig / Full Body IK

**Что сейчас:** KawaiiPhysics работает **после** стандартной анимации как пост-процесс нод.

**Что улучшить:** Интеграция как `ControlRig Node` в AnimGraph, чтобы физика могла влиять на IK-цели или получать их как входные данные.

**Как реализовать:**
- Создать `UControlRigComponent`-совместимый интерфейс
- Добавить входной пин `IK Target Override` — Vector array, перекрывающий позицию конечных костей
- Альтернатива: экспортировать результат симуляции в `Curve` для использования в других нодах

---

### 3. Mesh-based Collision (треугольная коллизия)

**Что сейчас:** Только Sphere, Capsule, Plane.

**Что улучшить:** Добавить `Convex Hull Collision` или `Triangle Mesh Collision` из StaticMesh/SkeletalMesh.

**Как реализовать:**
```cpp
// Новый тип лимита:
USTRUCT(BlueprintType)
struct FKawaiiPhysicsConvexLimit
{
    UPROPERTY() UStaticMesh* CollisionMesh;
    UPROPERTY() FTransform Offset;
    // При симуляции: GJK/EPA против convex hull из меша
};
```

Использовать `Chaos::FConvex` или `FKDopTree` для эффективного поиска.

---

### 4. LOD-aware Physics (уровни детализации)

**Что сейчас:** Физика работает на всех LOD одинаково.

**Что улучшить:** Снижать число итераций, отключать World Collision и Bone Constraints на дальних LOD.

**Как реализовать:**
```cpp
// В AnimNode добавить:
UPROPERTY(EditAnywhere, Category="LOD")
TMap<int32, FKawaiiPhysicsLODSettings> LODSettings;

// В EvaluateSkeletalControl_AnyThread:
int32 CurrentLOD = SkelComp->GetPredictedLODLevel();
FKawaiiPhysicsLODSettings& Settings = LODSettings.FindOrAdd(CurrentLOD);
// Применить облегчённые настройки
```

---

### 5. Blueprint-доступный Callback на столкновение

**Что сейчас:** Нет события при коллизии кости с примитивом.

**Что улучшить:** `OnBoneCollision` Delegate в Blueprint.

**Как реализовать:**
```cpp
DECLARE_DYNAMIC_MULTICAST_DELEGATE_ThreeParams(
    FOnKawaiiPhysicsBoneCollision,
    FName, BoneName,
    FVector, CollisionPoint,
    FVector, CollisionNormal
);

UPROPERTY(BlueprintAssignable, Category="KawaiiPhysics|Events")
FOnKawaiiPhysicsBoneCollision OnBoneCollision;
```

Вызывать в `ResolveCollision` при обнаружении пересечения.

---

### 6. Runtime Parameter Control через Blueprint

**Что сейчас:** Параметры задаются в редакторе, изменить в рантайме можно только через `Update Physics Settings In Game = true`.

**Что улучшить:** Blueprint-функции для прямого изменения параметров по имени кости.

**Как реализовать:**
```cpp
// В KawaiiPhysicsLibrary:
UFUNCTION(BlueprintCallable, Category="KawaiiPhysics")
static void SetBoneStiffness(
    USkeletalMeshComponent* SkelComp,
    FName BoneName,
    float Stiffness
);

UFUNCTION(BlueprintCallable, Category="KawaiiPhysics")
static void SetPhysicsEnabled(
    USkeletalMeshComponent* SkelComp,
    bool bEnabled
);
```

---

### 7. Предиктивная физика (Look-Ahead Integration)

**Что сейчас:** Верле-интеграция реагирует на движение после факта.

**Что улучшить:** Предсказывать движение на 1–2 кадра вперёд по вектору velocity актора, уменьшая задержку реакции.

**Как реализовать:**
```cpp
// В начале симуляции:
FVector PredictedActorLocation = ComponentTransform.GetLocation() 
    + ComponentVelocity * DeltaTime * PredictionScale;
// Использовать PredictedActorLocation для расчёта WorldDamping
```

---

### 8. Cloth-подобная сеть костей (Grid Constraints)

**Что сейчас:** Bone Constraints поддерживает произвольные пары, но без автоматической сетки.

**Что улучшить:** Утилита/редактор для автоматического построения сетки ограничений для юбок (4-connected или 8-connected grid).

**Как реализовать:**
- Editor Tool Button в деталях нода
- Анализировать иерархию костей юбки (spine_01 → skirt_L_01, skirt_R_01...)
- Автоматически добавлять горизонтальные ограничения между соседними "полосами"

---

### 9. Physics State Serialization (Сохранение состояния)

**Что сейчас:** При появлении актора физика начинается с нуля (Warm Up Frames помогает, но ограниченно).

**Что улучшить:** Сохранять и восстанавливать `FVector Location` / `FVector PrevLocation` для каждой кости.

**Применение:** Persistent characters, save-game системы, multiplayer (синхронизация вторичного движения через RPC).

---

### 10. Multi-threaded Simulation (параллельная симуляция)

**Что сейчас:** Симуляция проходит в `AnyThread`, но цепочки костей обрабатываются последовательно.

**Что улучшить:** При наличии нескольких независимых цепочек (волосы + юбка + хвост в одном ABP) — параллельная обработка через `ParallelFor`.

**Ограничения:** Нельзя параллелить, если цепочки имеют общие кости (Exclude Bones не решает). Нужен граф зависимостей.

---

## 🛠️ Настройка под UE 5.5 — что проверить

1. **Chaos Physics** — убедиться что `Allow World Collision` не конфликтует с Chaos collision layers. Использовать `Override Collision Params` с правильным каналом.

2. **Animation Optimization** — в UE 5.5 есть `Animation Budget Allocator`. KawaiiPhysics не интегрирован с ним по умолчанию. Для оптимизации добавить поддержку `FAnimInstanceProxy::GetSkelMeshComponent()->GetAnimationMode()`.

3. **Nanite / Virtual Shadow Maps** — не влияют на KawaiiPhysics напрямую, но World Collision дороже на сценах с Nanite из-за complexity коллизионных мешей.

4. **Control Rig Pipeline** — если используешь Control Rig, ставь KawaiiPhysics **после** CR нода в AnimGraph (Post-process позиция).

5. **Network / Replication** — KawaiiPhysics **не реплицируется** по сети. Это нормально для вторичной анимации, но помни об этом при разработке мультиплеера.

---

## 📋 Быстрый старт (шаги)

1. Скопировать папку `Plugins/KawaiiPhysics` в свой проект
2. Открыть Animation Blueprint нужного персонажа
3. В AnimGraph добавить нод `KawaiiPhysics` после нода анимации
4. В деталях нода указать `Root Bone` (например `hair_root` или `skirt_root`)
5. Установить `Dummy Bone Length = 5.0` (или длину последнего сегмента)
6. Настроить `Damping = 0.6`, `Stiffness = 0.05` как начальные значения
7. Добавить `Spherical Collision` для торса персонажа чтобы волосы не проваливались
8. Сохранить DataAsset с коллизиями через `Limits Data Asset` для переиспользования

---

## 🔗 Полезные ссылки

- GitHub: https://github.com/pafuhana1213/KawaiiPhysics
- Wiki (параметры): https://github.com/pafuhana1213/KawaiiPhysics/wiki
- DeepWiki (AI-документация): https://deepwiki.com/pafuhana1213/KawaiiPhysics
- FAB (готовый плагин): https://www.fab.com/listings/f870c07e-0a02-4a78-a888-e52a22794572
- Туториал на Epic Dev: https://dev.epicgames.com/community/learning/tutorials/d1Z9/unreal-engine-how-to-setup-kawaii-physics-in-unreal
