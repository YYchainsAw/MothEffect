# Moth Effect（飞蛾效应）：UE 工程配置

版本 v0.11 · 2026-10-06 · 单人 UE 5.8 C++ 与蓝图混合开发

本文件记录工程现状和 G0 配置检查，区分静态核对与运行证据。2026-10-03 本机已核对 UE 5.8.0、VS 2026 18.4 与 Windows SDK 10.0.26100.0 存在，修改前模板目标曾返回 Target is up to date。2026-10-06，[T02 Issue #3](https://github.com/YYchainsAw/MothEffect/issues/3) 已关闭、Project 为 Done，但干净编译、独立 Windows 包版本和启动证据仍未填写。用户负责资产配置与实际验收，助手不代为编译；本次只同步 Docs，不执行 UE 编译或打包。

## 1. 已静态核对的工程

静态核对结果：MothEffect.uproject 的 EngineAssociation 为 5.8，Runtime 模块为 MothEffect；现有 AMothEffectCharacter、AMothEffectPlayerController、AMothEffectGameMode 源码以及 ThirdPerson 蓝图/关卡文件均存在。Build.cs 已声明 EnhancedInput、AIModule、UMG 等依赖。模板还包含 Combat、Platforming、SideScrolling 变体和 StateTree 相关依赖，它们不自动成为本作玩法，G0 时核对标准 ThirdPerson 入口及蓝图父类，不重复添加已有模块，也不先批量删除示例文件。

DefaultGame.ini 当前 ProjectName 与 ProjectDisplayedTitle 均为 Moth Effect，与用户确认的英文名一致。中文显示名统一为飞蛾效应；后续制作标题、窗口名称与投稿资料时采用对应语言的名称。本轮仅更新文档，未修改 DefaultGame.ini。

现有入口为 /Game/ThirdPerson/Lvl_ThirdPerson，默认游戏模式指向 BP_ThirdPersonGameMode。现有输入资产位于 /Game/Input：IMC_Default、IMC_MouseLook 与 IA_Move/Look/MouseLook/Jump。复用方式与新增输入动作见 [技术设计](../02_Design_Doc/TDD/Technical_Design.md)。

工程清单启用了 ModelingToolsEditorMode（仅 Editor）、StateTree、GameplayStateTree。模块声明中还包含 Slate 与 StateTree 相关依赖。插件启用、模块依赖和本作已实现功能是三种不同状态，不因依赖存在而判定玩法已完成。

本地 `/Game/Characters/Mannequins/Anims/Rifle` 曾静态核对有 Fire/Reload/Equip/DryFire、八方向 Walk/Jog、跳跃与 AO_Rifle。T04 的基础移动/瞄准项已在 Issue 勾选，Project 仍为 Pending Acceptance；T05 已勾选基础枪械接入，上半身 Montage 及完整动画中断分支尚待记录。具体步骤见 [Player_Setup](../05_Development_Guide/Player_Setup.md) 和 [Weapon_Setup](../05_Development_Guide/Weapon_Setup.md)，不根据静态资产清单推断全部动画已验收。

用户确认首版不使用 GAS，现有 .uproject/Build.cs 未显式启用或声明 GameplayAbilities/GameplayTags/GameplayTasks；本轮不添加这些依赖，也不为玩家新增 StateTree。后续新增输入与动画资产、镜头 Yaw 转向、Anim Class 配置遵循 [技术设计第 3 节](../02_Design_Doc/TDD/Technical_Design.md#3-越肩射击与输入)；在编辑器逐项核验并以独立包测试，模板资源恢复仍按 [版本管理](../03_Code_Standard/Version_Control.md)。

## 2. 工具链与首个独立包

T02 需补档的验证节点为“现有模板干净编译 → Windows Development 包 → 关闭编辑器独立启动”，不再安排重新建工程。当前入口声明为 /Game/ThirdPerson/Lvl_ThirdPerson，游戏模式为 BP_ThirdPersonGameMode；在编辑器核对后使用。检查 Game Default Map、游戏模式及必需地图/资产能被 Cook；界面和 DataAsset 引用不只靠编辑器临时加载。UE 5.8 的官方兼容表列 VS 2022 17.14+、VS 2026 18.0+；实际工具链与 Windows SDK 需本机核验，本文未替用户安装。[Epic：VS 配置](https://dev.epicgames.com/documentation/en-us/unreal-engine/setting-up-visual-studio-development-environment-for-cplusplus-projects-in-unreal-engine)；[Epic：Packaging](https://dev.epicgames.com/documentation/unreal-engine/packaging-your-project)

实际 UE 补丁号、编译器、SDK、测试机器、构建标识和启动截图需要在执行 G0 时补录。验收任务见 [开发任务与排期](../05_Development_Guide/Development_Plan.md)，记录格式见 [测试计划与验收](../06_Test_Doc/Test_Plan.md)。

## 3. 碰撞、输入与运行配置的来源

JamDevice/JamProjectile 对象通道及 JamWeaponTrace/JamGroundTrace 查询通道是后续计划，碰撞矩阵只维护在 [技术设计](../02_Design_Doc/TDD/Technical_Design.md)。输入门控、物理组件和生命周期也以技术设计为准；本次未向 Config 写入这些设置。

运行时设计数值来自 DataAsset/默认配置，设计账本见 [玩法参数基线](../02_Design_Doc/GDD/Gameplay_Parameters.json)，该 JSON 不承诺自动导入。主地图及 UI/DataAsset 引用完成后，按 [发布检查清单](../07_Release/Release_Checklist.md) 重新核对候选包包含的运行资源。

## 4. 公开仓库的 Android File Server 配置

当前 Windows 单机目标不使用 Android 文件服务器。在 DefaultEngine.ini 的 AndroidFileServerRuntimeSettings 节中，已设 bEnablePlugin=False、bAllowNetworkConnection=False、SecurityToken=（显式空值）；bIncludeInShipping、bAllowExternalStartInShipping、bCompileAFSProject 均保留 False。未修改 .uproject 或该配置节之外的引擎设置。

空 Token 会关闭令牌认证检查，因此必须与禁用该服务器用途同时处理。以后需要 Android 开发时，另行配置私有 Token 与部署方式。上述 AFS 配置已静态核对；未归档与该配置对应的编译/打包结果。[Epic：Android File Server](https://dev.epicgames.com/documentation/en-us/unreal-engine/android-file-server-for-unreal-engine)
