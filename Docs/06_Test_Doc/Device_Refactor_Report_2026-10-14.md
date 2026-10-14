# Moth Effect（飞蛾效应）：道具基础重构验证报告

版本 v0.1 · 2026-10-14 · 香港时间（UTC+8）· Refs #48/#49/#50/#51

代码基线 `4934627`，当前 `refactor` 分支分段提交。已实现地面固定/无实体碰撞与可调尺寸、D03 水平射向、三机关共用激活延迟及爆炸激活连锁。隔离 UE 5.8 Editor 构建通过，最终 **114 项 Editor、12 项实际主地图 Game 自动化全部 Success**。随后检测到编辑器已关闭，原工程常规 Editor 构建 56.79 秒通过，原 DLL 六项 Foundation 再次全部通过；当前硬件 PIE、手感/尺寸调优、多倍率保存重载及独立包仍待验收。

## 1. 实现与提交

| 提交 | 改动与验证阶段 |
|---|---|
| f6d6b2a | 地面固定、显式外力唤醒、独立尺寸、XY 射向与默认 0.3 秒延迟；先完成静态检查 |
| f91db81 | 爆炸触发 Dormant、下游新 HitId、尺寸倍率保存与 BP CDO 出生半径 |
| 0e5370a | 新增六项 Foundation、适配既有专项；修复首轮发现的会话关闭未立即取消等待和落地物理选项告警 |
| 4934627 | 修复无碰撞道具重叠掉落/拾取问题；净空单独检查已有固定道具实际体积；修正阴性布局位于爆炸范围边界与重力夹具 |

四项 Issue 保持 Open / In Progress，不用合并/原生测试替代用户完整验收。配置和操作见 [Device_Refactor_Setup](../05_Development_Guide/Device_Refactor_Setup.md)，现行 [规则](../02_Design_Doc/GDD/Device_Interaction_Rules.md)、[参数 v0.19](../02_Design_Doc/GDD/Gameplay_Parameters.json) 与 [测试计划](Test_Plan.md) 已同步。

## 2. 验证环境与边界

原编辑器运行期间使用独立临时工程，Source/Config/uproject 来自已提交代码，Content 以目录 junction 引用本机完整内容；Binaries/Intermediate/Saved 独立。最终 180 个源码/配置文件与提交 `4934627` 对照，仅规范化 CRLF/LF 后比较，全部一致。用户已有 BP_MothPlayer、控制器 `.cpp/.h`、UI_Setup 及 Build/ 修改没有纳入本轮提交；并行控制器代码没有混入验证工程。没有保存或重写现有 UE 资产。隔离验证阶段没有替换正在运行的原 DLL；随后确认编辑器已关闭，对原工作区（含用户未提交的控制器改动）做常规 Editor 构建并重链新 DLL，六项 Foundation 在原项目再次通过。隔离全量与原工程专项的验证范围分别记录。

首次构建 281.61 秒，最后两件测试增量 16.10 秒；收尾修正增量 10.49 秒。随后 UE 检测到 cl.exe 变化，使用 MSVC 14.51.36260 / Windows SDK 10.0.26100.0 完整重编译，最后构建 108.76 秒 Succeeded。编译器非 preferred 与引擎/既有测试 C4996 告警按原始日志保留。

最终 DLL SHA256：`19088f65d568455e1fac442a6149a7964afaf13230ee63932e646bb1bc344a16`。摘要见 [构建/测试摘要](Evidence/Device_Refactor_Summary_2026-10-14.json)，详细原始结果见 [Editor JSON](Evidence/Device_Refactor_Editor_2026-10-14.json)、[Game JSON](Evidence/Device_Refactor_Game_2026-10-14.json)、[原工程专项 JSON](Evidence/Device_Refactor_Original_2026-10-14.json) 与 [原始日志摘录](Evidence/Device_Refactor_Verification_2026-10-14.log)。摘录只规范化行尾空白，完整运行日志保留于本机 Saved/Automation/DeviceFoundation。

## 3. 原生测试与失败修正

| 轮次 | 实际结果 | 说明 |
|---|---|---|
| 首轮 Foundation | 5 Success / 1 Fail | 关闭会话后等待标记/额度未立即释放；另外暴露物理关闭顺序产生 Invalid Simulate Options |
| 首轮全量 Editor | 111 Success / 3 Fail | 基础六项全通过；爆炸阴性布局恰在半径边界、重力被激活重设、无碰撞旧道具导致重叠掉落/拾取受阻 |
| 最终全量 Editor | 114 Success / 0 Fail / 0 NotRun | 171.04 秒；113 无告警、1 带告警；50 条 DesignerProbe 绑定探测告警，errors=0 |
| 实际主地图 Game | 12 Success / 0 Fail / 0 NotRun | 126.17 秒；D3D12 / 1920×1080 离屏及音频初始化；errors=0 / warnings=232 |
| 原工程 Foundation | 6 Success / 0 Fail / 0 NotRun | 原 Editor DLL 重链后再次运行，包含当前用户控制器源码；errors=0，1 条引擎网络探测超时；结果和 DLL 指纹见摘要 |

失败历史没有删除。净空修复保持物理根 NoCollision，仅生成/释放查询按实际球体避免重叠，不用较大射击球扩大出生占用。最终 Editor 未再出现本轮 Invalid Simulate Options。

新增六项覆盖：真实落地关闭模拟/碰撞、W01/P01 查询、角色穿行不推动、外力起飞与再次落地；独立 Body/Shot/Visual 尺寸与重复刷新；带俯仰的 XY 投影及竖直 +X 后备；三类型默认延迟、D03 0.3+0.1 首发及 3 秒工作期；真实暂停/恢复/取消/会话关闭；三枚普通炸弹逐次激活且同一 600HP 角色依次 540→480→420，独立 HitId 与各自 Spent 观察。

旧伤害/几何/计数专项显式设零延迟以隔离所测规则；保存蓝图集成、新 Foundation 及主地图保留真实默认延迟。TestExit 的进程退出码不是测试结论，最终结论来自导出的逐项状态与事件。

## 4. 主地图与待验收

12 项包含三项场地、原型外观、默认反馈、两项 HUD/教学、五项三波/界外/死亡/重开流程。实际十次 OpenLevel 重开无旧等待效果泄漏；教学与真实反馈适配新时序。232 条告警类别为导航 19、Rig/资源加载 19、蓝图旧输入 133、动画脚本 38、输入配置提示 7、粒子 Notify 16，完整保留。Startup 的 Murdock→ParagonShinbi 缺资源记录仍在日志中；不扩大为第三方依赖恢复完成。

原工程已在编辑器关闭后完成常规原生构建与专项复测，重新打开即可加载新模块；仍需用户核对实际 BP 输入/携带动画、地面与爆炸连锁手感、四向和空中射向、不同尺寸的保存重载。原工程并行控制器/UI修改须由其对应工作继续验证。独立 Windows 包、外部盲玩、长期密度/稳定性与完整门槛 G1–G4 保留待验收。
