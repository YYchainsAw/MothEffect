# Moth Effect：竞技场第一轮画面优化接入

版本 v0.1 · 2026-10-18 · 首轮静态画面与接入已验证，动态清晰度和性能待验收。

本轮开启 TSR 抗锯齿，并为正式 SoulCity 竞技场统一灯光强度、固定曝光和基础调色。保留现有暖色夕阳与冷色阴影；不重建场景材质、几何体或地图资产。主要配置入口为 `Config/DefaultEngine.ini` 与 `Config/DefaultGame.ini`，运行时实现为 `AMothArenaVisualDirector`。

## 接入范围

`DefaultEngine.ini` 的 `r.AntiAliasingMethod=4` 开启项目默认 TSR。`r.TemporalAA.Upsampling=True` 单独存在时不会开启抗锯齿，旧配置的 `r.AntiAliasingMethod=0` 才是此前未启用抗锯齿的原因。TSR 设置作用于项目渲染默认值，不限于 SoulCity；后续控制台、相机或用户画质设置仍可能覆盖实际效果，应以运行时控制台值为准。

`AMothEffectGameMode::BeginPlay` 仅在 `L_Arena_SoulCity` 中生成一个未保存到地图的 `AMothArenaVisualDirector`，已有同类实例时不重复生成。导演自身也检查 `TargetMapName`，因此误放到其他地图时不会应用。`L_Arena_01` 的原型场景装饰逻辑继续使用原有接入。

导演不启用 Tick，通过一次 `ApplyLook()` 设置灯光与无边界后处理组件：

- 选择可见、投射阴影的方向光中原始强度最高的一盏作为主光；可见且不投射阴影的方向光作为补光。
- 选择可见天空光中原始强度最高的一盏作为天空光；其他天空光与点光、聚光等局部灯光不改。
- 只调整所选灯光强度，保留位置、旋转、颜色和阴影开关。重复应用时仍按保存的原始强度选择，避免切换主光。
- 后处理使用手动曝光并关闭物理相机曝光，设置曝光补偿、饱和度、对比度、阴影 Gamma 与 Bloom，同时将运动模糊量和色差设为 0。
- 无边界后处理权重为 1，优先级默认为 -10。更高优先级的关卡或相机效果仍可覆盖对应参数。

这层表现设置不修改敌人、子弹、机关、碰撞或导航数据，也不保存 `.umap` 或供应商素材。

## 参数与调整

数值权威记录在[参数账本](../02_Design_Doc/GDD/Gameplay_Parameters.json)的 `arenaVisuals`；以下为 `DefaultGame.ini` 中 `[/Script/MothEffect.MothArenaVisualDirector]` 的首轮配置。修改配置后重启运行；在编辑器预览实例上修改属性后调用 `ApplyLook()` 可立即重看效果。

| 参数 | 首轮值 | 用途 |
| --- | --- | --- |
| `bEnabled` | `True` | 是否允许应用灯光与后处理 |
| `TargetMapName` | `L_Arena_SoulCity` | 限定应用的地图短名称 |
| `SunIntensity` | `2.9` | 主方向光强度 |
| `SkyIntensity` | `1.2` | 主天空光强度 |
| `FillIntensity` | `0.65` | 无阴影方向补光强度 |
| `ExposureCompensation` | `1.263` | 固定曝光补偿，运行时限制在 -15～15 |
| `Saturation` | `0.96` | 全局饱和度，运行时限制在 0～2 |
| `Contrast` | `0.98` | 全局对比度，运行时限制在 0～2 |
| `ShadowGamma` | `1.06` | 阴影调色 Gamma，运行时限制在 0.01～2 |
| `BloomIntensity` | `0.2` | 高亮泛光强度 |
| `PostProcessPriority` | `-10.0` | 基础后处理优先级 |

`ExposureCompensation=1.263` 校准此前默认固定曝光与手动曝光路径的亮度换算。首版1.0和较低补光使阴影偏暗，最终保留原补光强度、稍增天空光，并用阴影Gamma补细节；太阳光由原3.4稍降为2.9。实际观感以同机位原图为准。

灯光原始强度在本次应用前保存在导演实例中。`RestoreLook()` 恢复仍存活灯光的原始强度，禁用本组件后处理并清空快照；销毁实例或结束游玩时也会恢复。它不会清除其他后处理效果。

若要关闭本轮灯光与后处理，将配置的 `bEnabled=False` 后重启运行。运行中修改实例的 `bEnabled` 不会自动轮询生效，需要调用 `ApplyLook()`，其禁用分支会恢复，或直接调用 `RestoreLook()`。这两种操作都保留全局 TSR；完整回到旧抗锯齿设置还需单独将 `r.AntiAliasingMethod` 改回 `0`。

## 编辑器预览

普通编辑器视口没有 GameMode 的运行时自动生成步骤，所以直接打开未修改地图时看到的是保存的原场景灯光。可进入 PIE 查看运行时效果，或在 SoulCity 地图中放置 `MothArenaVisualDirector`，从细节面板调用 **Apply Look**。单独放置预览实例不会因构造而自动应用，便于先检查参数。

也可在已启用 Python 编辑器脚本插件的临时工程中执行：

```python
import unreal
actors = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
cls = unreal.load_class(None, "/Script/MothEffect.MothArenaVisualDirector")
look = actors.spawn_actor_from_class(cls, unreal.Vector(0, 0, 0))
look.apply_look()
```

预览结束时恢复并删除该未保存实例：

```python
look.restore_look()
actors.destroy_actor(look)
```

预览不要保存地图；如需保存正式关卡前的编辑，应先恢复并删除预览实例。运行时自动生成的导演带 `RF_Transient` 标记，而上面的编辑器生成操作依赖“不保存地图”的预览流程。

## 固定机位 A/B 截图

配套脚本是 `scripts/stage_arena_visual_pass.py` 和 `scripts/capture_arena_visual_pass.py`。前者用普通 Python 准备独立工程，后者在真实 D3D12 编辑器中执行。截图需要图形渲染，不能使用 `-NullRHI` 或命令行无图形渲染结果替代。

1. 编译包含视觉导演的 Development Editor。关闭将要同步的临时工程编辑器后，在仓库根目录运行 `py scripts/stage_arena_visual_pass.py prepare`。准备步骤只允许首次创建；已有 Stage 时会拒绝覆盖。
2. 临时工程位于 `Saved/Automation/ArenaVisualPass/Stage_20261018_01`。它复制配置和二进制，为 Content 子目录建立目录联接，并启用 Python/编辑器脚本插件、关闭自动保存。目录联接不是文件系统只读保护；捕获流程禁止保存包，并用前后哈希检查源文件。
3. 在进程环境设置 `MOTH_VISUAL_CAPTURE_MODE=Before`，使用临时 `.uproject` 启动真实 D3D12 编辑器，通过 `-ExecutePythonScript=<仓库绝对路径>/scripts/capture_arena_visual_pass.py` 执行脚本。等待它保存结果并退出。
4. 如主工程二进制已更新，待上述进程关闭后运行 `py scripts/stage_arena_visual_pass.py sync-binaries`。再设置 `MOTH_VISUAL_CAPTURE_MODE=After`，用全新编辑器进程执行同一捕获脚本。
5. 两次均完成后运行 `py scripts/stage_arena_visual_pass.py verify`，确认源地图及 `Content/MothEffect` 文件哈希未变化。检查两个 `<Mode>_CaptureResult.json` 的 `success` 和截图文件，不能只看编辑器退出码。

脚本读取地图中保存的 `SC_View_Entrance`、`SC_View_Platform` 相机。两种模式统一输出 1600×900，所有 `sg.*` 画质档设为 3，渲染比例 100%，动态分辨率关闭，加载已用纹理。每个相机至少预热 30 秒、120 个视口帧，并要求着色器编译结束后的帧数达到 60；高分辨率截图额外使用 `r.HighResScreenshotDelay=64`。

Before 模式强制 AA 为 0，使用地图中保存的灯光，不生成视觉导演；After 模式强制 AA 为 4，并生成未保存的视觉导演调用 `ApplyLook()`。因此这组对比同时验证抗锯齿、灯光和调色整轮效果；若需单独判断 TSR 或某一个光照参数，应另做只改一项的对比。

输出保存在 `Saved/Automation/ArenaVisualPass`：

- `Before_Entrance.png`、`Before_Platform.png`、`After_Entrance.png`、`After_Platform.png`：原始截图。
- `Before_CaptureResult.json`、`After_CaptureResult.json`、`CaptureResult.json`：相机、实际控制台值、灯光强度、截图时间和源资产哈希等记录。
- `AssetVerification.json`：源资产与地图哈希核对结果；`Backup/L_Arena_SoulCity.umap` 为准备阶段保留的原地图副本。

应同时检查建筑、电线与栏杆边缘、亮地面细节、阴影内敌人轮廓，以及 Bloom 是否遮盖枪口和弹道预警。后两项需要在真实战斗中补充观察，固定空场机位只能支持场景观感判断。

## 验证边界与待补记录

UE5.8 Development Editor构建成功，两项 `MothEffect.ArenaVisual` 专项无警告通过；SoulCity真实Game专项通过，验证单实例视觉导演、TSR配置和54次出生/击杀、24峰值。入口/平台两机位D3D12原图与133个游戏资产SHA核对见[原始证据](../06_Test_Doc/Evidence/ArenaVisualPass/Automation_2026-10-18.json)。测试导出逐项核对，未以进程退出码代替结果。

可拖动查看[前后对比](../06_Test_Doc/Evidence/ArenaVisualPass/Compare.html)。原图：[入口Before](../06_Test_Doc/Evidence/ArenaVisualPass/Before_Entrance.png) / [入口After](../06_Test_Doc/Evidence/ArenaVisualPass/After_Entrance.png)，[平台Before](../06_Test_Doc/Evidence/ArenaVisualPass/Before_Platform.png) / [平台After](../06_Test_Doc/Evidence/ArenaVisualPass/After_Platform.png)。截图没有额外修图。

两模式均完成截图和完整元数据输出，但在退出时触发UE Python解释器GC崩溃，进程码为3。脚本已改为延迟退出、解绑并清理Python引用，重新捕获后此退出问题仍存在；保留首轮与最终原始日志，不把进程记为正常通过。两个图形捕获结果、资产SHA和不加载Python的原生Editor/Game验证分别记录，游戏运行回归没有该退出异常。

固定机位截图能够检查静态边缘和明暗层次，不能证明快速转向时的 TSR 拖影、移动目标清晰度、实际帧率或同屏 24 敌人的 GPU 性能。NullRHI 战斗回归可验证对象池、波次与视觉导演接入逻辑，无法验证图形质量和 GPU 耗时。性能与动态可读性应在真实 D3D12 战斗中，使用一致分辨率、画质和敌人数另行记录。
