# motheffect：开发文档入口

版本 v0.6 · 2026-10-03 · 单人 UE 5.8 C++ 与蓝图混合开发

本目录按用户建立的开发阶段结构归档。文件使用稳定的英文语义名称，正文为中文；v0.3 整理结构，v0.4 配置版本管理，v0.5 清理个人路径与发布配置，v0.6 将 Content 的 Git 范围收敛为 MothEffect 子目录。各文件保留自身最近更新版本；文档源文件是 Markdown/JSON，[浏览器阅读版](01_Project_Overview/Reading_View.html) 是汇总快照。

工程：[MothEffect.uproject](../MothEffect.uproject) · 显示名：motheffect · 风格：魔法朋克。第三人称模板工程已存在；自定义玩法、编译、独立包、测试与发布均待验收。已配置本地 Git/origin/忽略/LFS；本轮未暂存、提交或推送，详见 [版本管理与公开建议](03_Code_Standard/Version_Control.md)。

## 1. 目录与有效文件

| 目录 | 文件 | 职责 |
|---|---|---|
| 01_Project_Overview/ | [Project_Overview.md](01_Project_Overview/Project_Overview.md) | 项目概览 |
| 01_Project_Overview/ | [Reading_View.html](01_Project_Overview/Reading_View.html) | 浏览器阅读版 |
| 02_Design_Doc/GDD/ | [Game_Design.md](02_Design_Doc/GDD/Game_Design.md) | 游戏策划案 |
| 02_Design_Doc/GDD/ | [Device_Interaction_Rules.md](02_Design_Doc/GDD/Device_Interaction_Rules.md) | 道具与交互规则 |
| 02_Design_Doc/GDD/ | [Level_UI_Asset_Specification.md](02_Design_Doc/GDD/Level_UI_Asset_Specification.md) | 关卡、界面与资源规格 |
| 02_Design_Doc/GDD/ | [Gameplay_Parameters.json](02_Design_Doc/GDD/Gameplay_Parameters.json) | 玩法参数基线 |
| 02_Design_Doc/TDD/ | [Technical_Design.md](02_Design_Doc/TDD/Technical_Design.md) | 技术设计 |
| 02_Design_Doc/TDD/Decisions/ | [Design_Decisions.md](02_Design_Doc/TDD/Decisions/Design_Decisions.md) | 设计决策 |
| 03_Code_Standard/ | [Coding_Conventions.md](03_Code_Standard/Coding_Conventions.md) | 代码与资产命名规范 |
| 03_Code_Standard/ | [Version_Control.md](03_Code_Standard/Version_Control.md) | 版本管理规范 |
| 04_Engine_Config/ | [UE_Project_Configuration.md](04_Engine_Config/UE_Project_Configuration.md) | UE 工程配置 |
| 05_Development_Guide/ | [Development_Plan.md](05_Development_Guide/Development_Plan.md) | 开发任务与排期 |
| 05_Development_Guide/ | [Documentation_Workflow.md](05_Development_Guide/Documentation_Workflow.md) | 文档维护与变更流程 |
| 05_Development_Guide/ | [Changelog.md](05_Development_Guide/Changelog.md) | 变更日志 |
| 06_Test_Doc/ | [Test_Plan.md](06_Test_Doc/Test_Plan.md) | 测试计划与验收 |
| 07_Release/ | [Release_Checklist.md](07_Release/Release_Checklist.md) | 发布检查清单 |
| Assets/UML/ | 当前为空 | 后续实际产出的 UML 图及源文件 |

GDD = 游戏设计文档，TDD = 技术设计文档。玩法数值归属 GDD，UE 配置归属 Engine_Config。决策依据与变更历史分开维护；测试与发布也各自独立。

## 2. 按开发流程阅读

1. **目标与范围**：[项目概览](01_Project_Overview/Project_Overview.md) → [游戏策划案](02_Design_Doc/GDD/Game_Design.md)，明确本次要交付的体验和 P0/P1/P2。
2. **行为与内容**：[道具与交互规则](02_Design_Doc/GDD/Device_Interaction_Rules.md) + [玩法参数基线](02_Design_Doc/GDD/Gameplay_Parameters.json) + [关卡、界面与资源规格](02_Design_Doc/GDD/Level_UI_Asset_Specification.md)，明确状态、边界、波次与资源。
3. **技术与环境**：[技术设计](02_Design_Doc/TDD/Technical_Design.md) + [代码与资产命名规范](03_Code_Standard/Coding_Conventions.md) + [版本管理规范](03_Code_Standard/Version_Control.md) + [UE 工程配置](04_Engine_Config/UE_Project_Configuration.md)。
4. **执行与验证**：按 [开发任务与排期](05_Development_Guide/Development_Plan.md) 的 G0–G4 推进，用 [测试计划与验收](06_Test_Doc/Test_Plan.md) 留下实际证据。
5. **变更与发布**：改动遵循 [文档维护与变更流程](05_Development_Guide/Documentation_Workflow.md)，更新 [设计决策](02_Design_Doc/TDD/Decisions/Design_Decisions.md) 与 [变更日志](05_Development_Guide/Changelog.md)，提交时执行 [发布检查清单](07_Release/Release_Checklist.md)。

## 3. 维护约定

目录阶段编号保持现状；文件名用英文语义名称和下划线，不重复加另一套顺序编号。REQ/R/D/T/G/TC/DEC/CHG/RISK 等 ID 不随文件迁移改变。

交互、接口、内容、数值各保留唯一来源，具体分工见 [文档维护与变更流程](05_Development_Guide/Documentation_Workflow.md)。建议基线和待原型验证仍需实测；参数账本不是可直接导入 UE 的 DataTable。T02 保留工程已建立但运行打包待验收，T03 已配置版本管理但提交与恢复待验收；其余任务/测试状态未变。

修改源文件后同步更新版本、相对链接和阅读快照；不要只改 HTML。原始平铺文件已在工程外备份，避免在 Docs 中并存两套有效文件。
