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
| `simulation_space` | enum | `COMPONENT` | `COMPONENT` / `WORLD` (полная инерция мира) |
| `root_bone` | StringName | `""` | Корень цепи; вниз собираются все дочерние кости |
| `exclude_bones` | Array[StringName] | `[]` | Кость и всё её поддерево исключаются из цепи |
| `settings` | KawaiiSettings | — | damping/stiffness/world-damping/collision_radius/limit_angle_deg + кривые stiffness/damping |
| `additional_root_bones` | Array[StringName] | `[]` | Дополнительные корни (независимые цепочки в одном модификаторе) |
| `influence` | float 0–1 | 1.0 | Смешивание результата с позой (унаследовано от SkeletonModifier3D) |
| `bone_subdivision_count` | int | 0 | Dummy-кости между реальными костями (0–10): плавнее изгибы и коллизии |
| `bone_constraints` | Array[KawaiiBoneConstraint] | `[]` | Горизонтальные связи между костями разных цепочек (кость A, кость B, stiffness) |
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
| `debug_draw` | bool | false | Отладочная отрисовка (см. ниже) |

'## Ресурс KawaiiSyncBone

`source_bone` (симулированная кость), `target_bone` (кость-приёмник, может быть вне цепи), `offset` (Transform3D), `enabled`. После write-back target получает глобальную позу source с offset; target добавляется в список override и снимается при следующем шаге. Полезно для аксессуаров/отделённых частей.

## Кривые в KawaiiSettings

`stiffness_curve` и `damping_curve` (Curve, опционально) — множители по длине цепи: `t = индекс кости / (n-1)` (0 — корень, 1 — кончик). Позволяют сделать «мягкий кончик, жёсткий корень» без отдельных модификаторов. Без кривых параметры равномерные.

## Ресурс KawaiiWind

`enabled`, `direction` (мировой), `strength`, `turbulence` (0–1), `noise_scale`, `noise_speed`, `noise_seed` (детерминированный FastNoiseLite). `sample(time, world_pos)` отдаёт силу в мировом пространстве; модификатор добавляет её к `gravity + constant_force` с множителем `wind_scale`. Порывы = шум по времени/позиции; сила ветра равномерна по цепи (по-костный градиент — вне MVP).

## Ресурс KawaiiSyncBone

`source_bone` (симулированная кость), `target_bone` (кость-приёмник, может быть вне цепи), `offset` (Transform3D), `enabled`. После write-back target получает глобальную позу source с offset; target добавляется в список override и снимается при следующем шаге. Полезно для аксессуаров/отделённых частей.

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

Нода `KawaiiPhysicsManager` (не autoload — кладётся в сцену): находит все `KawaiiPhysics` в группе `kawaii_physics`, раз в `update_interval` сортирует по дистанции до `target` (или камеры) и держит включёнными не более `max_active_chains` цепей в радиусе `lod_distance`. Выключение — через `active = false` (движок не вызывает модификатор, 0 стоимости); при возврате — `reset_dynamics()` без скачка. Модификаторы, у которых `active` был выставлен вручную (например, ручной шаг демо), не трогаются. Без `target` и камеры (headless) — безопасный no-op.
Дистанция считается до **центра цепи** (`KawaiiPhysics.get_chain_center()`, среднее мировых позиций реальных костей), а не до позиции скелета. Встроенная статистика: `KawaiiPhysics.last_step_usec` (µs последнего шага), `KawaiiPhysicsManager.get_stats()` → `chains/active/step_usec/step_usec_ema`, опция `print_stats`.

## Известные ограничения (проверено)

- **Ветер** (`KawaiiWind`/`KawaiiWindZone`) сэмплируется по-костно (у каждой кости своя сила), зоны box/sphere с falloff; отдельного высотного профиля силы (градиент по высоте) нет — его можно собрать из нескольких зон.
- **Субдивижн**: dummy-кости полноценно интегрируются; режима «collision-only» (UE `bBoneSubdivisionCollisionOnly`) и передачи смещения с dummy на концы нет. Рёбра к dummy-кончику и ветвления (кость с 2+ детьми) не сабдивятся.
- **Горизонтальные связи** — простые дистанционные констрейнты: без сабдивижна вдоль связи и без bridge-переноса (UE `BoneConstraintSubdivisionCount`).
- **`_anim_global`** для костей внутри цепи читает `sk.get_bone_pose` (анимацию), поэтому override другого engine-driven `KawaiiPhysics` на той же цепи не учитывается; для предков вне цепи используется `get_bone_global_pose` (override учитывается).
- **SimulationSpace**: `simulation_space` = `COMPONENT` (по умолчанию; скелет-локальные координаты + компенсация движения по `world_damping_*`) или `WORLD` (полная компенсация движения/поворота скелета — жёсткая инерция, `world_damping_*` игнорируется; цепь всё равно тянется ограничением длин). `BASE_BONE` не реализован и не нужен в текущей архитектуре: все кости скелета уже в его пространстве, анимация base-кости уже входит в `pose`; base-кость имела бы смысл только для внешнего аттача. Проверено на тестовой сцене пользователя и GUT.
- Нет: editor-preview (осознанно отложен: требует @tool-режима и шага в редакторе; ценность ниже риска), SyncBones, кривых stiffness/damping, box-лимитов, mesh collision, cloth constraints, ragdoll, подшагов, shared collision, физического слоя коллизий.
- `settings` создаётся в `_ready` (не в `_init`): `KawaiiPhysics.new().settings == null` до входа в дерево.
- В игре новые возможности (F1–F4) пока ни к чему не подключены — по договорённости сначала аддон.

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
- `tests/test_kawaii_sync_bones.gd` — SyncBones (target вне цепи, выключение, нет target)\n- `tests/test_kawaii_bone_constraints.gd` — горизонтальные связи
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