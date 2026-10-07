# Moth Effect（飞蛾效应）

> 开发中 · 魔法朋克 · 第三人称越肩射击 · 单人开发

**Moth Effect（飞蛾效应）** 是一款正在开发中的魔法朋克风格第三人称越肩射击游戏。玩家在大型战斗场地中击败敌人，利用敌人留下的机关，通过拾取、投掷和射击激活，组合出改变战场的连锁反应。

本作参加 **TapTap 聚光灯 GameJam 2026（21 天游戏创作挑战）**，围绕本届主题 **「涌现」** 展开设计。参见 [官方主题公告](https://www.taptap.cn/moment/854692229854265392?group_id=792849)。

## 核心玩法

1. **射击与掉落**：以越肩视角与敌人战斗，敌人死亡后在原地留下不同的机关道具。
2. **拾取与投掷**：把机关投向合适的位置，调整战场布局。
3. **射击激活**：射击地面或空中的机关，启动各自的效果；投掷后也可以在空中击中并激活它们。
4. **组合与连锁**：利用机关的位置、运动、发射方向和激活时机，让简单规则产生更多战斗结果。

目前设计的三类核心机关：

| 机关 | 激活效果 |
| --- | --- |
| 弹射装置 | 从激活位置下方的地面升起柱体，弹起角色和可移动机关 |
| 爆炸装置 | 在当前位置产生范围伤害与推力，也可以在空中引爆 |
| 定向发射装置 | 沿激活子弹的飞行方向持续发射弹丸一段时间 |

例如，弹射装置把爆炸机关抬到空中，发射装置的弹丸再击中它，引发空中爆炸。机关之间的交互是本作对「涌现」的玩法表达。

以上为当前设计方向，具体玩法、手感与表现会随原型验证和开发持续调整。

## 技术栈

| 类别 | 技术 |
| --- | --- |
| 游戏引擎 | Unreal Engine 5.8 |
| 开发方式 | C++ 与蓝图（Blueprints）混合开发 |
| 输入系统 | Enhanced Input |
| 界面工具 | UMG |
| 版本管理 | Git + Git LFS（管理 `.uasset` / `.umap`） |

工程基于 UE 第三人称模板，当前计划面向 **Windows PC** 制作单机体验。

## 项目文档

- [开发文档入口](Docs/README.md)
- [游戏策划案](Docs/02_Design_Doc/GDD/Game_Design.md)
- [道具与交互规则](Docs/02_Design_Doc/GDD/Device_Interaction_Rules.md)
- [技术设计](Docs/02_Design_Doc/TDD/Technical_Design.md)
- [开发计划](Docs/05_Development_Guide/Development_Plan.md)
- [10/7 进度核查：已有实现、待验收与 UI/美术安排](Docs/05_Development_Guide/Progress_Audit_2026-10-07.md)
- [GitHub 开发流程：任务、看板、PR 与 Actions](Docs/05_Development_Guide/GitHub_Workflow.md)

工程入口为 [MothEffect.uproject](MothEffect.uproject)。当前仓库仅纳入 `Content/MothEffect/` 下的自有内容；模板资源保留在开发者本机，现有默认地图与游戏模式仍引用 `ThirdPerson`。克隆后需要补齐这些依赖，才能复现本地模板工程，详见 [版本管理说明](Docs/03_Code_Standard/Version_Control.md)。

## 关注开发

[我的 TapTap 个人主页](https://www.taptap.cn/user/756995379)

欢迎关注项目后续的开发进展，交流玩法想法与试玩反馈。
