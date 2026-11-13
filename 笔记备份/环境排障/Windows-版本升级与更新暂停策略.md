---
date: 2026-11-09
tags:
  - 指南
  - 工具
---

# Windows 版本升级与更新暂停策略

> [!note] 一句话
> 本机 2026-11-09 实测为 **Windows 11 25H2 / Build 26200.8457**（2026-05-12 的补丁），Windows 更新被本人手动暂停到 **2036-05-18**，落后 5 个补丁日。
> 决定：**11/14（周六）一次性升级到最新版 26H2，观察 3–7 天稳定后重新暂停**，不再「永久冻结」。

## 一、先搞懂三个词：版本、Build、UBR

这三个词经常混着说，其实分工很明确：

| 词 | 是什么 | 怎么读 | 例子 |
|---|---|---|---|
| 版本（25H2 / 26H2） | 微软每年一次的大版本，官方叫「功能更新」 | 25H2 = 2025 年下半年；26H2 = 2026 年下半年 | 本机是 25H2 |
| Build | 大版本对应的内部编号，打完补丁也不变 | 25H2 = 26200；26H2 = 26300；24H2 = 26100 | 本机 26200 |
| UBR / 小版本 | 每月补丁在同一个 Build 上累加的数字 | 后半段越大 = 补丁越新 | 本机 `.8457` |

> [!tip] 比喻
> **Build 是楼层号，UBR 是这一层装修到第几版。**
> 想换楼层 = 装功能更新（25H2 → 26H2）；只在同层换装修 = 装每月补丁（26200.8457 → 26200.9818）。

一个小坑：注册表里的 `ProductName` 在 Win11 上仍然写着 `Windows 10 Home China`，这是微软的历史遗留字段，**不代表你装的是 Win10**。判断版本要看 `DisplayVersion`（25H2）和 `CurrentBuild`（26200）。

## 二、本机现状（2026-11-09 实测）

| 项目 | 值 |
|---|---|
| 系统 | Windows 11 家庭中文版（Home China，SKU 代码 `CoreCountrySpecific`） |
| 版本 / Build | 25H2 / 26200.8457 |
| 这个 build 对应 | 2026-05-12 安全更新 KB5089549 |
| 25H2 当前最新 | 26200.9818（2026-10-28，可选预览）；安全基线 26200.9711（2026-10-13） |
| 最新大版本 | 26H2 / Build 26300（2026-09-29 发布） |
| 更新状态 | 暂停至 2036-05-18（2026-05-31 设置） |
| 落后 | 5 个补丁日：2026-06 / 07 / 08 / 09 / 10 |
| 硬件余量 | Ultra 5 225H、31.5 GB 内存、C: 剩 118.9 GB（升级毫无压力） |

### 复查命令（只读，随时可跑）

```powershell
# 1) 最直观：弹出「关于 Windows」窗口，看版本 + Build
winver

# 2) 读注册表：DisplayVersion = 版本名，CurrentBuild = Build，UBR = 补丁号
Get-ItemProperty 'HKLM:\SOFTWARE\Microsoft\Windows NT\CurrentVersion' |
  Select-Object DisplayVersion,CurrentBuild,UBR

# 3) 看最近装过的补丁（按安装时间倒序）
Get-HotFix | Sort-Object InstalledOn -Descending | Select-Object -First 5

# 4) 看更新是不是被暂停了（Pause* 有值 = 暂停中）
$k = 'HKLM:\SOFTWARE\Microsoft\WindowsUpdate\UX\Settings'
Get-ItemProperty $k |
  Select-Object PauseFeatureUpdatesStartTime,PauseFeatureUpdatesEndTime,
                PauseQualityUpdatesStartTime,PauseQualityUpdatesEndTime,PauseUpdatesExpiryTime
```

## 三、为什么「暂停更新」不等于「安全」

更新是分通道的。暂停 Windows 更新只冻结了其中一层：

| 通道 | 暂停后是否还更新 | 说明 |
|---|---|---|
| Store 应用、Edge / Chrome、VS Code | ✅ 照常 | 各自独立的更新器 |
| Defender 病毒库、卡巴斯基病毒库 | ✅ 照常 | 走自己的更新通道（本机实测签名龄 0 天） |
| WSL 内核 / 发行版 | ✅ 照常 | `wsl --update` |
| Windows 内核、SMB、RDP、打印后台、Win32k | ❌ 冻结 | 正好是**提权与远程攻击面**那一层 |

所以真正被放弃的不是「所有安全」，而是**「被攻破之后能不能提权、能不能驻留」**这最后一环：浏览器本身是最新的，但利用链的提权环节要靠 OS 补丁补。

> [!warning] 一个常见误解
> 「我现在很稳定，所以不用更新」——你现在的稳定，是**打到 2026-05 为止的补丁**换来的，不是「不打补丁」换来的。
> 「稳定优先」的正确形态是**少更、滞后更**（观察一两个月再装），而不是**永不更**。

另外，25H2 家庭版的支持终点是 **2027-10-12**。作为对照：24H2 家庭版在 **2026-10-13** 就已经停止支持了——一直不升级，最终会落到「版本本身都没人修」的状态。

## 四、决策（2026-11-09）

| 项 | 内容 |
|---|---|
| 做什么 | 一次性升级到最新版 **26H2（Build 26300）** |
| 何时 | **11/14（周六）**，不与周复盘（11/15 21:00）撞车 |
| 前提 | 升级前查 [Windows release health](https://learn.microsoft.com/windows/release-health/) 与 26H2 当月更新说明，**无未解决的严重问题**才动手 |
| 为什么可行 | 26H2 与 25H2 同一服务分支，只是一个**启用包**（enablement package）：下载小、一次重启、不动文件和 WSL |
| 观察期 | 升级后 **3–7 天**：无蓝屏、无睡眠/休眠异常、USB 直通（ST-Link）正常、WSL 正常 |
| 回退触发条件 | 出现反复蓝屏 / 无法睡眠 / WSL 起不来且 `wsl --update` 无效 → 用「设置 → 系统 → 恢复 → 返回」或等修复补丁 |
| 之后 | 观察通过就**重新暂停更新**（见第六节命令） |
| 检查点 | **2027-10-12**（25H2 支持终点）前再评估一次；若届时仍在 26H2，支持到 2028-10-10 |

## 五、周末升级操作步骤

### 0. 升级前 3 件事

1. **插电**（笔记本别用电池），C: 留 ≥ 20 GB（本机 118 GB，够）。
2. **建还原点**（可选但推荐）：设置 → 系统 → 关于 → 系统保护 → 创建。
3. **备份关键配置**：记一下 `~/.ssh/config` 和密钥的位置（升级不会删 WSL 文件，但习惯要养成）。

### 1. 先关掉 WSL

```bash
wsl --shutdown
```

> 为什么要关：升级要重启，WSL 的虚拟磁盘（vhdx）如果还挂着，容易出现「升级后 WSL 起不来 / 磁盘需要修复」。关掉最省事。

### 2. 解除暂停（管理员 PowerShell）

```powershell
$k = 'HKLM:\SOFTWARE\Microsoft\WindowsUpdate\UX\Settings'
'PauseFeatureUpdatesStartTime','PauseFeatureUpdatesEndTime',
'PauseQualityUpdatesStartTime','PauseQualityUpdatesEndTime',
'PauseUpdatesExpiryTime' |
  ForEach-Object { Remove-ItemProperty -Path $k -Name $_ -ErrorAction SilentlyContinue }

Restart-Service wuauserv -ErrorAction SilentlyContinue
Start-Process ms-settings:windowsupdate
```

> 逐句解释：
> - `Remove-ItemProperty`：把 5 个 `Pause*` 注册表值删掉，等于「取消暂停」。`-ErrorAction SilentlyContinue` 是「不存在就跳过，别报错」。
> - `Restart-Service wuauserv`：重启 Windows Update 服务，让新状态立刻生效。
> - `Start-Process ms-settings:windowsupdate`：直接打开更新设置页。

### 3. 装 26H2

在设置页点「检查更新」→ 出现 **Windows 11 版本 26H2** 就点下载安装 → 重启。
如果没被推送（微软是分批放量），用 **Windows 11 安装助手**或 26H2 的 ISO 跑 `setup.exe` 做就地升级（保留文件和程序），效果一样。

### 4. 验证

```powershell
# 期望：CurrentBuild = 26300，UBR ≥ 9711（理想是 9818）
Get-ItemProperty 'HKLM:\SOFTWARE\Microsoft\Windows NT\CurrentVersion' |
  Select-Object DisplayVersion,CurrentBuild,UBR

# 顺便看有没有异常关机/蓝屏记录（近 7 天）
Get-WinEvent -FilterHashtable @{LogName='System'; Id=41,1001,6008; StartTime=(Get-Date).AddDays(-7)} -ErrorAction SilentlyContinue |
  Select-Object TimeCreated,Id,ProviderName
```

> 事件 ID 说明：`41` = 非正常断电/死机重启，`1001` = 蓝屏（BugCheck），`6008` = 上次关机是意外的。查不到记录 = 这 7 天没有异常。

### 5. 观察 3–7 天后再暂停

确认没毛病，跑第六节的重新暂停命令。

## 六、重新暂停更新（观察期结束后）

```powershell
$k = 'HKLM:\SOFTWARE\Microsoft\WindowsUpdate\UX\Settings'
$start = (Get-Date).ToUniversalTime().ToString('yyyy-MM-ddTHH:mm:ssZ')
$end   = '2036-05-18T06:03:12Z'   # 沿用原来的暂停终点；也可改成你想要的日期

Set-ItemProperty $k -Name PauseFeatureUpdatesStartTime -Value $start
Set-ItemProperty $k -Name PauseFeatureUpdatesEndTime   -Value $end
Set-ItemProperty $k -Name PauseQualityUpdatesStartTime -Value $start
Set-ItemProperty $k -Name PauseQualityUpdatesEndTime   -Value $end
Set-ItemProperty $k -Name PauseUpdatesExpiryTime       -Value $end

Restart-Service wuauserv -ErrorAction SilentlyContinue
```

> [!caution] 这是非官方做法
> 设置界面只允许暂停最多 5 周，写注册表才能设到 2036。副作用是「设置页可能显示状态不一致」，属正常。**Windows 大版本升级后这条暂停通常会被清掉**，所以升级完要复查一遍，没清才需要重设。

## 七、备选方案：完全不解除暂停，手动挑补丁

如果哪次不想解除暂停，也可以只装某个月的补丁：

1. 去 [Microsoft Update Catalog](https://www.catalog.update.microsoft.com/Search.aspx?q=Windows+11+25H2+cumulative+update+x64) 搜对应月份的累积更新（LCU）。
2. 下载 `.msu` 后双击安装（或 `wusa.exe 文件.msu`）。
3. **手动安装不会解除暂停**，装完 WU 还是暂停状态。

挑选原则：
- 只装 **B 版**（每月第二个周二的安全更新），跳过 **D 版**（月底的可选预览）。
- 累积更新是「累积」的，可以跳跃安装，不用一个月一个月补。
- 例：现在要追平 25H2，下 **KB5127936**（26200.9818）或保守点用 **KB5127648**（26200.9711）。

## 八、可能的坑与解法

| 现象 | 原因 | 解法 |
|---|---|---|
| 检查更新报 `0x800xxx` | 长期暂停后组件状态错乱 | `DISM /Online /Cleanup-Image /RestoreHealth` → `sfc /scannow` → 重试 |
| 解除了暂停也搜不到 26H2 | 微软分批放量，还没轮到 | 等几天，或用安装助手 / ISO 就地升级 |
| 升级后 WSL 起不来 | vhdx 状态异常 | `wsl --shutdown` 再开；仍不行 `wsl --update` |
| ST-Link 直通失效 | 功能更新后 USB 设备需要重新挂载 | 在 Windows 侧重新 `usbipd attach` |
| 升级后无法睡眠 / 休眠变成睡眠 | 个别机型的 S3/休眠问题 | 先观察；持续存在就用「返回上一版本」 |
| 设置页显示暂停状态混乱 | 手写注册表的副作用 | 用第六节命令重写一遍端点日期 |

> [!note] 家庭中文版的限制（与本主题无关但顺带记）
> Home 版不能加域、没有 `gpedit.msc`、没有 Hyper-V 角色（WSL2 用的是「虚拟机平台」，不受影响）。升级到 26H2 不会改变这些。

## 九、术语小词典

| 术语 | 大白话 |
|---|---|
| 功能更新（Feature Update） | 换「楼层」：25H2 → 26H2 |
| 质量更新 / LCU | 同层「换装修」：每月的累积补丁包 |
| SSU | 服务栈更新，负责「把补丁装好」的那套组件，通常和 LCU 一起装 |
| enablement package | 启用包：功能代码早就躺在系统里，装它只是「打开开关」，所以快 |
| UBR | Update Build Revision，Build 后面的小数字 |
| release health | 微软官方的「已知问题公告板」，升级前必看 |
| 提权（Privilege Escalation） | 攻击者从普通权限拿到管理员权限的那一步 |

> [!question]- 自测（点开看答案）
> 1. 本机 `26200.8457` 里，`26200` 和 `8457` 分别代表什么？
>    → `26200` 是 25H2 的 Build（楼层），`8457` 是补丁累计号（装修版本），对应 2026-05-12 的 KB5089549。
> 2. 暂停 Windows 更新后，浏览器和杀毒软件还会更新吗？
>    → 会。它们走独立通道。被冻结的只是 OS 层（内核、SMB、RDP、打印后台等）。
> 3. 为什么说 26H2 升级风险低？
>    → 它和 25H2 同一服务分支，只是启用包：下载小、一次重启、不动文件和 WSL；支持期还从 2027-10 延到 2028-10。

## 关联

[[2026秋 Index]] · [[每日日志/大二上/W10/2026-11-09|2026-11-09 日志]] · [[somezhishi/环境排障/WSL-串口权限-dialout组与chmod临时方案|WSL 串口权限]] · [[somezhishi/工具速查/内网穿透-反向SSH远程WSL|反向 SSH 远程 WSL]] · [[somezhishi/somezhishi|somezhishi]]
