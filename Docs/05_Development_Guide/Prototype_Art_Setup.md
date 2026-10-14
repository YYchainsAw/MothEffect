# Moth Effect：T20 原型外观与替换入口

版本 v0.1 · 2026-10-13 · 状态：原型资产和接口已接入；当前玩家 PIE 与独立包待验收

本轮提供三种原创机关外壳、一个金属掩体模块、一个符文灯饰和共用材质，作为你后续深度修改机制与全部替换 UI 的可运行基线。模型由可复现 Python 几何代码生成，你可以改源几何、换自己的模型或关闭这层外观。实际构建、回归、渲染与保留失败见 [T20 报告](../06_Test_Doc/T20_Report_2026-10-13.md)。

## 当前入口

打开 `/Game/MothEffect/Maps/L_Arena_01`，Play 后标题、教学与战斗共用原型场景外观；三种设备与敌人背包读取同一外观配置。主地图、现有机关/敌人/玩家蓝图没有重存。

| 资产/接口 | 调整内容 |
| --- | --- |
| `/Game/MothEffect/PrototypeArt/DA_PrototypeArt` | 三种机关的模型、材质、局部显示 Transform；掩体和灯饰开关、灯饰布置与亮度 |
| `/Game/MothEffect/PrototypeArt/Meshes` | `SM_D01Launcher`、`SM_D02Bomb`、`SM_D03Emitter`、`SM_CoverModule`、`SM_RuneBeacon` |
| `/Game/MothEffect/PrototypeArt/Materials` | 两个母材质、九个实例；金属 `Tint/Metallic/Roughness`，发光 `Tint/Intensity` |
| 机关 → `Device / Art` | `bUsePrototypeAppearance`、`PrototypeArtProfile`、`RefreshPrototypeAppearance`；可按类/实例使用不同配置 |
| GameMode → `Moth / Prototype / PrototypeSceneArtClass` | 主竞技场场景层类；清空则不自动生成，或替换为你的子类 |
| `MothPrototypeSceneArt` Actor | `bEnabled`、`Profile`、`RefreshArt`；可以放进编辑器世界预览，或运行时刷新/关闭 |

保存 Data Asset 后停止并重新 Play，使机关、背包与场景一致重载。编辑器中的现有机关实例可用 `RefreshPrototypeAppearance` 更新；敌人重新构造后更新背包。原型场景层没有每帧重建，编辑配置后用 `RefreshArt` 明确刷新。

## 换模型与材质

三种默认轮廓为：D01 青色向上棱柱/底座，D02 橙色晶核/封印环，D03 紫色定向枪管。默认设备顶点均在原点半径 20cm 内，根物理球和射击球仍由原有玩法组件负责。D03 模型枪口朝局部 +X；Active 世界锁定箭头继续来自 T19 反馈组件。

`Devices` 按 `Launcher/Bomb/Emitter` 选择 `Mesh`、`Materials` 和 `RelativeTransform`。默认材质列表的三项含义是 Metal、Copper、Glow：原生代码按网格槽名查找，适应 Unreal 导入后的槽重新排序。自有模型可以保留这三个槽名；按自己的槽顺序分配时关闭 `bNamedMaterialSlots`，列表改为实际索引顺序。模型没有对应槽名时回退到列表索引。

敌人背包读取实际 `DropDeviceClass` 的 `GetVisualAppearance()`，保持类型、模型与材质对应；背包比例为设备显示比例的 1.4 倍，安装位置属于敌人组件。只改显示 Transform 会改变外壳显示，不会移动 Actor、修改物理球、命中球、质量、拾取距离或发射方向。

需要自己的直接组件外观时，关闭机关 `bUsePrototypeAppearance`，再编辑 `DeviceMesh`。运行时关闭会恢复本轮覆盖前的外观；已经直接替换的用户模型/材质会优先保留。复制 Profile 并修改 `PrototypeArtProfile` 可以按设备变体调整；默认路径不存在或外观关闭时使用已有灰盒。

若你打算改变实际尺寸与规则，同步修改 `BodyRadiusCm`、`ShotRadiusCm`、相关玩法配置/文档与受影响测试即可；20cm 是当前原型基线，不限制你的后续设计。

## 掩体与符文场景层

主 GameMode 仅在 `L_Arena_01` 自动生成一个 `MothPrototypeSceneArt`。已有场景层时不会重复生成；自定义地图可以自行放置该 Actor。层识别 `T17Arena` 标签与矮/高掩体材质，并按 `CoverSelectionMaxExtentCm` 排除共用材质的外围大墙。默认对应六块矮掩体和三块高掩体。

原始 Cube 继续承担阻挡、导航与承托，层只隐藏其渲染并添加无碰撞/Overlap/导航的模块。`SM_CoverModule` 每轴 [-50,50]cm，复制原 Cube 的实际世界 Transform；关闭/刷新/销毁层会恢复原可见性。模型局部的凹槽与铜饰不会打开玩法碰撞，当前掩体依旧是完整盒体。你修改场地碰撞时，应同时调整实体与显示，随后验证射线和通路。

默认北侧有三个符文灯饰，位置/朝向/缩放在 `RuneTransforms`。`RuneLightIntensity` 与 `RuneLightRadiusCm` 控制局部无阴影点光源；亮度或半径设 0 可保留模型并关闭灯光。`bCoverArt`、`bRuneArt` 可以分别关闭；Actor 的 `bEnabled` 关闭整层。显示组件本身不计入机关容量。

## 原始制作与初始化

[制作源](../../SourceArt/PrototypeArt/README.md) 与 [清单](../../SourceArt/PrototypeArt/PrototypeArtManifest.json) 记录五件 OBJ、MTL、尺寸、三角数和 SHA256。几何由本项目标准库代码构造，没有下载网格或纹理；仓库未提供独立 LICENSE，不据此声明公共领域或其他开放授权。

- [generate_t20_art.py](../../scripts/generate_t20_art.py) 显式生成会重写自有源文件；用户修改先另存。`--check` 只读核对源文件、法线/绕序/UV、设备半径和字节确定性。
- [create_t20_art.py](../../scripts/create_t20_art.py) 在已构建的 Editor Python 中执行，只创建缺失自有包。已有模型、材质与 Profile 保留；不保存玩家/机关/敌人/HUD/主地图，不调用 SaveAll。
- OBJ 为 cm、+Z 上、+X 前。UE legacy FBX OBJ 导入执行 Y 轴左右手转换，脚本按此验证实际导入 bounds；D03 的 +X 与向上 +Z 保留。材质槽按名字使用。
- `/Game/MothEffect/PrototypeArt` 已加入 Always Cook。配置目录改变时同步 Cook 引用；独立包实际资源与表现留 T21。

## 机制与 UI 深度改造

外观入口见 [Profile](../../Source/MothEffect/Public/Art/MothPrototypeArtProfile.h)、[机关应用](../../Source/MothEffect/Private/Art/DeviceAppearance.cpp)、[场景层](../../Source/MothEffect/Public/Art/MothPrototypeSceneArt.h)。机制由现有 Devices、Characters、AI、Framework 等模块负责；表现没有接管生命、激活、投掷或波次结算。你确定新规则后同步对应权威文档、实际生效配置和回归测试，旧断言随新设计调整。

UI 可以整套替换。当前 [UI 编辑入口](UI_Setup.md) 提供数据与按钮接口；全面改变控件名称或结构时同步调整原生绑定和控制器接入。T20 的模型、材质和场景层独立于 UMG 布局，T19 命中/受伤 Alpha 可以用于你的新 UI。

## 依赖与恢复边界

UE 实测版本为 5.8.0-55116800；原 Rifle、Murdock 和既有动画/持物接口继续使用当前工程配置。`Content/MothEffect` 是跟踪内容，ParagonMurdock/ThirdPerson/Characters/Input 等外部目录被 Git 忽略；复制或恢复项目时须带回原已获授权的依赖包及其原 `/Game/...` 路径。

已有记录尚缺完整第三方包来源版本、许可核对和实际完整恢复成功证据。运行日志仍有 Murdock `LevelStart_Montage` 的 ParagonShinbi `LevelStart` 缺失、旧 Rig/输入、动画除零和空粒子提示；本轮没有用新原型伪造这些资源。原始错误/告警在交付证据保留，依赖修复及独立包仍需实际验证。

## 当前用户验收

以下五组全部**待验收**，使用实际 Murdock 玩家与正常渲染。助手自动化与截图不能替代你的操作和当前机器表现：

1. **轮廓与背包**：远近观察三种机关与三个敌人；击杀后背包类型与实际掉落一致，颜色、形状和 Active/Spent/过期反馈可辨。
2. **持物、瞄准与玩家表现**：拾取/放下/投出、走动/跳跃/瞄准/射击/换弹；模型不遮关键准星与持物提示，观察 TC34/TC35/TC38 受影响分支。
3. **方向与命中**：地面四向和空中启动 D03，箭头与实际 P01 一致；外壳滚动不改变锁定方向；三机关/自然连锁与受阻枪口按实际规则工作。
4. **场地与开关**：绕行掩体、走两侧坡道，敌人导航与枪弹遮挡正常；灯饰不挡路，开关/刷新/重开没有重复模型，关闭层恢复灰盒。调灯光确认实际亮度与帧率。
5. **独立包与依赖**：T21 中核对模型/材质/字体/音效、现有玩家依赖、独立启动与整局重开；恢复缺包后重新验证，最终候选包交 T25。

用户确认后按实际范围更新 [#22](https://github.com/YYchainsAw/MothEffect/issues/22)。原先 T14–T19 待验收范围保留，完整 G4 不随代码或截图自动通过。
