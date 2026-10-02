# motheffect：文档维护与变更流程

版本 v0.3 · 2026-10-02 · 单人 UE 5.8 C++ 与蓝图混合开发

本文件规定文档归属、命名与变更顺序，用来避免多个文件维护不同版本的规则。目录入口见 [文档入口](../README.md)；设计依据见 [设计决策](../02_Design_Doc/TDD/Decisions/Design_Decisions.md)，变更历史见 [变更日志](Changelog.md)。

## 1. 唯一来源与状态

玩法目标和优先级以 [游戏策划案](../02_Design_Doc/GDD/Game_Design.md) 为准；交互行为以 [道具与交互规则](../02_Design_Doc/GDD/Device_Interaction_Rules.md) 为准；类接口与碰撞以 [技术设计](../02_Design_Doc/TDD/Technical_Design.md) 为准；场地、波次和资源需求以 [关卡、界面与资源规格](../02_Design_Doc/GDD/Level_UI_Asset_Specification.md) 为准；数值只维护在 [玩法参数基线](../02_Design_Doc/GDD/Gameplay_Parameters.json)。其他文件引用这些来源，避免复制第二份参数表或状态机。

每份文档保留标题、版本、日期及实际状态。用户已确认、建议基线、待原型验证和已实测分别记录；未执行的任务/测试不能填写通过。REQ、R、D、T、G、TC、DEC、CHG、RISK 等现有 ID 保持稳定，重命名文件不重排 ID。

## 2. 命名与归档

保留已有阶段目录编号；文件使用英文语义名称和下划线，扩展名小写，例如 Technical_Design.md。目录已经表示阶段，文件不再附加另一套顺序编号。README.md 为根目录入口，正文继续使用中文。

GDD 放玩法设计和玩法参数；TDD 放实现设计；Decisions 放设计决策；Code_Standard 放源码/资产约定；Engine_Config 放 UE 配置；Development_Guide 放任务和变更流程；Test_Doc 放测试；Release 放打包投稿检查。Assets/UML 供后续实际产出的图示使用，当前保留空目录，不创建占位图。

本轮保留综合决策账本，避免单人 Jam 为每个决策另建空模板；较大技术决策后续可按 ADR_序号_主题.md 独立记录，并从决策账本链接。

## 3. 修改规则的工作流程

先复现问题并写清症状，再判断是参数、表现、规则还是范围变化。

参数变化：更新 [玩法参数基线](../02_Design_Doc/GDD/Gameplay_Parameters.json) 中对应字段，更新版本和调参理由，重跑受影响的 TC。相机数值及道具大小变动属于此类。

规则变化：先更新 [设计决策](../02_Design_Doc/TDD/Decisions/Design_Decisions.md) 与 [变更日志](Changelog.md)，再更新 [道具与交互规则](../02_Design_Doc/GDD/Device_Interaction_Rules.md)、相关 [技术设计](../02_Design_Doc/TDD/Technical_Design.md) 实现及 [测试计划与验收](../06_Test_Doc/Test_Plan.md) 测试。例子是爆炸也能启动道具、激活发射器能重新拾取、空中柱子改为平台。这些变化不能只落在单个蓝图里。

范围变化：更新 [游戏策划案](../02_Design_Doc/GDD/Game_Design.md) 优先级、[关卡、界面与资源规格](../02_Design_Doc/GDD/Level_UI_Asset_Specification.md) 内容和 [开发任务与排期](Development_Plan.md) 工时。增加一种敌人前先说明挤占的工作与对应验收项。

日常修复：只改变实现使其符合已有规则，记录缺陷及修复版本，不必重写策划案。

## 4. 阅读快照与证据

先修改对应 Markdown/JSON 源文件，再同步版本和日期、修正文档相对链接，并重新生成 [浏览器阅读版](../01_Project_Overview/Reading_View.html)。阅读版只用于浏览，不能单独修改为与源文件冲突的另一套设计。

任务、规则或测试变更时保留关联 ID；实际构建/缺陷证据按 [测试计划与验收](../06_Test_Doc/Test_Plan.md) 记录。发布状态按 [发布检查清单](../07_Release/Release_Checklist.md) 记录，上传成功不能替代审核通过。
