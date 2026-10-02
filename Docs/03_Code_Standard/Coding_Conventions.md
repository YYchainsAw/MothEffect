# motheffect：代码与资产命名规范

版本 v0.3 · 2026-10-02 · 单人 UE 5.8 C++ 与蓝图混合开发

本文件约定后续代码与资产的命名、目录和职责边界。当前只整理文档，未执行源码重命名或资源迁移。自定义类/接口签名以 [技术设计](../02_Design_Doc/TDD/Technical_Design.md) 为准。

## 1. 命名与目录

新增资产统一放 `/Game/MothEffect/{Core,Player,Combat,Devices,Enemies,Levels,UI,Data,Audio,VFX,Tests}`，新增源码放在现有 Source/MothEffect 模块内。现有 ThirdPerson、Characters、Input、LevelPrototyping 与模板变体资产保持原路径；角色、控制器和游戏模式复用 [技术设计](../02_Design_Doc/TDD/Technical_Design.md) 类映射表中的现有类及蓝图，不先做批量重命名。使用英文稳定名称：C++ 无 BP 前缀，新增蓝图 `BP_`，地图 `L_`，界面 `WBP_`，输入 `IA_/IMC_`，数据 `DA_`。必要的资源整理安排在 G1 后核对依赖再进行。

## 2. C++ 与蓝图职责

C++ 负责玩法状态、命中、伤害、推力、定时器与容量；蓝图负责组件组装、相机配置、界面及视听表现，调用既定 C++ 入口。伤害与激活流程只保留一套，具体类职责见 [技术设计](../02_Design_Doc/TDD/Technical_Design.md)。机关行为以 [道具与交互规则](../02_Design_Doc/GDD/Device_Interaction_Rules.md) 为准，数值以 [玩法参数基线](../02_Design_Doc/GDD/Gameplay_Parameters.json) 为准，不在 Tick、关卡脚本或特效中另藏伤害数值。

## 3. 构建与调试

新建 UCLASS、改反射字段或继承关系后关闭编辑器做完整编译再打开，避免把 Live Coding 状态当作干净构建成功。


日志记录 HitId、状态前后、方向、登记数和构建版本；开发调试线/日志受 Development 开关控制，提交包关闭。工具链与首包验收见 [UE 工程配置](../04_Engine_Config/UE_Project_Configuration.md)，提交与恢复见 [版本管理规范](Version_Control.md)。
