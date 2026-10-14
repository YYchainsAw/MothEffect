# Moth Effect：T19 默认反馈与替换入口

版本 v0.2 · 2026-10-13 · 状态：默认反馈已接入并完成自动化；四组用户 PIE 待验收

打开 `/Game/MothEffect/Maps/L_Arena_01` 后 Play，进入教学或正式战斗即可使用默认反馈。已有 D01/D02/D03 蓝图继承原生 `DefaultFeedback` 组件，步枪、生命与弹丸从已提交的动作发出反馈；不需要重制玩家或机关 BP，也不需要给现有蓝图补声音事件。本轮提供可调整的原型，具体 UI 和美术由用户制作。

## 默认会看到、听到什么

| 动作/状态 | 默认反馈 | 判定范围 |
| --- | --- | --- |
| W01 实际开火 | 枪口短青色闪线、短促开火声 | 空弹匣、连射间隔内被拒绝的请求不发出新反馈；枪口受阻仍按实际消耗的一枪反馈 |
| 子弹碰到表面 | 命中点短白色闪线 | 墙面命中不显示成功命中标记；受阻枪口不会伪造远处命中 |
| 已接受伤害/机关启动 | 短命中声；本地玩家确认命中时准星周围显示白色 X | 拒绝或重复命中不重复反馈；伤害来源必须是本地玩家才显示确认标记 |
| 本地玩家受伤 | 短受伤声、屏幕四边红线 | 由真实生命提交触发；菜单不绘制默认边线 |
| D01 成功启动 | 青色向上轮廓、上升短线和升调声音 | 支撑检查失败时使用红色 X 和警告声，不伪装成功抬升 |
| D02 爆炸 | 橙色轮廓、扩张的水平分段环和低音爆破声 | 原规则为立即爆炸；圆环按本次实际爆炸半径显示，没有新增引信倒计时 |
| D03 启动/发射 | 紫色侧向轮廓、启动声、短脉冲声；Active 时显示世界方向箭头 | 箭头沿启动命中的世界方向锁定，随道具平移，不随物理滚动、镜头或投掷速度转向 |
| 敌人 P01 发射 | 红色短闪线、短脉冲声 | 复用脉冲音效槽，颜色区别于 D03 |
| Dormant / Active / Spent | 类型轮廓常亮 / 慢脉动 / 较小灰色 X | 颜色与形状、运动共同提示；Spent 按既有规则回收 |
| Dormant 过期警告 | 黄色快速脉动、短警告声；保留原有过期文字 | 每次进入本段 Dormant 警告只提示一次；Held 隐藏世界标记并暂停闲置计时，放下后按剩余时间继续 |

世界反馈使用实际静态网格和自有发光材质，不依赖 DrawDebug。网格没有碰撞、Overlap、导航影响或阴影，不改变射线、刚体、伤害、启动方向或规则计时。短闪线与圆环会自动结束；效果强度、尺寸和持续时间仍需在实际视角下确认不会遮挡下一次瞄准。

## 调整默认配置

1. 停止 PIE，打开 `/Game/MothEffect/Feedback/DA_DefaultFeedback`，修改并保存，再 Play 确认。
2. `Styles` 有十二个槽：`Shot`、`Impact`、`Hit`、`Damage`、`Launcher`、`Bomb`、`Emitter`、`EmitterShot`、`EnemyShot`、`Expiry`、`Spent`、`Failure`。每槽可设置颜色、声音、音量、持续时间、尺寸和声音最小间隔。`Impact` 与 `Spent` 默认无声音；同一槽的音频间隔只限声音，不抑制已经提交的视觉提示。
3. `Material` 默认为 `/Game/MothEffect/Feedback/M_DefaultFeedback`。替换材质保留向量参数 `Color` 和标量参数 `Intensity`，原生实例会更新这两个参数。`Attenuation` 默认为 `/Game/MothEffect/Feedback/SA_DefaultFeedback`，可调整距离衰减与空间化。
4. `Master Volume` 调整总体音量，`Emissive Strength` 调整发光强度；`Marker Height Cm`、`Marker Size Cm` 调整机关状态轮廓，`Direction Length Cm` 调整 D03 箭头长度。`Spent Color` / `Warning Color` 提供状态色。
5. `Hit Marker Seconds` / `Damage Edge Seconds` 调整屏幕反馈时长。`Max Visual Cues` / `Max Audio Voices` 默认 24 / 16，限制同时存活的瞬时效果和声音；超预算先结束较早的实例。机关持久状态轮廓属于机关组件，不计入瞬时效果预算。

| 开关/入口 | 用途 |
| --- | --- |
| Profile `bEnabled` | 关闭整套默认反馈，包括屏幕 Alpha |
| Profile `bWorldVisuals` | 关闭默认世界闪线和机关轮廓，保留声音与屏幕 Alpha |
| Profile `bAudio` | 关闭默认声音，保留视觉 |
| 单个机关 → `DefaultFeedback` → `bEnabled` | 关闭该机关的默认类型/状态轮廓与 D03 箭头；启动声音和瞬时效果仍由全局 Profile 控制 |
| `WBP_MothHUD` Class Defaults → Moth / Feedback → `bDrawDefaultCombatFeedback` | 关闭原生命中 X 与受伤边线，便于自制 UMG；不影响世界反馈与 Alpha |

若要复制成自己的 Profile，在 [DefaultGame.ini](../../Config/DefaultGame.ini) 的 `[/Script/MothEffect.MothFeedbackSubsystem]` 修改 `DefaultProfile` 路径，并将新资产所在目录纳入 Cook。默认目录 `/Game/MothEffect/Feedback` 已配置为 Always Cook。反馈参数不会从 [Gameplay_Parameters.json](../02_Design_Doc/GDD/Gameplay_Parameters.json) 导入；战斗规则继续归原有类与配置管理。

## 你制作 UI 的接口

现有 [T18 UI 编辑入口](UI_Setup.md) 的控件树和必需绑定保持原用途，本轮不增加必需 Designer 控件。原生命中 X 使用现有 `CrosshairText` 实际几何中心；玩家受伤时只画屏幕边线。

文字没有有效字体来源时，原生预构造会补默认字体来源，避免保存的共享字体重载后出现方框；已有有效字体、字号、描边、颜色与布局均保留。你在 Designer 指定自己的 Font Asset 后直接使用该字体，无需改代码。

自制 UMG 时关闭 `bDrawDefaultCombatFeedback`，从当前世界取得 `MothFeedbackSubsystem`：

- `Get Hit Marker Alpha`：0–1 的命中确认强度。
- `Get Damage Edge Alpha`：0–1 的受伤强度。
- `Get Profile`：读取当前反馈配置。

Alpha 基于当前世界时间，暂停时不会继续衰减；结算、重置和世界结束后清零。将 Alpha 用于自己的图片透明度、准星动画或受伤遮罩即可。实现入口见 [MothFeedbackSubsystem.h](../../Source/MothEffect/Public/Feedback/MothFeedbackSubsystem.h)、[MothHUDWidget.cpp](../../Source/MothEffect/Private/UI/MothHUDWidget.cpp)；不要从视觉 Alpha 反推或改写生命与机关状态。

## 暂停、结算与排查

暂停期间拒绝新增反馈，短效果寿命、轮廓脉动和屏幕 Alpha 随世界时间冻结；存活声音组件同步暂停。继续后沿剩余时间结束。玩法关闭、管理对象重置以及 World EndPlay 清理短效果与声音，隐藏机关轮廓并清零屏幕反馈，避免上一轮效果留到结果页或重开。

PIE 控制台输入 `MothFeedbackStats`，在 Output Log 查看：

- `profile`、当前 `visuals` / `voices`、`hitAlpha` / `damageAlpha`。
- 十二种 `cue` 的 `committed` 累计计数。此计数归当前世界，清理反馈不会将历史动作数归零；核对是否重复触发时比较操作前后的差值。

测试无效果时先确认已进入教学/战斗、Profile 和组件开关开启、音频设备可用、资产路径有效。声音与视觉的实际观感需用正常渲染和真实输出设备检查；NullRHI / `-nosound` 的自动化只验证逻辑。

## 资产来源与重建

本轮保存十一件自有资产：一个 Profile、一个材质、一个衰减、八个 SoundWave。八个声音使用 Python 标准库的振荡器、固定随机种子噪声和包络合成，没有录音、下载或第三方样本；来源与 SHA256 见 [AudioManifest.json](../../SourceArt/Feedback/AudioManifest.json)。源 WAV 为 48kHz、单声道、16-bit PCM，时长 0.07–0.36s，峰值 0.7。

- [generate_t19_audio.py](../../scripts/generate_t19_audio.py) 重现八个源 WAV 和账本；会重写 `SourceArt/Feedback` 的原始生成文件，修改过源声音时先另存。
- C++ 构建完成后，通过 Editor Python 执行 [create_t19_feedback.py](../../scripts/create_t19_feedback.py)，只创建缺失的反馈资产，校验已有类型与十二个槽。已有材质、音效、衰减和 Profile 不覆盖，不调用 SaveAll，不保存玩家/机关/HUD 蓝图。

## 当前用户 PIE 验收

以下四组均**待验收**，用当前 `L_Arena_01`、实际玩家和正常声音输出测试。完整 TC 仍按 [测试计划](../06_Test_Doc/Test_Plan.md) 与 [T19 报告](../06_Test_Doc/T19_Report_2026-10-13.md) 的实际范围记录。

1. **开火、命中与三机关**：开火打墙、角色和机关，空弹匣与贴墙受阻分别检查；墙面只出现碰撞提示，真实伤害/启动出现命中 X，拒绝请求不重复。触发 D01/D02/D03，分辨升调、爆破和脉冲声音，以及上箭头、菱形和侧箭头；D01 支撑失败可见失败提示。受伤见短边线。所有提示结束及时，不挡下一次瞄准。
2. **TC03 / TC04 方向与状态**：地面 D03 从前后左右分别启动，箭头与实际发射方向一致；启动后绕行并转镜头，方向不追踪玩家。投出 D03，在上升、顶点附近、下降三个阶段分别启动，故意让命中方向与投掷速度不同；道具下落/滚动时箭头随位置移动，朝向与 P01 仍沿命中方向。观察 Active 慢脉动、Spent 灰色 X 和回收。
3. **TC18 过期与 Held**：让 Dormant 道具进入最后警告阶段，确认黄脉动、文字与一次短声音；警告时拾取并持有超过原剩余时间，不消失也不反复鸣响。E 放下，再次警告并沿剩余时间过期；另一次用投掷恢复计时。检查过期后 Spent/回收及无残留轮廓。
4. **暂停、结算与重开**：开火或机关提示出现时暂停，等待超过音效/短效果时长；世界与声音不继续，恢复后正常结束。分别在失败、胜利、教学重置/结束、返回标题和重新开战后检查旧声音、闪线、方向/状态标记和屏幕 Alpha 已清理。多次重开无重复音效；必要时以 `MothFeedbackStats` 核对存活实例与动作计数差值。

四组确认后才更新 [#21](https://github.com/YYchainsAw/MothEffect/issues/21) 的用户验收。自动化不能代替真实四向/空中操作、听感、遮挡和当前硬件的暂停/重开体验。
