---
date: 2026-09-11
tags:
  - 排障
  - obsidian
  - wsl
---

# Obsidian 启动排障：依赖缺失、T 态挂起与 WSLg 窗口不可见

> [!note] 说明
> 本文合并自 2026-08-04（AppImage 依赖缺失）、2026-08-11（GUI 不出窗口）两篇排障记录，按「故障一 / 故障二 / 故障三」组织。
> 环境：WSL2 Ubuntu-24.04 + WSLg，Obsidian Linux AppImage（Obsidian 1.13.x）。

## 故障一：AppImage 依赖缺失（libnspr4 / libasound2）

**场景**：WSL2 Ubuntu-24.04 安装 Obsidian Linux AppImage。

### 现象

- 启动报 `error while loading shared libraries: libnspr4.so: cannot open shared object file`
- 修完后再报 `libasound.so.2: cannot open shared object file`

### 原因

Electron 运行时依赖系统库。最小化 WSL 未装齐。`ldd <AppImage 解压后的 obsidian 可执行文件> | grep "not found"` 可一次列出全部缺失。

### 解决

```bash
sudo apt install -y libnspr4 libnss3 libasound2t64
```

- Ubuntu 24.04 音频库包名是 `libasound2t64`（旧版叫 `libasound2`，带 t64 后缀的是新命名）。
- 若缺库：把 AppImage 解压后 `ldd squashfs-root/obsidian | grep "not found"` 再装对应包。
- 无 libfuse2 的系统（如 24.04）需 `APPIMAGE_EXTRACT_AND_RUN=1` 运行。

## 故障二：GUI 不出窗口（T 态挂起 + WSLg 渲染通道损坏）

**场景**：WSL2 Ubuntu-24.04 + WSLg，Obsidian Linux AppImage（`~/bin/Obsidian.AppImage`，Obsidian 1.13.6）。

### 现象

- 终端输入 `obsidian-gui`，只见 `[WARN:COPY MODE]`，GUI 不弹出
- 反复尝试：任务栏出现 Obsidian 图标，但点击窗口不可见（或完全无窗口）
- `/tmp/obsidian.log` 里有 `App is up to date.` 字样

### 根因（两层叠加，与 Obsidian 自身无关）

#### 层 1：后台作业被终端作业控制挂起（进程 T 态）

旧 alias `APPIMAGE_EXTRACT_AND_RUN=1 ~/bin/Obsidian.AppImage &` 用 `&` 后台运行，但 **stdout/stderr 仍挂在终端上**。AppImage 是后台作业却尝试与终端交互 → 被作业控制信号挂起，进程停在 **T 态**（`wchan=do_signal_stop`），GUI 初始化前就被冻结。

- 诊断：`ps -o pid,stat,cmd -p <pid>` 看 STAT 是不是 `T`；`cat /proc/<pid>/wchan` 见 `do_signal_stop`
- 伴随症状：`~/.config/obsidian/` 残留 `SingletonLock/SingletonCookie/SingletonSocket`（指向已无进程的 socket），不清掉则新实例启动报「已在运行」

#### 层 2：WSLg 会话的 RDP 渲染通道损坏（任务栏有图标但窗口不可见）

WSL 本次启动时（09:10）WSLg 打开 RDP 共享内存失败：

```
/mnt/wslg/weston.log: rdp_allocate_shared_memory: Failed to open "/mnt/shared_memory/{...}" with error: Input/output error
```

后果：窗口在 X 服务器侧**完全正常**（`xwininfo` 可见 1046x804 已映射），weston 也把窗口注册进 RDP rail（任务栏出现图标），但**帧缓冲传不到 Windows 桌面**，窗口永远不可见。

- 诊断顺序：① `xwininfo -root -tree` 确认 X 侧窗口存在 → ② `grep -E "shared_memory|ClientGetAppidReq" /mnt/wslg/weston.log` 看渲染通道是否建好

#### 两个「疑似元凶」实为良性（不是故障原因）

- **`[WARN:COPY MODE]`**：AppImage runtime 提示，表示「解压到 /tmp 再运行」而非 FUSE 挂载（alias 强制 `APPIMAGE_EXTRACT_AND_RUN=1`），正常现象
- **`App is up to date.`**：AppImage 启动器每次启动的更新检查输出（加载 asar → 查 GitHub → 报已最新），正常现象

### 解决

#### 1. 清理被挂起的实例与残留锁

```bash
kill <T态PID>            # ps aux | grep -i obsidian 找 PID；杀不掉再 kill -9
rm -f ~/.config/obsidian/SingletonLock ~/.config/obsidian/SingletonCookie ~/.config/obsidian/SingletonSocket
rm -rf /tmp/scoped_dir*   # SingletonSocket 指向的临时目录（若存在）
```

#### 2. alias 加固（根治层 1）

`~/.bashrc` 第 125 行改为：

```bash
alias obsidian-gui='nohup env APPIMAGE_EXTRACT_AND_RUN=1 ~/bin/Obsidian.AppImage >/tmp/obsidian.log 2>&1 &'
```

要点：`nohup` 脱离终端（不再受作业控制信号影响）+ 输出重定向（日志落 `/tmp/obsidian.log`，不占终端）。改完 `source ~/.bashrc`。

#### 3. WSLg 渲染通道损坏 → 重启 WSL（根治层 2）

**Windows 侧**（PowerShell/CMD）执行：

```
wsl --shutdown
```

等 5 秒重新打开 WSL 终端（自动重启 WSLg，重建共享内存），再跑 `obsidian-gui`。
⚠️ 会杀掉所有 WSL 进程（opencode 会话、VS Code Remote-WSL 断开，重开即可），无数据风险。

#### 4. 启动后自检

```bash
sleep 8 && ps -o pid,stat,cmd -p $(pgrep -f "AppImage" | head -1)   # 期望 S/Sl 态（不是 T）
xwininfo -root -tree | grep -i obsidian                             # 期望见映射窗口
tail -3 /tmp/obsidian.log                                           # 期望无异常
```

## obsidian-gui 加固实现与逐命令详解

### 输入即可打开 GUI（实现步骤）

1. 确认 AppImage 存在：`ls -la ~/bin/Obsidian.AppImage`
2. 把加固 alias 写入 `~/.bashrc` 末尾：
   ```bash
   echo 'alias obsidian-gui='"'"'nohup env APPIMAGE_EXTRACT_AND_RUN=1 ~/bin/Obsidian.AppImage >/tmp/obsidian.log 2>&1 &'"'"'' >> ~/.bashrc
   ```
3. 生效：`source ~/.bashrc`（新开终端自动生效）
4. 使用：任意终端输入 `obsidian-gui` 回车，GUI 数秒内弹出
5. 若 GUI 不出：按上文「解决」3 步排查（T 态 → 锁 → WSLg）

### 涉及的文件（4 个）

| 文件 | 角色 |
|---|---|
| `~/bin/Obsidian.AppImage` | 启动的目标程序：官方 Obsidian Linux 版 AppImage（约 136MB 单文件 ELF，内含 Electron 运行时 + Obsidian 本体 + CLI）。需有执行权限（`-rwxr-xr-x`，含 `x`） |
| `~/.bashrc` | bash 配置文件。**每个新终端启动时自动读取**，alias 写在这里才能「任何终端都有效」。仅交互式 shell 读取（脚本/非交互 shell 不读——这也是排查时自动化 shell 里报 command not found 的原因） |
| `/tmp/obsidian.log` | 启动日志：alias 把 stdout/stderr 都重定向到这里（`>` 覆盖写，每次启动重新生成）。`/tmp` 重启即清，排障时 `tail -3` 查异常；`App is up to date.` 就在这里 |
| `/tmp/appimage_extracted_*` | 运行时解压目录（AppImage 启动自动生成到 /tmp，重启 WSL 后清空），无需手动管理 |

### 每个命令行

1. **`ls -la ~/bin/Obsidian.AppImage`** —— 验证文件存在 + 有执行权限，无副作用。
2. **`echo 'alias obsidian-gui='"'"'...'"'"'' >> ~/.bashrc`** —— 把 alias 追加进配置：
   - `echo '文本'`：打印文本；**引号拼接**：单引号内无法直接写 `'`，用 `'"'"'` 三段式（`'` 结束当前串 + `"'"` 双引号包单引号字符 + `'` 重新开串），把两段拼成完整一行 alias
   - `>>`：**追加**（两个 `>`，不覆盖已有内容；误用单个 `>` 会清空整个 .bashrc）
   - 等效做法：`nano ~/.bashrc` 手动加同一行
   - 最终写入的内容就是第 2 步加固 alias 那一行
3. **`source ~/.bashrc`** —— 当前终端立即重读配置，alias 马上生效，无需重开终端。
4. **`obsidian-gui`** —— 输入后 shell 把 alias 展开成完整命令，以 `&` 后台启动，立即返回提示符，GUI 数秒内弹出。

### alias 命令本身每个部分的作用

```bash
nohup env APPIMAGE_EXTRACT_AND_RUN=1 ~/bin/Obsidian.AppImage >/tmp/obsidian.log 2>&1 &
```

| 部分 | 作用 | 缺了会怎样 |
|---|---|---|
| `nohup` | 忽略 SIGHUP（终端关闭不杀进程）+ 脱离终端作业控制 | 今天元凶：`&` 后台但不脱离终端 → 被作业控制挂起（T 态、GUI 冻结） |
| `env APPIMAGE_EXTRACT_AND_RUN=1` | 强制 AppImage 走解压模式（WSL 无 libfuse2，挂载模式跑不了） | 报 FUSE 错误起不来 |
| `~/bin/Obsidian.AppImage` | 程序本体 | — |
| `> /tmp/obsidian.log` | 标准输出重定向到日志 | 输出喷终端 + 后台作业写终端有被挂起风险 |
| `2>&1` | 标准错误并入同一日志 | 报错污染终端（同样有挂起风险） |
| `&` | 后台运行，立即还提示符 | 前台占终端 5-10 秒，关终端进程即死 |

**为什么这组合最稳**：`&`（后台）+ `nohup`（脱离作业控制）+ 重定向（不与终端交互）= 不占终端、不受终端关闭/作业控制信号影响、报错有日志可查——把本次故障的两个根因（T 态挂起、无法观察日志）全部堵死。

### 为什么不直接做脚本/符号链接

alias 是 bash 特性，展开后等价于亲手输入该命令，环境变量继承最干净；脚本也可行但多一层封装。唯一限制：只在交互式终端可用（这正是预期场景——「输入命令即开 GUI」）。

## 故障三：运行中卡死（WSLg 渲染通道停摆）+ 启动器与 CLI 配套修复

**时间**：2026-09-23 晚。**现象**：Obsidian 用着用着窗口假死（点击无反应），但进程全部存活、CPU 接近 0；重跑 `obsidian-gui` 不弹窗口、反而打印 CLI 帮助；`obsidian` CLI 报 `The CLI is unable to find Obsidian`。

### 定位（weston 日志是关键证据）

- `/mnt/wslg/weston.log` 里窗口 `0x5` 从启动起持续刷：

  ```
  surface width/height doesn't match with buffer (windowId:0x5)
  	surface width 1204, height 842
  	buffer width 2408, height 1684
  ```

  共 4449 条（约 1.6 条/秒，倍增缓冲来自 `--force-device-scale-factor=2`）
- 卡死时 weston 日志**完全停止写入**（23:22:13 后 85 秒+ 无输出）→ 是 WSLg 合成器停摆，不是 Obsidian 逻辑死锁
- 内存充足、进程无 T 态 → 排除资源/作业控制类原因

### 渲染路径与 RDP 的关系（为什么 WSLg 这么脆）

```
Obsidian (Electron，GPU 走 Mesa d3d12)
  → Wayland（或 X11/XWayland）
    → weston（WSLg 合成器，rdp-backend + rdprail-shell，FreeRDP）
      → RDP 图形重定向 + 共享内存 /mnt/shared_memory
        → Windows 端 MSRDC 客户端 → 屏幕上的“Windows 窗口”
```

WSLg 的本质是**本地 RDP 远程桌面 + RAIL**：每个 Linux 窗口被编码成 RDP 表面传给 Windows 客户端。所以「任务栏有图标但窗口不显示」「buffer 不匹配刷屏」「Xwayland 一起死」都发生在这一层。

### 处置

| 步骤 | 做法 | 结果 |
|---|---|---|
| 1. 止损 | 杀掉 Obsidian 全部进程（注意 Electron 子进程是小写 `obsidian`），清理 Singleton 锁 | weston 日志恢复写入 |
| 2. 换渲染协议 | 启动器 `--ozone-platform=wayland` → `x11` | weston `doesn't match` 告警归零（观察中） |
| 3. 修启动器 | 实例检测：已在运行则给提示，不再触发 AppImage 的 CLI 透传 | 不再“弹 CLI 帮助” |
| 4. 修 CLI 环境 | `XDG_RUNTIME_DIR` 改用系统默认 `/run/user/<uid>`（WSLg 已给 wayland socket 做 symlink，无需覆盖） | `obsidian` CLI 正常连上 App |

### 容易踩的坑（记住）

1. **AppImage 第二次执行 = CLI 透传**：同一时刻只有一个 GUI 实例；已有实例时再执行 AppImage 会把参数当 CLI 命令处理，无参数就打印帮助。**这不是故障**；需要重开用 `obsidian restart`。
2. **CLI 靠 socket 找 App**：socket 路径是 `$XDG_RUNTIME_DIR/.obsidian-cli.sock`。启动 App 和运行 CLI 的 `XDG_RUNTIME_DIR` 必须一致，否则报 `unable to find Obsidian`。本机统一用 `/run/user/<uid>`。
3. 卡死后 X 服务可能一并死（`xwininfo: unable to open display ":0"`、Electron 段错误）→ `wsl --shutdown` 重启 WSLg 最干净；手动拉起 Xwayland 仅应急（会退回软件渲染）。
4. 启动器回滚包：`~/.local/bin/obsidian-gui.bak-20260923`（旧 Wayland 版）。

## 故障三后续：两机对照分析（2026-09-26）

**背景**：9-23 的假死发生在**主机**（日常主力机，仓库在 `~/SomeThingFunny/projects`）。9-26 换到**拯救者 R9000P**（外接 1080p 显示器方案）后，同款 WSLg + Obsidian AppImage 全天长跑、零告警。本节在 R9000P 上用只读命令采集配置，逐项对照，回答「为什么一台崩、一台不崩」。

### 先分清两台电脑和各自的时间线

| 代号 | 是哪台 | 日期与事件 |
|---|---|---|
| **主机**（故障机） | 日常主力机，仓库路径 `~/SomeThingFunny/projects` | 8 月中下旬起主力在此；**09-23** 假死 + weston 刷 4449 条 mismatch，切 X11 缓解 |
| **R9000P**（正常机） | 拯救者 R9000P（联想 83LV，Ryzen 9 8945HX + RTX 5060），外接 1920x1080 显示器 | `~/bin/Obsidian.AppImage` 落盘于 **08-04**、路径与故障一笔记吻合——8 月上旬的装机与故障一应发生在这台；**09-26** 本节配置采集现场，Obsidian 全天运行零告警 |

> [!note] 反直觉但最关键的发现
> 两台机器 **WSLg 版本完全相同（都是 1.0.73.2）**，一台反复假死、一台零告警。说明问题**不在 WSLg 版本**，而在两台电脑的**显示环境差异**。因此 9-23 提出的「回退 WSLg 2.6.2（内置 1.0.71）」备选路线，必要性大幅下降。

### 配置对照表

主机列已于 09-26 下午在主机实测补全（见本页「主机实测复现与归因修正」）；R9000P 列为 09-26 只读采集（`wsl.exe --version`、`/mnt/wslg/versions.txt`、`/mnt/wslg/weston.log`、PowerShell `Win32_VideoController`）。

| 维度 | 主机（故障机） | R9000P（正常机） |
|---|---|---|
| WSLg | 1.0.73.2 | **1.0.73.2（完全相同）** |
| WSL / 内核 | 2.7.13.0 / 6.18.33.2-2 | 2.7.10.0 / 6.18.33.2 |
| Windows / RDP 客户端 | Win11 26200.8457 / MSRDC 1.2.7214 | Win11 26100.4652 / MSRDC 1.2.6676 |
| GPU | Intel Arc 130T（d3d12，`/dev/dxg` 正常） | RTX 5060 Laptop（NVIDIA 驱动 610.88），`/dev/dxg` 正常 |
| 显示器拓扑 | 主屏 3120x2080 + **spacedesk 副屏（2880x1920 激活）**；共 6 个虚拟适配器 | **单屏**：外接 1920x1080（RDP 报 `UseMultimon:0`），物理 540x310mm ≈ 24.5 寸 |
| DPI 缩放 | **225% 分数缩放**（`AppliedDPI=216`；weston 报 `desktopScaleFactor:225, deviceScaleFactor:180`；F1 启动器已无 `--force-device-scale-factor=2`） | **100%**（weston 日志 `desktopScaleFactor:100, scale:1, clientScale:1.00`） |
| 渲染协议 | 当前走 wayland + 反节流 flags（9-24 F1 实验版） | 默认 X11/XWayland（启动零参数） |
| weston mismatch 告警 | **4449 条**（9-23 假死时）；09-26 实测 **1346 条 / 14 分钟，且速率仍在加快** | **0 条** |
| 虚拟显示驱动 | **6 个**：spacedesk（Running，2880x1920 激活）、IddDesk、Parsec、MuMu、**Honor Virtual（Status: Error）** | Parsec / MuMu 已装但**未激活**；**无 spacedesk** |

### 可能原因（按嫌疑从大到小）

1. **200% HiDPI 缩放（头号嫌疑）**。通俗说：200% 缩放 = Windows 按物理像素的 2 倍渲染文字图像，Obsidian 因此被要求交出「2 倍尺寸的画布」（buffer）——但 WSLg 发给它的窗口尺寸（surface）没有同步放大，两者持续对不上，weston 只能每秒刷 1.6 条告警反复重新协商；协商久了合成器停摆。R9000P 是 100% 缩放，buffer 与 surface 永远一致，零告警。WSLg 的 HiDPI 路径本就薄弱（它自己的日志都写着 `enable_fractional_hi_dpi_support=0`，即分数缩放不支持）。
2. **spacedesk 虚拟副屏（触发器/放大器）**。spacedesk 是「用软件模拟出一块屏幕」的虚拟显示器，靠网络传画面，拓扑随时可能变（断连、休眠、分辨率变化）；每次变化 RDP 客户端都要上报新布局、weston 重建所有窗口表面。「最小化→恢复后收不到帧回调（`wl_callback.done=0`）」正符合多屏拓扑变化下回调断链的 bug 路径。R9000P 单屏、两个虚拟适配器全未激活，拓扑恒定。
3. **Wayland + 2x 缩放组合（主机已自行缓解）**：假死时主机走 Electron 原生 Wayland；切 X11 后 mismatch 归零。R9000P 天生走 XWayland 默认路径，从未踩进这条路径。
4. **GPU 驱动（次要）**：R9000P 走 NVIDIA d3d12（WSL 下最成熟的 GPU 路径）；主机 GPU 待补。但本故障是合成器停摆、不是 Electron GPU 崩溃，故排次要。
5. ~~WSLg 版本回归~~：基本排除，见上方 callout。

> [!note] 09-26 下午修正
> 主机实测后，本节第 1 条的「200%」应修正为「**225% 分数缩放**」，且「2 倍 buffer 来自 `--force-device-scale-factor=2`」的旧解释被现场证伪——两处修正与两条新证据见下节「主机实测复现与归因修正」。

### 对实验计划的修正（下一步，回主机执行）

| 优先级 | 实验 | 做法 | 观察点 |
|---|---|---|---|
| ① | **E3（本次新增；「去掉 flag」这半已被 09-26 下午现场证伪，见下节 v2）** | 主屏缩放临时改整数倍（200% / 100%），用半天 | `grep -c "doesn't match" /mnt/wslg/weston.log` 是否归零；最小化→恢复是否复现假死。若归零 → 坐实分数缩放是主因 |
| ② | E2 | 停用 spacedesk 副屏后用一天 | 同上 |
| ③ | E1 | 窗口只放主屏、不拖去 spacedesk | 同上 |
| 暂缓 | 回退 WSLg 2.6.2 | 同版本 1.0.73.2 在 R9000P 上健康，版本回归基本排除 | — |

**主机数据**：09-26 下午已在主机只读采集补全（写入上表与下节）。

### 主机实测复现与归因修正（2026-09-26 下午）

**背景**：09-26 下午回到主机（HONORbook14pro）做只读采集，补全上表「待补」数据；采集当下**假死条件正在活跃复现**（weston mismatch 14 分钟刷 1346 条且速率加快），并拿到两条推翻旧归因的新证据。

#### 两条新证据

1. **2 倍 buffer 与 `--force-device-scale-factor=2` 无关**：当前 F1 启动器已不含该 flag，但 buffer 依然精确 2 倍（surface 1032x913 vs buffer 2064x1826）。→ 倍增来自 **225% 分数缩放本身**：WSLg 日志明示 `enable_fractional_hi_dpi_support=0`，weston 把 225% 协商成 deviceScale 180%，Chromium 的 Wayland 缓冲只支持整数倍、交 2x → 每帧错位，永远协商不拢。E3 的「去掉 flag」一半已被现场证伪。
2. **mismatch 爆发与缩放协商风暴强相关**：13:39:12–13:39:23 scale 五连变（225→200→100→0→200→200），紧接着 13:40 爆发到 355 条/分；13:43–13:45 静止 3 分钟后恢复并加速（130→567 条/分）——正是 9-23 假死前「最小化→恢复后回调断链」的窗口形态。

#### 修正后的嫌疑排序

1. **225% 分数缩放（头号，已实证）**——旧记录只知道「200% + flag」，实际是 225% 分数缩放；R9000P 是 100% 整数倍，永远一致。
2. **显示拓扑复杂度（触发/放大器）**——spacedesk 常驻激活 + Honor Virtual（Error）+ 共 6 个适配器；任何拓扑/缩放变化触发 RDP 重报 → weston 重建表面（13:39 五连变即现场证据）。
3. **Wayland 路径（放大器）**——F1 实验版走 wayland，直接暴露在 scale 协商之下；9-23 切 x11 后 mismatch 曾归零。
4. **次要**：Intel d3d12 vs NVIDIA d3d12、MSRDC/WSL 小版本差异——故障机理在合成器协商层而非 GPU 崩溃层，维持次要。
5. ~~WSLg 版本回归~~——两机同版本，已排除。

#### 实验计划 v2

| 优先级 | 实验 | 做法 | 观察点 |
|---|---|---|---|
| ① E3' | 主屏缩放 225% → 整数倍（优先 200%） | 重启 Obsidian 用半天 | mismatch 是否停止增长；最小化→恢复是否复现 |
| ② E2 | 停 spacedesk（服务 + 断开副屏客户端） | 观察 scale 五连变是否消失 | 同上 |
| ③ | 卸载 / 禁用 Honor Virtual Display Device | 设备管理器看状态 | 适配器台账变干净 |
| ④ | 换回 `obsidian-gui.x11-test-20260923` 对照 | 确认 x11 下是否也随 225% 出现 mismatch | 分离「缩放」与「协议」两个变量 |

#### 结论（2026-09-27 更新）

**E3' 已执行：把主屏缩放从 225% 分数缩放改为整数倍后，obsidian-gui 未再复现崩溃——缩放（分数缩放）为主因得到坐实。** 防复发要点：主屏保持整数倍缩放；spacedesk / Honor Virtual 等虚拟显示设备按需启用、不用时禁用。

## 关联

- [[somezhishi/编程基础/Linux任务控制-JobControl与kill|Linux 任务控制：Job Control 与 kill]]（T 态挂起、nohup/setsid 的底层机制）
- [[归档/30天学习/30天学习 Index]] · [[归档/30天学习/术语表/术语表|术语表]]
