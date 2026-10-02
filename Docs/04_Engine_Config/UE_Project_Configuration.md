# Moth Effect（飞蛾效应）：UE 工程配置

版本 v0.7 · 2026-10-03 · 单人 UE 5.8 C++ 与蓝图混合开发

本文件记录工程现状和 G0 配置检查，区分静态核对与待执行项。已静态核对文件，此前仅修改 Android File Server 的三项配置；本轮仅统一文档名称。未启动编辑器、编译、安装工具链或打包。

## 1. 已静态核对的工程

静态核对结果：MothEffect.uproject 的 EngineAssociation 为 5.8，Runtime 模块为 MothEffect；现有 AMothEffectCharacter、AMothEffectPlayerController、AMothEffectGameMode 源码以及 ThirdPerson 蓝图/关卡文件均存在。Build.cs 已声明 EnhancedInput、AIModule、UMG 等依赖。模板还包含 Combat、Platforming、SideScrolling 变体和 StateTree 相关依赖，它们不自动成为本作玩法，G0 时核对标准 ThirdPerson 入口及蓝图父类，不重复添加已有模块，也不先批量删除示例文件。

DefaultGame.ini 当前 ProjectName 与 ProjectDisplayedTitle 均为 Moth Effect，与用户确认的英文名一致。中文显示名统一为飞蛾效应；后续制作标题、窗口名称与投稿资料时采用对应语言的名称。本轮仅更新文档，未修改 DefaultGame.ini。

现有入口为 /Game/ThirdPerson/Lvl_ThirdPerson，默认游戏模式指向 BP_ThirdPersonGameMode。现有输入资产位于 /Game/Input：IMC_Default、IMC_MouseLook 与 IA_Move/Look/MouseLook/Jump。复用方式与新增输入动作见 [技术设计](../02_Design_Doc/TDD/Technical_Design.md)。

工程清单启用了 ModelingToolsEditorMode（仅 Editor）、StateTree、GameplayStateTree。模块声明中还包含 Slate 与 StateTree 相关依赖。插件启用、模块依赖和本作已实现功能是三种不同状态，不因依赖存在而判定玩法已完成。

## 2. 工具链与首个独立包

后续首个验证节点为“现有模板干净编译 → Windows Development 包 → 关闭编辑器独立启动”，不再安排重新建工程。当前入口声明为 /Game/ThirdPerson/Lvl_ThirdPerson，游戏模式为 BP_ThirdPersonGameMode；在编辑器核对后使用。检查 Game Default Map、游戏模式及必需地图/资产能被 Cook；界面和 DataAsset 引用不只靠编辑器临时加载。UE 5.8 的官方兼容表列 VS 2022 17.14+、VS 2026 18.0+；实际工具链与 Windows SDK 需本机核验，本文未替用户安装。[Epic：VS 配置](https://dev.epicgames.com/documentation/en-us/unreal-engine/setting-up-visual-studio-development-environment-for-cplusplus-projects-in-unreal-engine)；[Epic：Packaging](https://dev.epicgames.com/documentation/unreal-engine/packaging-your-project)

实际 UE 补丁号、编译器、SDK、测试机器、构建标识和启动截图需要在执行 G0 时补录。验收任务见 [开发任务与排期](../05_Development_Guide/Development_Plan.md)，记录格式见 [测试计划与验收](../06_Test_Doc/Test_Plan.md)。

## 3. 碰撞、输入与运行配置的来源

JamDevice/JamProjectile 对象通道及 JamWeaponTrace/JamGroundTrace 查询通道是后续计划，碰撞矩阵只维护在 [技术设计](../02_Design_Doc/TDD/Technical_Design.md)。输入门控、物理组件和生命周期也以技术设计为准；本次未向 Config 写入这些设置。

运行时设计数值来自 DataAsset/默认配置，设计账本见 [玩法参数基线](../02_Design_Doc/GDD/Gameplay_Parameters.json)，该 JSON 不承诺自动导入。主地图及 UI/DataAsset 引用完成后，按 [发布检查清单](../07_Release/Release_Checklist.md) 重新核对候选包包含的运行资源。

## 4. 公开仓库的 Android File Server 配置

当前 Windows 单机目标不使用 Android 文件服务器。在 DefaultEngine.ini 的 AndroidFileServerRuntimeSettings 节中，已设 bEnablePlugin=False、bAllowNetworkConnection=False、SecurityToken=（显式空值）；bIncludeInShipping、bAllowExternalStartInShipping、bCompileAFSProject 均保留 False。未修改 .uproject 或该配置节之外的引擎设置。

空 Token 会关闭令牌认证检查，因此必须与禁用该服务器用途同时处理。以后需要 Android 开发时，另行配置私有 Token 与部署方式。当前仅完成静态配置核对，没有编译/打包结果。[Epic：Android File Server](https://dev.epicgames.com/documentation/en-us/unreal-engine/android-file-server-for-unreal-engine)
