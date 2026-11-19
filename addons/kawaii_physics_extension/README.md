# KawaiiPhysics — пружинная физика костей для Skeleton3D

Godot 4.7 аддон: `SkeletonModifier3D`-нода, имитирующая вторичную физику
(хвосты, волосы, ткани, украшения) порогом **MVP-порта** плагина
[KawaiiPhysicsExtension](https://github.com/Far-g-Us/KawaiiPhysicsExtension)
(UE5, MIT). Ядро симуляции — `scripts/kawaii_sim_core.gd` (чистая статика),
нода — `scripts/kawaii_physics.gd`.

## Установка и использование

1. Project Settings → Plugins → **KawaiiPhysics** → Enable. (Включение
   плагина не обязательно для работы ноды — класс зарегистрирован через
   `class_name`; плагин нужен для отображения аддона в списке.)
2. Добавить `KawaiiPhysics` как ребёнка `Skeleton3D`.
3. Задать `root_bone` — кость, от которой вниз по иерархии строится симулируемая цепь.

Смена `root_bone`, `exclude_bones`, `dummy_bone_length` в рантайме
пересобирает цепь на следующем шаге (`root_bone`, `additional_root_bones`, `exclude_bones`, `dummy_bone_length`, `bone_subdivision_count`).

## Параметры ноды

| Параметр | Тип | По умолчанию | Описание |
|---|---|---|---|
| `simulation_space` | enum | `COMPONENT` | `COMPONENT` / `WORLD` / `BASE_NODE` |
| `ragdoll_blend` | float | 0.0 | Блендинг позы к ragdoll-источнику (0–1) |
| `ragdoll_source` | NodePath | &"" | `Skeleton3D`-источник ragdoll-позы (например, скелет под `PhysicalBoneSimulator3D`) |
| `base_node` | NodePath | &"" | Внешний узел-база для `BASE_NODE` (относительно модификатора) |
| `root_bone` | StringName | `""` | Корень цепи; вниз собираются все дочерние кости |
| `exclude_bones` | Array[StringName] | `[]` | Кость и всё её поддерево исключаются из цепи |
| `settings` | KawaiiSettings | — | damping/stiffness/world-damping/collision_radius/limit_angle_deg + кривые stiffness/damping |
| `preset` | KawaiiPreset | null | Пресет: settings + сабдивижн + коллизии + гравитация; применяется в `_ready` (или `apply_preset()`) |
| `additional_root_bones` | Array[StringName] | `[]` | Дополнительные корни (независимые цепочки в одном модификаторе) |
| `influence` | float 0–1 | 1.0 | Смешивание результата с позой (унаследовано от SkeletonModifier3D) |
| `bone_subdivision_count` | int | 0 | Dummy-кости между реальными костями (0–10): плавнее изгибы и коллизии |
| `bone_subdivision_collision_only` | bool | false | Dummy не интегрируются, стоят на линии концов и передают им выталкивание |
| `bone_constraints` | Array[KawaiiBoneConstraint] | `[]` | Горизонтальные связи между костями разных цепочек (кость A, кость B, stiffness) |
| `constraint_leaf_first` | bool | true | Решать констрейнты от листьев к корню (heuristic order) |
| `sync_bones` | Array[KawaiiSyncBone] | `[]` | Кость-приёмник получает глобальную позу симулированной кости (+offset) |
| `dummy_bone_length` | float | 0.0 | > 0 добавляет фиктивный кончик к листовым костям (влияет на поворот родителя) |
| `target_framerate` | int 1–240 | 60 | Референсная частота для масштаба stiffness |
| `gravity` | Vector3 | (0, −9.8, 0) | В **метрах**, переводится в пространство скелета |
| `constant_force` | Vector3 | 0 | Постоянная сила (ветер и т.п.), складывается с гравитацией |
| `teleport_distance_threshold` | float | 3.0 | Сдвиг скелета дальше → полный reset к позе |
| `teleport_rotation_threshold_deg` | float | 10.0 | Поворот скелета больше → reset к позе |
| `warmup_frames` | int | 0 | Прогрев: N шагов симуляции перед первым кадровым шагом |
| `sphere_limits` / `capsule_limits` / `plane_limits` | Array | `[]` | Ресурсы лимитов (ниже) |
| `world_collision_enabled` | bool | false | Коллизии костей с физикой мира (лучи родитель→кость) |
| `world_collision_mask` | int (3D physics flags) | 1 | Слои, с которыми сталкиваются кости |
| `world_collision_margin` | float | 0.005 | Отступ от поверхности при выталкивании |
| `world_collision_radius_scale` | float | 1.0 | Множитель радиуса кости для коллизии |
| `self_collision_enabled` | bool | false | Раздвижение близких костей цепи |
| `self_collision_min_distance` | float | 0.01 | Минимальная дистанция пары |
| `self_collision_radius_multiplier` | float | 1.0 | Множитель min_distance |
| `self_collision_push_split_ratio` | float | 0.5 | Доля раздвижения на каждую кость пары |
| `sleep_enabled` | bool | false | Авто-засыпание при низкой подвижности |
| `sleep_motion_threshold` | float | 0.002 | Порог движения кости за шаг для сна |
| `sleep_frames` | int | 5 | Сколько шагов ниже порога до сна |
| `editor_preview` | bool | false | @tool-превью симуляции в редакторе (обновляйте с `active=false`) |
| `debug_draw` | bool | false | Отладочная отрисовка: линии, кольца, скорости (см. ниже) |

'## Сабдивижн горизонтальных связей

`KawaiiBoneConstraint`: `subdivision_count` (0–10) — число сферических проб вдоль линии между костями; `feedback_scale` (0–2) — сила переноса выталкивания на endpoint-кости. Коллайдер, проходящий между соседними полосами ткани, раздвигает реальные кости. Работает при `world_collision_enabled`.

## Ресурс KawaiiBoxLimit

`driving_bone`, `offset_location`, `offset_rotation_deg`, `size` (полные габариты), `enabled`: кости выталкиваются наружу бокса (после restore длин, до world collision) по оси минимального проникновения с учётом `collision_radius`.

## Ресурс KawaiiSyncBone

`source_bone` (симулированная кость), `target_bone` (кость-приёмник, может быть вне цепи), `offset` (Transform3D), `enabled`. После write-back target получает глобальную позу source с offset; target добавляется в список override и снимается при следующем шаге. Полезно для аксессуаров/отделённых частей.

## Импульсы (hit reaction)

`add_impulse(impulse: Vector3, bone: StringName = &"", falloff := true)` — одноразовый толчок: `impulse` это **приращение скорости** (м/с), применяется через `prev_loc` (Verlet). Без `bone` — по всем реальным костям с затуханием к кончику (`falloff`), с `bone` — ровно в указанную кость. Будит спящую цепь. Удобно для реакции на удар и порывов.

## 2D/3D: planar_mode

Для 2D/билборд-мобов: `planar_mode = true` + `plane_normal` (мировая нормаль плоскости, обычно (0,0,1) для сайдскроллера). После всех коллизий смещения костей проецируются на плоскость с сохранением длин звеньев — цепь колышется только в кадре, как 2D-физика. Для 3D-мобов режим выключен (полный 3D). Режим задаётся **на модификатор**, т.е. на цепь/моба.

## Коллизия с телом (KawaiiMeshCollision)

`target_path` (относительно модификатора, `NodePath`), `bone_names` (пусто = все кости), `radius`, `enabled`. Из target-скелета берутся мировые позиции костей как сферы; кости цепи выталкиваются наружу (радиус сферы + `collision_radius`). Нужно для 3D-моба: своё тело под одеждой/волосами не попадает в world collision (предки исключены), а этот ресурс его возвращает.

## Пресеты (KawaiiPreset)

`settings` (копируется при применении), `bone_subdivision_count`, `bone_subdivision_collision_only`, `world_collision_enabled`, `self_collision_enabled`, `gravity`, `constant_force`. Назначь в `KawaiiPhysics.preset` — применится в `_ready`, или вызови `apply_preset(p)`. Удобно для тел/волос/одежды.

## Внешние силы (KawaiiExternalForce)

База `KawaiiExternalForce`: `enabled`, `sample(time, world_pos) -> Vector3` (мир). Наследники:
- `KawaiiCurveForce` — `direction`, `strength`, `curve` (0–1), `time_scale`, `phase`: сила по кривой во времени (анимированные порывы);
- `KawaiiConstantForce` — `force` (мировые единицы).
Список `external_forces` сэмплируется **по-костно** вместе с ветром/зонами; не зависит от `wind_scale`.

## Кривые в KawaiiSettings

`stiffness_curve` и `damping_curve` (Curve, опционально) — множители по длине цепи: `t = индекс кости / (n-1)` (0 — корень, 1 — кончик). Позволяют сделать «мягкий кончик, жёсткий корень» без отдельных модификаторов. Без кривых параметры равномерные.

## Ресурс KawaiiWind

`enabled`, `direction` (мировой), `strength`, `turbulence` (0–1), `noise_scale`, `noise_speed`, `noise_seed` (детерминированный FastNoiseLite). `sample(time, world_pos)` отдаёт силу в мировом пространстве; модификатор добавляет её к `gravity + constant_force` с множителем `wind_scale`. Порывы = шум по времени/позиции; сила ветра равномерна по цепи (по-костный градиент — вне MVP).

## Сабдивижн горизонтальных связей

`KawaiiBoneConstraint`: `subdivision_count` (0–10) — число сферических проб вдоль линии между костями; `feedback_scale` (0–2) — сила переноса выталкивания на endpoint-кости. Коллайдер, проходящий между соседними полосами ткани, раздвигает реальные кости. Работает при `world_collision_enabled`.

## Ресурс KawaiiBoxLimit

`driving_bone`, `offset_location`, `offset_rotation_deg`, `size` (полные габариты), `enabled`: кости выталкиваются наружу бокса (после restore длин, до world collision) по оси минимального проникновения с учётом `collision_radius`.

## Ресурс KawaiiSyncBone

`source_bone` (симулированная кость), `target_bone` (кость-приёмник, может быть вне цепи), `offset` (Transform3D), `enabled`. После write-back target получает глобальную позу source с offset; target добавляется в список override и снимается при следующем шаге. Полезно для аксессуаров/отделённых частей.

## Импульсы (hit reaction)

`add_impulse(impulse: Vector3, bone: StringName = &"", falloff := true)` — одноразовый толчок: `impulse` это **приращение скорости** (м/с), применяется через `prev_loc` (Verlet). Без `bone` — по всем реальным костям с затуханием к кончику (`falloff`), с `bone` — ровно в указанную кость. Будит спящую цепь. Удобно для реакции на удар и порывов.

## 2D/3D: planar_mode

Для 2D/билборд-мобов: `planar_mode = true` + `plane_normal` (мировая нормаль плоскости, обычно (0,0,1) для сайдскроллера). После всех коллизий смещения костей проецируются на плоскость с сохранением длин звеньев — цепь колышется только в кадре, как 2D-физика. Для 3D-мобов режим выключен (полный 3D). Режим задаётся **на модификатор**, т.е. на цепь/моба.

## Коллизия с телом (KawaiiMeshCollision)

`target_path` (относительно модификатора, `NodePath`), `bone_names` (пусто = все кости), `radius`, `enabled`. Из target-скелета берутся мировые позиции костей как сферы; кости цепи выталкиваются наружу (радиус сферы + `collision_radius`). Нужно для 3D-моба: своё тело под одеждой/волосами не попадает в world collision (предки исключены), а этот ресурс его возвращает.

## Пресеты (KawaiiPreset)

`settings` (копируется при применении), `bone_subdivision_count`, `bone_subdivision_collision_only`, `world_collision_enabled`, `self_collision_enabled`, `gravity`, `constant_force`. Назначь в `KawaiiPhysics.preset` — применится в `_ready`, или вызови `apply_preset(p)`. Удобно для тел/волос/одежды.

## Внешние силы (KawaiiExternalForce)

База `KawaiiExternalForce`: `enabled`, `sample(time, world_pos) -> Vector3` (мир). Наследники:
- `KawaiiCurveForce` — `direction`, `strength`, `curve` (0–1), `time_scale`, `phase`: сила по кривой во времени (анимированные порывы);
- `KawaiiConstantForce` — `force` (мировые единицы).
Список `external_forces` сэмплируется **по-костно** вместе с ветром/зонами; не зависит от `wind_scale`.

## Кривые в KawaiiSettings

`stiffness_curve` и `damping_curve` (Curve, опционально) — множители по длине цепи: `t = индекс кости / (n-1)` (0 — корень, 1 — кончик). Позволяют сделать «мягкий кончик, жёсткий корень» без отдельных модификаторов. Без кривых параметры равномерные.

## Ресурс KawaiiWindZone

Наследует `KawaiiWind`: `shape` (BOX/SPHERE), `center` (мировые координаты), `size`, `radius`, `falloff` (0–1, плавное затухание к границе). `sample()` возвращает 0 вне объёма. Ветер сэмплируется **для каждой кости отдельно** в её мировой позиции (по-костный), глобальный `wind` тоже по-костный; `constant_force`/`gravity` остаются общими.

## Ресурс KawaiiBoneConstraint'

`bone_a`, `bone_b` — имена костей (обычно концы соседних цепочек), `stiffness` 0–1, `enabled`. В шаге расстояние между костями подтягивается к анимированному (`pose`) расстоянию; порядок: restore длин → констрейнты → повторный restore → world collision.

## Ресурсы лимитов

Каждый лимит привязан к **driving_bone** — его текущая глобальная поза задаёт
позицию/ориентацию коллайдера, плюс локальный offset:

- `KawaiiSphereLimit`: `radius`, `limit_type` (`OUTER` — кость держится **снаружи** сферы, сфера-препятствие; `INNER` — кость держится **внутри** сферы, сфера-оболочка; семантика UE: `ESphericalLimitType`).
- `KawaiiCapsuleLimit`: `radius`, `length` (капсула вдоль локальной оси Z поворота).
- `KawaiiPlaneLimit`: план по нормали `rotation × UP`, кость выталкивается на положительную сторону (+радиус).

Общие поля: `driving_bone`, `offset_location`, `offset_rotation_deg` (эйлер,
в градусах), `enabled`. Ресурсы с чужим скриптом, без driving_bone или
выключенные пропускаются. **Лимиты применяются ко всем костям цепи, включая
корень** — как в UE (флаг `bSkipSimulate` не портирован).

Порядок шага (как в UE Simulation.cpp): simulate → коллизии лимитов →
self-collision → угловые лимиты (`settings.limit_angle_deg`) → restore длин →
bone constraints → restore длин → world collision → write-back → debug draw.

## Debug draw

Цепь — белые линии (поза→текущая и между костями), кольца костей (3 оси,
16 сегментов) — cyan, сферы — yellow, капсулы — orange + линия оси, планы —
red-квадрат ±0.1. Рисуется в модельном пространстве скелета
(`MeshInstance3D`-ребёнок модификатора, unshaded/no-depth/vertex-color).

## World collision

- Луч от ближайшего реального предка к кости; выталкивание по нормали (`radius + margin`); предки-`CollisionObject3D` исключаются автоматически.
- `world_collision_ccd`: swept-sphere от `prev_loc` к `loc` — ловит тонкие стены между кадрами.
- `world_collision_exclude_group`: коллайдеры (и их дети) из этой группы игнорируются — «своё тело»/свои цепи.
- Backstop: `KawaiiSphereLimit` с `limit_type = OUTER` на кости тела — кости цепи не проваливаются в тело (готовый рецепт «задней стенки»).

Включается `world_collision_enabled = true`. Для каждой не-корневой кости пускается луч из позиции родителя в позицию кости; при попадании кость ставится в `hit.position + hit.normal * (radius + margin)`. Радиус — из `KawaiiSettings.collision_radius`, множитель — `world_collision_radius_scale`. Предки-`CollisionObject3D` (например, капсула персонажа) автоматически исключаются. Применяется **после** восстановления длин костей, чтобы выталкивание не отменялось.

Ограничения: лучи дешёвые, но не ловят тонкую геометрию между родителем и костью; кости, чей родитель уже внутри препятствия, не корректируются (луч стартует внутри). Для статики (стены, пол, ящики) этого достаточно.

## Отличия от UE-версии

- Гравитация фиксированная exported-Vector3 (0, −9.8, 0) в метрах; нет
  привязки к ProjectSettings. Масштаб сцены — метры.
- Self-collision: дистанция = `min_distance × multiplier` (не радиусы костей),
  раздвижение `PushAmount = (MinDist − Dist) × PushSplitRatio`, симметричный
  пуш, O(n²) без spatial hash (цепи < 50 костей).
- Guard на вырожденные случаи — `EPS² = 1e-12` вместо UE
  `KINDA_SMALL_NUMBER`; документированное расхождение на нулевых векторах.
- Резолв driving_bone лимитов — из **чистой позы** (композиция локальных поз
  вверх по иерархии), а не `get_bone_global_pose`: полное чтение возвращает
  override прошлых шагов симуляции и даёт петлю обратной связи.
- Лимиты с нулевым радиусом/длиной скипаются (сфера: `radius <= 0`; капсула:
  `radius <= 0` или `length <= 0`) — как в UE (`!bEnable || Radius <= 0`).
- Write-back пишет `Transform3D(Basis(rot), loc)` — **масштаб кости не
  сохраняется** (в UE то же ограничение; squash/stretch-анимации масштаб
  потеряют).
- Снятие override — per-bone: нода помнит индексы костей, которым писала
  (`_written_bones`), и снимает только их (`set_bone_global_pose_override`
  с amount 0) на пустой цепи, пустом root_bone, **пересборке цепи** (кости,
  покинувшие цепь при runtime-смене root/exclude/dummy_length, очищаются
  до перестроения), смене скелета, exit_tree и reset. Чужие override на
  костях вне цепи переживают шаги `KawaiiPhysics`. Конфликт двух
  модификаторов на **одной** кости неразрешим (один слот override,
  last-writer-wins).
- Пороги телепорта (`teleport_distance_threshold`,
  `teleport_rotation_threshold_deg`) меряются по смещению/повороту скелета в
  **модельном пространстве** (как UE ComponentSpace): масштаб скелета влияет
  на срабатывание.
- Угловые лимиты и restore длин — **два прохода** (сначала все углы, потом
  все длины); в UE — один per-bone цикл (угол кости → restore той же кости).
  Лёгкое расхождение результата на длинных цепях.
- Self-collision скипает dummy-кости — как в UE (там dummy тоже исключены
  из self-пар). Коллизионные лимиты (сфера/капсула/план) применяются ко
  всем костям, включая dummy: кончик DummyBoneLength толкается лимитом и
  вращает родителя через write-back (UE-паритет).
- Направление dummy-кости — вдоль сегмента лист↔родитель (нулевая длина —
  fallback по оси Z кости); в UE dummy всегда растёт от forward позы
  (GetBoneForwardVector(PoseRotation)).
- Портировано: **world collision** (`world_collision_*`, лучи родитель→кость, dummy-сегменты тоже проверяются; авто-исключение `CollisionObject3D`-предков скелета), **доп. корни**, **bone subdivision**, **bone constraints**.
- Не портировано (вне MVP): box-лимиты, mesh collision, cloth constraints, ragdoll, wind, подшаги (substeps), shared collision, физический слой коллизий.

## KawaiiPhysicsManager (LOD и квота)

Нода `KawaiiPhysicsManager` (не autoload — кладётся в сцену): находит все `KawaiiPhysics` в группе `kawaii_physics`, раз в `update_interval` сортирует по дистанции до `target` (или камеры) и держит включёнными не более `max_active_chains` цепей в радиусе `lod_distance`. При `frame_budget_usec > 0` дополнительно гасит самые дальние активные цепи, пока сумма их `step_usec_ema` не влезет в бюджет (ближайшая всегда остаётся). Выключение — через `active = false` (движок не вызывает модификатор, 0 стоимости); при возврате — `reset_dynamics()` без скачка. Ручные цепи (`active=false`, шаг через `step_manual`) управляются через `lod_enabled`; движковые — через `active`. `cull_by_view` дополнительно отсекает цепи вне фрустума камеры. Без `target` и камеры (headless) — безопасный no-op.
Дистанция считается до **центра цепи** (`KawaiiPhysics.get_chain_center()`, среднее мировых позиций реальных костей), а не до позиции скелета. Встроенная статистика: `KawaiiPhysics.last_step_usec` (µs последнего шага) и `step_usec_ema`, `KawaiiPhysicsManager.get_stats()` → `chains/active/step_usec/step_usec_ema/budget_skipped/sleeping`, опция `print_stats`. Сон: `sleep_enabled` — цепь засыпает при `sleep_frames` шагах с движением ниже `sleep_motion_threshold` и пропускает шаги (0 стоимости), просыпается от движения скелета/смены позы или `wake()`. Per-LOD настройки: `lod_steps` (ступени дистанции) + `lod_presets` (KawaiiPreset на ступень) — при переходе ступени пресет применяется.

## Shared collision (цепи одного скелета)

`shared_collision_enabled`: модификаторы одного `Skeleton3D` регистрируются в статическом реестре; кости реальных цепей расталкиваются как сферы (`radius` + `radius`), каждый модификатор двигает свои кости на половину проникновения и восстанавливает длины звеньев. Включай на обеих цепях для симметрии. В редакторе не регистрируется.

## KawaiiRagdollBuilder (физический ragdoll)

Утилита `KawaiiRagdollBuilder` (static): `build(sk, only)` создаёт `PhysicalBoneSimulator3D` + `PhysicalBone3D` на выбранные кости — `bone_name`, поза кости, pin-joint (`JOINT_TYPE_PIN`), сфера-коллайдер (радиус от длины кости); `start(sim)` запускает симуляцию **с именами костей** (без этого физические кости не стартуют); `pair_distances(sim)` — контроль длин звеньев. Проверено на Sammy: суставы держат длины (0.3/0.3; хвост 0.1428; волосы 0.08–0.09), ragdoll blend к физическому источнику — 4/4 кости.

## Ragdoll blend

`ragdoll_blend` + `ragdoll_source`: после симуляции поза каждой кости цепи смешивается с глобальной позой одноимённой кости в `Skeleton3D`-источнике (origin lerp, basis slerp). `ragdoll_source` может быть скелетом под `PhysicalBoneSimulator3D` (отдельный инстанс) — так Kawaii-цепь плавно отдаёт управление физическому ragdoll. При `ragdoll_blend = 1` выход равен позе источника; при 0 — симуляции. Позиции/повороты источника берутся в пространстве скелета-источника (риги должны совпадать по именам костей). Если у скелета-источника есть дочерний `PhysicalBoneSimulator3D`, блендинг читает трансформы `PhysicalBone3D` (по `bone_name`) — так работает связка с настоящим физическим ragdoll (позы скелета там пишутся через override и `get_bone_global_pose` их не видит).

## Ручной шаг и менеджер

Если меши висят на `BoneAttachment3D` и движковый путь не двигает их, шагай вручную: `mod.active = false`, в своём `_process` вызывай `mod.step_manual(delta)` (учитывает `lod_enabled`) и пиши позы обратно. `KawaiiPhysicsManager` управляет и движковыми (`active=true`), и ручными цепями: для ручных он переключает `lod_enabled` (LOD/квота/бюджет/видимость через `cull_by_view`), а `step_manual` пропускает выключенные. `debug_draw` в редакторе — после Reload Project (класс `KawaiiDebugDraw` загружается динамически).

## Известные ограничения (проверено)

- `simulation_space = WORLD` — политика инерции (полная компенсация), не смена координат для лимитов/коллизий; `BASE_NODE` — компенсация от внешнего узла.
- Субдивижн: collision-only есть; рёбра к dummy-кончику и ветвления (2+ детей) не сабдивятся; горизонтальные констрейнты — с subdivision/bridge-фидбеком.
- `_anim_global` не видит override других engine-модификаторов на той же цепи (для предков вне цепи видит).
- Ragdoll: есть `ragdoll_blend` + `ragdoll_source` (блендинг к позе отдельного скелета, например под `PhysicalBoneSimulator3D`); готового физического ragdoll-скелета аддон не создаёт.
- Twist не выражается точечным солвером (есть `limit_angle_deg` = bend/swing); pose-множители — только `gravity_scale`; warping заменён порогом телепорта; bone scaling уже в pose-длинах; GPU/deterministic не портируем.
- Гизмо лимитов в редакторе нет (лимиты видны в `debug_draw`/превью).
- `settings` создаётся в `_ready` (не в `_init`); в редакторе для `editor_preview` используется внутренний `_tool_settings`, сцена не помечается грязной.
- Менеджер в сцены по умолчанию не добавлен; ручные цепи (`active=false`) управляются через `lod_enabled`, движковые — через `active`; состав группы пересканируется в `update_lod`.

## Квирки Godot 4.7

- Движок вызывает виртуалку `_process_modification_with_delta(delta)`
  напрямую (старая `_process_modification()` в 4.7 не вызывается) —
  доказано headless-smoke: `tests/smoke_kawaii.gd`, 90/90 кадров.
- `get_skeleton()` может вернуть null вне стандартного процесса — нода
  падает на `get_parent() as Skeleton3D`.
- `get_bone_global_pose_no_override()` до первого dirty-пересчёта возвращает
  нули кеша — снимок поз строится из `get_bone_pose` локальной цепочки.

## Тесты

- `tests/test_kawaii_sim_core.gd` — 32 теста мат-ядра (верлет, лимиты
  сфера/капсула/план по UE-семантике, self-collision, restore, угловые лимиты,
  safe-хелперы, детерминизм).
- `tests/test_kawaii_world_collision.gd` — world collision (в т.ч. с сабдивижном)
- `tests/test_kawaii_multi_root.gd` — дополнительные корни
- `tests/test_kawaii_subdivision.gd` — dummy-кости между реальными
- `tests/test_kawaii_wind.gd` — ветер (детерминизм, сила/направление, отклонение цепи)
- `tests/test_kawaii_wind_zone.gd` — зоны box/sphere, falloff, цепь в зоне
- `tests/test_kawaii_manager.gd` — LOD/квота, ручной active=false, нет камеры/цели, статистика
- `tests/test_kawaii_moving_skeleton.gd` — движущийся/вращающийся скелет, телепорт
- `tests/test_kawaii_simulation_space.gd` — Component vs World на скачке скелета
- `tests/test_kawaii_curves.gd` — кривые stiffness/damping по длине цепи
- `tests/test_kawaii_sync_bones.gd` — SyncBones (target вне цепи, выключение, нет target)
- `tests/test_kawaii_sleep.gd` — сон/пропуск шагов/пробуждение
- `tests/test_kawaii_collision_only.gd` — collision-only сабдивижн и фидбек
- `tests/test_kawaii_substepping.gd` — число подшагов/кламп, стабильность на dt=0.1
- `tests/test_kawaii_box_limit.gd` — бокс-лимит выталкивает кости, выключение
- `tests/test_kawaii_planar.gd` — planar-режим держит цепь в плоскости, без него — нет
- `tests/test_kawaii_impulse.gd` — импульс толкает цепь, будит сон
- `tests/test_kawaii_constraint_subdivision.gd` — bridge-фидбек раздвигает концы
- `tests/test_kawaii_external_forces.gd` — curve/constant силы и отклонение цепи
- `tests/test_kawaii_capsule_preset.gd` — capsule-зона, применение пресета
- `tests/test_kawaii_shared_collision.gd` — цепи одного скелета расталкиваются
- `tests/test_kawaii_prediction_gravity.gd` — gravity_scale/prediction_scale
- `tests/test_kawaii_game_scenes.gd` — интеграция `KawaiiSammy.tscn` (18 цепей) и `KawaiiHuman.tscn` (ragdoll-цепи)
- `tests/test_kawaii_ragdoll_blend.gd` — ragdoll_blend заменяет позу позой источника
- `tests/test_kawaii_manager.gd` — LOD/квота/статистика/бюджет/per-LOD пресеты
- Скрипт `@tool`: `editor_preview = true` шагает симуляцию в редакторе; `debug_draw` рисует и скорости (голубые отрезки)
- `tests/test_kawaii_ccd_groups.gd` — CCD через тонкую стену, группа-исключение
- `tests/test_kawaii_mesh_collision.gd` — кость выталкивается из сферы тела, выключение\n- `tests/test_kawaii_bone_constraints.gd` — горизонтальные связи
- `tests/test_kawaii_physics_node.gd` — 39 нодовых тестов (цепь, гравитация,
  influence-бленд без накопления, телепорт-пороги, смена root/exclude/dummy в
  рантайме (включая очистку override костей, покинувших цепь при пересборке),
  лимиты (включая скип нулевого радиуса сферы и капсулы и толчок
  dummy-кончика сферой), self-collision, warmup, reset, constant_force,
  target_framerate, debug draw, смена скелета, восстановление после
  исключения root-кости, очистка override на пустой цепи и при пустом
  root_bone, переживание чужого override на кости вне цепи, подкласс
  settings).
- `tests/smoke_kawaii.gd` — headless smoke: реальный движок гоняет модификатор
  90 кадров, маркер `engine_calls=90`, движение цепи под гравитацией.

Запуск: `verify.ps1 -Task kawaiiplan` или GUT вручную
(`-gtest=res://tests/test_kawaii_sim_core.gd`).

## Проверка на реальном скелете (уровень игры)

`TSCN/KawaiiHuman.tscn` + `scripts/KawaiiHuman.gd` — простая человекообразная модель для проверки аддона вне тестовых шагов:

- `Skeleton3D` из 18 костей (root/hips/spine/chest/neck/head, руки, ноги, коса `hair_1..4`);
- детализированные меши на `BoneAttachment3D` (без скиннинга): голова с глазами/ушами/волосами, туника с воротником и пряжкой, суставы, сапоги с подошвой, коса;
- шесть `KawaiiPhysics` — детей скелета: коса (`root_bone = "hair_1"`, damping 0.08, stiffness 0.03, limit 85°) и пять ragdoll-цепей (`spine`, `arm_l/r`, `leg_l/r`; `influence = 0` до смерти, у `spine` — `exclude_bones` на косу и руки, чтобы цепи не перетирали друг друга);
- процедурная анимация ходьбы в `_process` (ноги/руки/таз) — цепь реагирует на движение тела;
- `capture_rest()`, `ponytail_tip()`, `ponytail_deviation()`, `head_height()`, `die()`, `is_ragdoll_active()` — метрики и смерть (падение + обмякание, упрощённый Euphoria).
- Нюанс: модель без скина, поэтому модификаторы создаются с `active = false` (иначе движок шагает их сам), а `KawaiiHuman._process` шагает их вручную и переносит симулированные глобальные позы в позы костей — только так `BoneAttachment3D` видят результат.

`tests/test_kawaii_human.gd` (5/5) + `tests/test_kawaii_ragdoll.gd` (4/4): модификаторы висят на `Skeleton3D`, коса качается при анимации (dev до 0.26 м), стабильна в покое (< 2 см), едет за головой при телепорте; при `die()` все ragdoll-цепи включаются, голова опускается 1.52 → 0.10 м, предплечье меняет ориентацию. Рендер-проба `dev_tools/probes/probe_kawaii_human.gd` печатает `KAWAII_HUMAN shot=N dev=...` и снимает кадры (+ `debug_draw`, кадры смерти `32_death_*`) в `.opencode/evidence/kawaii_game/shots/`.

Ограничение: модель — тестовый стенд, в игровые сцены не встроена; реальные скины/анимации персонажей в проекте отсутствуют (мобы — билборды, игрок — FPS-капсула).