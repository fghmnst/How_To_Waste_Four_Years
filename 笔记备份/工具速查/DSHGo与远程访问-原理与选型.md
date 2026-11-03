---
date: 2026-11-04
tags: [指南, 工具, linux, ssh, 项目]
---

# DSHGo与远程访问-原理与选型

> 用途：一篇看完 DSH Go 的本机 Linux 环境、远程连接机制，以及通用的内网穿透 / 虚拟组网 / ZTNA 选型。
> 拆分细节：[[somezhishi/工具速查/DSHGo-本机Linux与PRoot|DSH Go 本机 Linux 与 PRoot]]、[[somezhishi/工具速查/DSHGo-远程连接机制|DSH Go 远程连接机制]]、[[somezhishi/工具速查/远程访问-内网穿透策略|内网穿透策略]]、[[somezhishi/工具速查/远程访问-虚拟组网与ZTNA|虚拟组网与 ZTNA]]。
> 结论先行：**DSH Go 本机不是虚拟机，是 PRoot + Ubuntu glibc；DSH Remote 不是“NAT 组网”，是应用层 P2P NAT 穿越 + 中继回退 + E2EE。**

## 目录

- [一、DSH Go 本机 Linux 环境](#一dsh-go-本机-linux-环境)
- [二、DSH Go 远程连接机制](#二dsh-go-远程连接机制)
- [三、通用远程访问策略](#三通用远程访问策略)
- [四、三句话总结](#四三句话总结)
- [五、术语总表](#五术语总表)
- [六、自测](#六自测)

## 一、DSH Go 本机 Linux 环境

### 1.1 一句话结论

| 问题 | 答案 |
|---|---|
| 是虚拟机吗 | 不是。没有 guest kernel、没有 QEMU、没有 KVM |
| 用 Android 的 Linux 内核吗 | 用。系统调用最终都进入同一个 Android 内核 |
| 像 Termux 那样原生吗 | 默认不是。Termux 是 bionic 原生；DSH Go 是 glibc + PRoot 拦截 |
| CPU 性能 | 接近原生（不模拟指令） |
| 文件 I/O / 启动性能 | 有明显开销（每次路径类系统调用多绕一圈） |
| 有真 root 吗 | 没有。`id` 显示 root 是假 root，真实进程仍是 App 的 Android UID |
| 能跑 Docker/systemd/iptables 吗 | 不能。没有 namespace/cgroup/内核模块 |
| 适合做什么 | 零配置跑 DSH；不适合替代通用 Linux 工具箱 |

### 1.2 启动链路

```text
Flutter UI
  → Kotlin DshProcess
    → libproot.so（ptrace/seccomp 拦截系统调用）
      → Ubuntu 24.04 rootfs 里的 /opt/node/bin/node
        → dsh web（只监听 127.0.0.1）
```

关键事实：

- PRoot 版本是 **5.1.107.98**，直接取自 Termux 的 `proot` deb 包，复制成 `libproot.so` 放进 APK。
- 启动参数包含 `--kill-on-exit --link2symlink --sysvipc -L -0`：
  - `-0`：假 root；
  - `--link2symlink`：模拟硬链接；
  - `--sysvipc`：模拟 System V IPC。
- rootfs 是 Ubuntu 24.04.5 + Node 24 + DSH + pnpm；APK 里压缩后约 151MB，装完建议预留约 1GB。
- `targetSdk = 28`（和 Termux 一样）：绕开 Android 10+ 对 App 私有目录 `exec` 的限制，让 rootfs 里的二进制能执行。这是明确的兼容性取舍，不适合直接上 Google Play。

### 1.3 PRoot 是什么 / 不是什么

**它做的事：**

- 程序执行 `open("/etc/passwd")` 时，PRoot 把路径改写成 `<rootfs>/etc/passwd`；
- `chroot`、`bind mount`、部分 IPC 由 PRoot 在用户态“模拟”；
- 通过 `ptrace`（以及 seccomp 加速）拦截系统调用，修改参数/返回值。

**它不做的事：**

- 不启动 guest 内核；
- 不模拟 CPU 指令；
- 不创建 namespace、不用 cgroup；
- 不是 Docker 那种容器隔离；
- **不是安全沙箱**。它只解决路径/root 兼容问题，里面的程序仍然能访问 App 能访问的东西。

> [!warning] 常见误解
> “PRoot 里 `id` 是 root”不等于“拿到了 root 权限”。真实进程仍是 App 的 UID，受 Android SELinux 和沙箱约束；不能挂载内核模块、不能改内核参数、不能绕过 Android 权限。

### 1.4 性能到底差多少

仓库自己的实测（x86_64 模拟器，同一份 rootfs、同一个 DSH 主目录）：

| 跑法 | warm 启动 | cold 启动 |
|---|---:|---:|
| PRoot | 1.93s | 2.27s |
| 绕开 PRoot，用 rootfs 的 `ld.so` 直接跑同一个 Node | 1.07s | 1.35s |
| 差异 | **-44%** | **-41%** |

原因：DSH 启动要发起约 **28,100 次路径类系统调用**（`statx`、`openat`、`readlink`、`access`），PRoot 下每次约 30µs，累计就是秒级。

项目做的优化：

- **启动期路径缓存**：把 `realpathSync`、`existsSync`、`package.json` 读取缓存起来，启动段 warm 快 14%、cold 快 12%；
- **打开 App 即拉起引擎**：与 Flutter 初始化并行，端到端从 3.02s 降到 2.46s（-18%）；
- **可选 Direct 模式**：主引擎绕过 PRoot，启动段再快约 35%–44%。

> [!note] 性能结论
> CPU 密集型任务几乎不亏；**文件 I/O、进程启动、大量小文件操作**亏得最明显。npm install、编译、git 操作会比 Termux 原生慢。

### 1.5 Direct 模式：更接近原生，但默认关闭

Direct 模式不启动 PRoot，而是用 rootfs 里的 glibc 动态链接器直接加载 Node：

```text
<rootfs>/usr/lib/aarch64-linux-gnu/ld-linux-aarch64.so.1
  --library-path <rootfs>/usr/lib/...:<rootfs>/lib/...
  <rootfs>/opt/node/bin/node
  --import=<rootfs>/opt/dsh-go/direct/preload.mjs
  <rootfs>/opt/dsh/node_modules/@deepseek-ai/dsh/lib/bin.js web ...
```

限制：

- **默认关闭**，要在“开发者选项 → 直接启动本机引擎”里手动打开；
- 部分机型（Android 16 / x86_64）的 seccomp 会拦截 glibc loader，启动失败后自动回退 PRoot；
- 只让主引擎绕过 PRoot，agent 的 bash/git/python/apt 等工具子进程仍可能回到 PRoot 兼容路径；
- 不是“零成本”，只是把最重的启动路径从 ptrace 里拿出来了。

### 1.6 和 Termux 对比

先明确：**Termux 也不是“另一个内核”**。它是 Android 上的终端 App，用户态是 Android 的 bionic libc，直接跑在同一个 Linux 内核上；它不需要 PRoot，所以没有 ptrace 开销。要在 Termux 里跑 Ubuntu/glibc，通常用 `proot-distro`，那时才引入 PRoot，和 DSH Go 是同一类机制。

| 维度 | Termux 原生 | Termux + proot-distro | DSH Go 本机 |
|---|---|---|---|
| 内核 | Android Linux | Android Linux | Android Linux |
| 用户态 | bionic | glibc（Ubuntu/Debian） | glibc（Ubuntu 24.04） |
| 隔离/翻译 | 无，原生系统调用 | PRoot ptrace 翻译 | PRoot ptrace 翻译（可选 Direct） |
| root | 无 | 假 root | 假 root |
| 性能 | 最接近原生 | 系统调用密集场景明显变慢 | 同左，启动有缓存优化 |
| 安装 | 装 Termux + `pkg` | 装 Termux + `proot-distro` | 装 APK 即自带 |
| 体积 | 较小 | 中（再加 rootfs） | 大（APK 约 196MB，装完约 1GB） |
| 通用性 | 强，完整终端/包管理/SSH/X11 | 强，可换发行版 | 弱，围绕 DSH 定制 |
| 终端 | 完整终端 | 完整终端 | DSH 工作区终端（本机/SSH 可用） |
| Android API | `termux-api`（相机、传感器、短信等） | 同左 | 手机操控（无障碍/Shizuku/ADB） |
| 后台保活 | 需自行配置 | 需自行配置 | 前台服务 + 保活引导 |
| 内核模块/iptables/Docker/systemd | 否 | 否 | 否 |

### 1.7 优缺点与适用场景

**优点：**

- 零配置：不用装 Termux、不用 `proot-distro install ubuntu`、不用自己装 Node/DSH。
- 和 DSH App 深度集成：会话、插件、模型配置、手机操控、前台服务、日志、崩溃恢复、安全模式、运行时校验/回滚都在一个 App 里。
- 针对 DSH 启动做了路径缓存、预启动、可选 Direct 模式，实际体验比“裸 proot-distro”更顺。
- 对 glibc 生态兼容更好：Node、Python、JVM、LibreOffice WASM、原生扩展都能直接跑，不用为 bionic 重新编译。
- 运行环境有版本、SHA-256、回滚和安全模式管理，升级由 App 负责。

**缺点：**

- **更重**：APK 约 196MB（1.0.8 实测，其中 151MB 是 rootfs），装完约 1GB；Termux 原生轻得多。
- **I/O 性能不如 Termux 原生**：PRoot 的 ptrace 开销在 npm install、大量小文件、编译、git 操作上很明显。
- **不是通用 Linux 环境**：没有 Termux 的包生态、没有 X11/VNC、没有默认 sshd、没有 `termux-api`；终端是 DSH 的工作区终端。
- **控制权少**：不能换发行版、不能真正 root、不能 systemd/Docker/iptables/mount、不能加载内核模块；假 root 下 setuid 等语义也不完整。
- **沙箱能力受限**：PRoot 下 DSH 的 `workspace-write` 沙箱（bwrap/Landlock）可能不可用；它不能当安全隔离用。
- **兼容性取舍**：`targetSdk 28`、仅侧载、Android 8+、仅 arm64/x86_64；Android 16 等新系统上 Direct 模式还可能被 seccomp 拦截。

**怎么选：**

- 只想在手机上零配置跑 DSH：DSH Go 本机引擎。
- 想要通用 Linux / 编译 / 终端 / 更好性能：Termux 原生；需要 Ubuntu 时用 Termux + proot-distro。
- 想要接近原生的 Linux 性能：rooted chroot 或 Android AVF（真实 Linux VM），但不是免 root 方案。

## 二、DSH Go 远程连接机制

### 2.1 两条远程路径

| 路径 | 本质 | 是否经过第三方 | 适合 |
|---|---|---|---|
| SSH 主机 | 手机直连电脑 SSH，在远端启动 `dsh web`，再把远端回环端口转发到手机 | 否 | 同 LAN、有公网 IP、已有 VPN/异地组网 |
| DSH Remote | 电脑插件出站连服务器，手机登录同账号；WebRTC P2P/TURN 或 WebSocket Relay 承载 | 是（默认托管服务器；可自建） | 电脑在 NAT 后、没有公网 IP |

### 2.2 SSH 主机：直连 + 本地端口转发

```text
手机 DSH Go
  │ ① dartssh2 直连 电脑:22
  ↓
电脑 sshd
  │ ② 启动/复用 dsh web --host 127.0.0.1 --port 0 --no-open
  │    （nohup，让它活过 SSH 会话；从日志解析 ?token=）
  ↓
手机本地 127.0.0.1:<随机端口>
  │ ③ SSH direct-tcpip 转发
  ↓
电脑 127.0.0.1:<dsh端口>
  │ ④ DshClient 带 Host: 127.0.0.1:<远端端口> 登录
```

关键点：

- 电脑必须开着 SSH，并装好 DSH（`npm i -g @deepseek-ai/dsh`）。
- 手机用密码或私钥登录；主机指纹 TOFU（首次确认后 pin，变更则失败关闭）。
- 远端 `dsh web` 只绑 `127.0.0.1`，不暴露公网；手机通过 SSH 隧道访问。
- **不经过第三方服务器**，但电脑必须“手机能直接访问到”。

适用与限制：

- 同 Wi-Fi、家里/公司 LAN：最直接。
- 有公网 IP 或 VPS：直接连。
- 已有 Tailscale/ZeroTier/WireGuard：在其上跑 SSH，等于借了对方的组网能力。
- **没有内建 NAT 穿透**：电脑在 CGNAT/双层 NAT 后、又没有公网入口时，SSH 模式默认连不上。
- 不是“内网穿透”：没有公网端口映射，也没有中继；它就是“能连上 SSH 就能用”。

### 2.3 DSH Remote：账号制、应用层 P2P/中继

```text
电脑上的 DSH
  └─ ds-harness-remote 插件（李国宝的项目）
       └─ 主动出站 WSS → 协调服务器（默认 dsh.r2049.cn，可自建）
                          ↑
手机 DSH Go ─ 登录同一账号 ─┘
  ├─ 拉取在线 Host 列表、校验身份指纹
  ├─ Control WebSocket：hello → connect.request → connect.accepted
  ├─ 尝试 WebRTC：LAN → P2P → TURN
  │    └─ DataChannel "dsh"：flutter_webrtc（手机）/ wrtc 或 werift（电脑）
  └─ 失败则回落 Relay：WebSocket 转发密文
```

关键点：

- **电脑不需要公网 IP，也不需要开入站端口**；插件只做出站连接。
- 服务器负责账号、设备发现、在线状态、连接协调和信令；它**看不到明文业务数据**。
- 端到端加密：`Noise_IK_25519_ChaChaPoly_SHA256`，双方 X25519 设备身份密钥；Host 指纹首次确认后 pin，之后密钥变化直接失败关闭。
- 传输选路：优先 LAN，其次 P2P（STUN 打洞），再 TURN 中继，最后 Server Relay；每条路径都承载同一套 Noise 密文。
- 默认服务器是第三方托管服务；也支持自建 Server（邮箱+密码登录）。但开源的最小 Server **只包含 Relay，不包含 WebRTC/TURN**，需要自己补反向代理和 TURN。
- DSH Remote 只开放 DSH 的 ApiProxy 能力，不是通用 TCP/UDP 隧道。

适用与限制：

- 电脑在 NAT 后、没有公网 IP：首选。
- 手机用蜂窝网络、电脑在家：P2P 失败时自动 TURN/Relay。
- 只想远程用 DSH：功能刚好；要远程桌面/文件共享则不够。
- 限制：一次只连一台主机；Relay 模式带宽/大小有限；App 内终端在 DSH Remote 下不可用；只支持图片附件等。

### 2.4 SSH 与 DSH Remote 对比

| 维度 | SSH 主机 | DSH Remote |
|---|---|---|
| 连接方式 | 手机直连电脑 SSH | 电脑插件出站连服务器，手机登录同账号 |
| 是否需要公网 IP | 通常需要（除非已有 VPN） | 不需要 |
| 是否经过第三方 | 否 | 是（可自建） |
| 传输 | SSH / TCP | WebRTC DataChannel / TURN / WebSocket Relay |
| 端到端加密 | SSH | Noise IK（应用层 E2EE） |
| 访问范围 | 被转发的 DSH 端口 | 只 DSH API |
| 配置 | 电脑开 sshd + 装 DSH | 电脑装远程插件 + 登录账号 |
| 适合 | 同 LAN、VPS、已有 Overlay | NAT 后、零配置远程 |

### 2.5 它到底算哪一类？

| 说法 | 是否准确 | 说明 |
|---|---|---|
| NAT 组网 | 不准确 | “NAT 组网”不是标准术语；DSH Remote 不创建虚拟网络 |
| 内网穿透 | 不准确（效果像） | 没有公网端口映射；它是应用层 P2P/中继，不是 frp/ngrok 式端口转发 |
| 异地组网 | 不准确 | 没有虚拟网卡/虚拟 IP；只隧道 DSH API |
| P2P NAT 穿越 + 中继回退 | 准确 | WebRTC ICE/STUN 打洞，失败走 TURN/Relay |
| 应用层零信任远程访问 | 比较接近 | 账号成员校验 + Host 指纹 pin + E2EE + API allowlist |

一句话：

> **DSH Remote = 带中继回退的 P2P NAT 穿越 + 应用层隧道 + 端到端加密。**

它和 Tailscale 的相似点是“协调服务器 + P2P + 中继 + E2EE”；区别是 Tailscale 给你一个 L3 虚拟网络，DSH Remote 只给你一条 DSH 专线。

### 2.6 安全与验证边界

**安全模型：**

- SSH：主机指纹 TOFU pin；密码/私钥存 Android Keystore；不经过第三方。
- DSH Remote：Noise IK E2EE；服务器只看到连接元数据和密文；Relay 票据只走数据 socket；Host 身份指纹首次确认后 pin。
- **首次连接依赖服务器分发的 Host 公钥**：恶意服务器理论上可在首次接触做中间人；指纹只有 48 bit；之后 pin 失败关闭。
- 账号被盗等于 Host 被访问；这是上游模型本身的边界。
- 自建 Server 时，最小开源版没有 WebRTC/TURN，且需要自己配 HTTPS/WSS 反向代理。

**验证边界（仓库自己写明）：**

- LAN/P2P 在 Android 35 x86_64 模拟器上真实跑通过；
- **真实公网 NAT、真实 TURN、移动网络、OEM 真机尚未验证**；
- “P2P”在模拟器同宿主环境下的成功，不等于所有公网 NAT 都能打洞；
- Relay 模式是确定能用的兜底，但带宽/大小有限。

### 2.7 怎么选

- 电脑在同一 Wi-Fi 或有公网 IP/VPS：**SSH 主机**，最直接，不经过第三方。
- 电脑在 NAT 后、没有公网 IP：**DSH Remote**。
- 要访问电脑上的所有服务（SMB/RDP/数据库）：**另装 Tailscale/ZeroTier/WireGuard，再在其上跑 SSH/RDP**；DSH Remote 不做通用网络。
- 只远程用 DSH：DSH Remote 足够；接受它的功能限制。

## 三、通用远程访问策略

### 3.1 先对齐术语

| 说法 | 准确含义 | 不是什么 |
|---|---|---|
| NAT | 网络地址转换：把私有 IP/端口映射成公网 IP/端口 | 不是一种组网技术 |
| NAT 穿透 / NAT traversal | 让 NAT 后的设备通过信令 + 打洞直接建立连接 | 不是“NAT 组网” |
| 内网穿透 | 通常指反向隧道/端口映射，把内网服务暴露到公网 | 不一定是 P2P |
| 虚拟组网 / Overlay | 在物理网络之上建虚拟网络，设备像在同一 LAN | 不是单端口转发 |
| ZTNA | 零信任应用访问：身份认证 + 应用级代理 | 不是完整 VPN |
| SD-WAN | 企业多链路、策略路由、站点互联 | 不是个人远程工具 |

### 3.2 策略全景

| 策略 | 核心机制 | 典型产品/协议 | 适合 |
|---|---|---|---|
| 公网直连 / 端口映射 | 公网 IP + 路由器/防火墙放行端口 | 路由器端口转发、IPv6、UPnP/NAT-PMP/PCP、DDNS | 有公网 IP 的服务器、游戏服务器 |
| 反向隧道 / 内网穿透 | 内网主机主动连公网服务器，公网服务器暴露端口 | frp、nps、ngrok、cloudflared、rathole、`ssh -R` | 无公网 IP，但有一个公网 VPS/SaaS |
| P2P NAT 穿越 | 信令 + STUN/ICE 让两端直接打洞 | WebRTC、STUN/ICE、UDP/TCP hole punching | 低延迟远程桌面、游戏、文件传输 |
| 中继转发 | 所有流量经公网服务器转发 | TURN、DERP、WebSocket Relay、frp 中继 | P2P 失败兜底；严格防火墙 |
| 虚拟组网 / Overlay | 虚拟网卡 + 加密隧道，设备像在同一 LAN | Tailscale、ZeroTier、Netbird、Nebula、WireGuard、Headscale | 远程办公、homelab、多服务/多设备 |
| 零信任应用访问 ZTNA | 身份 + 应用级代理，只开放单个应用 | Cloudflare Access/Tunnel、Twingate、Teleport、Pomerium、Tailscale Serve | 企业远程办公、SaaS、替代传统 VPN |
| 企业 SD-WAN / Site-to-Site VPN | 多分支互联、多链路选路、策略路由 | IPsec、WireGuard、SD-WAN、SASE | 企业分支、总部-门店、多云 |
| 远程桌面协议 | 应用层协议，可跑在任何传输上 | RDP、VNC、Parsec、Moonlight、RustDesk、AnyDesk | 远程操作桌面 |

### 3.3 内网穿透四类

#### 1. 反向端口映射 / 反向隧道

```text
内网服务 <内网IP>:8080
   ↑
frpc / ngrok agent / ssh -R
   │ 主动出站连接
   ↓
公网服务器 frps / ngrok edge / VPS
   │ 监听公网端口 8080
   ↓
互联网用户 → 公网服务器:8080 → 隧道 → 内网服务
```

内网主机主动向公网服务器建立长连接，公网服务器在某个端口上接收外部请求，再通过这条连接转发给内网。**内网不需要公网 IP，也不需要开入站端口。**

常见实现：frp、nps、ngrok、cloudflared、rathole、sish、`ssh -R`。

**优点：** 对网络要求最低（能出站即可）、不改路由器、可暴露任意 TCP/UDP/HTTP 服务、frp/nps 可自建。  
**缺点：** 所有流量走公网服务器，带宽/延迟/成本受限制；公网服务器是单点故障和攻击面；TLS 终止在服务器时能看到明文；端口管理麻烦。  
**适合：** 没有公网 IP 但有 VPS；暴露一两个 Web 服务或 SSH；临时开发、演示、远程支持。

> [!warning] 安全底线
> 不要把 RDP、数据库、SSH 直接裸奔到公网。至少加：强密码/密钥、MFA、TLS、IP 白名单、fail2ban、只暴露必要端口。

#### 2. HTTP/HTTPS 反向代理型穿透

```text
浏览器 → https://app.example.com
          → Cloudflare / nginx / Caddy（公网）
            → tunnel → 内网 localhost:3000
```

公网侧只开 80/443，按域名/Host/Path 路由；自动 TLS、WebSocket、HTTP/2、HTTP/3；常和 SSO/访问策略结合。代表：Cloudflare Tunnel、ngrok HTTP、frp HTTP、Caddy + 反向隧道、Tailscale Funnel/Serve。

**优点：** 只开 443、容易加认证、支持多域名、自动证书、对防火墙友好。  
**缺点：** 主要适合 HTTP(S)；TCP/UDP 需要额外方案；SaaS 会看到 TLS 终止后的明文（除非应用层 E2EE）。

#### 3. P2P NAT 穿越（打洞）

不是“端口映射”，而是让两端**直接建立连接**，不经过业务中继。

核心组件：

- **信令服务器**：交换双方的候选地址、SDP、公钥等。
- **STUN**：告诉客户端“你在公网看起来是什么 IP:端口”。
- **ICE**：收集候选地址（host、srflx、relay），逐一测试连通性，选最优路径。
- **TURN**：打洞失败时的中继兜底。
- **UDP/TCP hole punching**：双方同时向对方公网地址发包，让 NAT 建立映射。

NAT 类型与打洞难度：

| NAT 类型 | 行为 | 打洞难度 |
|---|---|---|
| Full Cone | 内部主机出站后，任何外部主机都可发入 | 容易 |
| Restricted Cone | 只允许已通信过的外部 IP 发入 | 较易 |
| Port-Restricted Cone | 只允许已通信过的外部 IP:端口发入 | 中等 |
| Symmetric NAT | 每个目标分配不同公网端口，外部无法预知 | 难，通常要 TURN |

再加上 CGNAT、双层 NAT、企业防火墙、UDP 被封，P2P 成功率会明显下降。所以成熟方案一定是 **P2P 优先 + 中继兜底**。

**优点：** 延迟最低、不消耗中继带宽、隐私更好、适合实时音视频/远程桌面/游戏/文件传输。  
**缺点：** NAT/防火墙行为不可控、需要 STUN/TURN/信令、UDP 可能被封、移动网络切换会断、直连暴露双方 IP。

#### 4. 中继转发（TURN / DERP / WebSocket Relay）

当 P2P 失败时，流量经公网服务器转发：

- **TURN**：WebRTC 标准中继，支持 UDP/TCP/TLS；服务器分配 relay 地址。
- **DERP**：Tailscale 的中继，走 HTTPS/WebSocket，转发 WireGuard 密文。
- **WebSocket Relay**：应用层中继，DSH Remote 的最终兜底。
- **frp/ngrok**：本质上也是中继，只是更偏“端口映射”。

**优点：** 成功率高，几乎一定能连。  
**缺点：** 增加延迟、消耗服务器带宽、成本高；服务器能看到流量元数据（E2EE 时看不到明文）。

**结论：** 现代远程访问的标配是 **P2P 优先 + 中继兜底**，而不是二选一。

### 3.4 虚拟组网四类

#### 1. WireGuard 点对点 / Hub-and-Spoke

```text
手机 ── WireGuard 隧道 ──→ VPS（Hub）──→ 家里电脑
                                    └──→ 公司服务器
```

每台设备装 WireGuard，配置对端公钥和 Endpoint；所有流量走加密隧道。Hub-and-Spoke 就是用一个有公网 IP 的 VPS 做中心节点。

**优点：** 快、加密强、配置透明、可路由整个网段。  
**缺点：** 本身没有 NAT 穿透/协调能力，需要公网 Endpoint 或静态配置；Hub 是单点/带宽瓶颈；移动端切换网络要重连。  
**适用：** 有 VPS、固定公网 IP、少量设备、自己掌控一切的场景。

#### 2. Mesh Overlay（Tailscale / ZeroTier / Netbird / Nebula）

```text
设备 A ──┐
设备 B ──┼── 协调服务器（控制面：身份、密钥、ACL、在线状态）
设备 C ──┘        │
                  ├─ 尽量 P2P 直连（NAT 穿越）
                  └─ 失败时走中继（DERP / Relay）
```

- **Tailscale**：基于 WireGuard 的 L3 虚拟网络；协调服务器管身份/密钥/ACL；P2P 优先，失败走 DERP；支持 MagicDNS、Subnet Router、Exit Node、SSO/MFA；可自建 Headscale。
- **ZeroTier**：L2 虚拟以太网，支持广播/多播，可桥接物理 LAN；自己的协议，Planet 根服务器协调，可自建 Moon。
- **Netbird**：WireGuard + SSO + ACL，开源可自建，偏企业/团队。
- **Nebula**：证书体系 + Lighthouse 节点，轻量、去中心化，适合自建。

**优点：** 一次组网所有端口/IP 可达；稳定虚拟 IP/DNS；通常带 SSO/ACL/审计；P2P 直连延迟接近原生，失败有中继；跨平台好。  
**缺点：** 每台设备都要装客户端；依赖协调服务器；中继有带宽/延迟成本；子网路由/DNS/MTU/ACL 有学习成本；移动端耗电/保活；默认网络级权限需要 ACL 细化。  
**适用：** 远程办公、homelab、家里 NAS、多台服务器、IoT；需要访问多个服务/端口。

#### 3. Site-to-Site VPN / SD-WAN

- **Site-to-Site VPN**：用 IPsec/WireGuard 把两个固定网络（总部-分支、云上 VPC-办公室）连起来；透明、稳定、所有内网 IP 可达；但需要两端网关、静态配置、运维复杂。
- **SD-WAN**：多链路（MPLS、宽带、5G）+ 应用感知路由 + 策略 + 安全；常和 SASE 结合（SD-WAN + ZTNA + SWG + CASB + FWaaS）；链路冗余、体验好、集中策略，但贵、复杂、面向企业。

#### 4. ZTNA（零信任应用访问）

```text
用户 → 身份认证（SSO/MFA/设备姿态）
     → ZTNA 云/网关
       → 出站连接器（部署在内网）
         → 只开放被授权的应用
```

代表：Cloudflare Access + Tunnel、Twingate、Teleport、Pomerium、Zscaler Private Access、Tailscale Serve。

**优点：** 最小权限、不开入站端口、SSO/MFA/设备合规/审计、比传统 VPN 更适合远程办公/第三方访问。  
**缺点：** 按应用接入，不适合任意 TCP/端口；依赖供应商/云；需要连接器和身份源；对非 HTTP 协议支持有限。  
**和 VPN 的区别：** VPN 先连入网络、权限大；ZTNA 先验证身份/设备、只放行单个应用。

### 3.5 内网穿透 vs 虚拟组网

| 维度 | 内网穿透 | 虚拟组网 |
|---|---|---|
| 目标 | 暴露某个服务/端口 | 让设备像在同一 LAN |
| 访问范围 | 单服务/单端口/单域名 | 所有 IP/端口 |
| 网络层 | 应用层/TCP/HTTP 隧道 | L3/L2 虚拟网络 |
| 客户端 | 通常只需内网侧 agent + 公网服务器 | 每台设备都要装客户端 |
| 寻址 | 公网域名/端口 | 虚拟 IP / MagicDNS |
| 性能 | 走中继时受服务器带宽限制 | 直连时接近原生，失败走中继 |
| 安全 | 公网暴露面，需 TLS/认证 | 默认私有，需 ACL/SSO |
| 典型 | frp、ngrok、cloudflared、`ssh -R` | Tailscale、ZeroTier、Netbird、WireGuard |
| 适合 | Webhook、开发联调、单服务远程访问 | 远程办公、homelab、多服务/多设备 |

**一句话：**

- **内网穿透 = 开一扇门**（只开你要的那扇）。
- **虚拟组网 = 修一条路**（路通了，所有门都能走）。
- **ZTNA = 只给你一张指定门的门禁卡**（最细粒度）。

### 3.6 这些策略过时了吗？

**没有过时，但适用边界变了。**

| 策略 | 现状 |
|---|---|
| 端口映射 / 公网直连 | 没过时；VPS、服务器、游戏服务器仍在用。但消费者受 CGNAT/IPv4 枯竭限制，不适合裸奔敏感服务 |
| frp / ngrok / 反向隧道 | 没过时；开发联调、Webhook、远程支持仍常用，正在吸收 HTTP/2、HTTP/3/QUIC、SSO |
| P2P NAT 穿越 | 没过时，反而更重要；WebRTC、QUIC、Tailscale/ZeroTier、远程桌面都依赖它 |
| 虚拟组网 / Overlay | 没过时，是当前主流；Tailscale/ZeroTier/Netbird 是个人/中小企业首选之一 |
| 传统 VPN | IPsec/OpenVPN 仍大量存在，但新部署更倾向 WireGuard/ZTNA；PPTP、无加密 L2TP 已过时且不安全 |
| SD-WAN | 没过时，但属于企业级；现代趋势是 SASE = SD-WAN + 零信任 + 云安全 |
| UPnP / NAT-PMP / PCP | 越来越不推荐：路由器常关、任何应用都能开端口、CGNAT 下无效 |

**现代最佳实践的组合拳：**

1. **身份优先**：SSO、MFA、设备姿态、最小权限。
2. **出站优先**：尽量不开放入站端口，用反向隧道/连接器。
3. **P2P 优先**：能直连就直连，降低延迟和带宽成本。
4. **中继兜底**：TURN/DERP/Relay 保证可达性。
5. **端到端加密**：WireGuard/Noise/TLS 1.3，别只依赖链路加密。
6. **应用级授权**：能 ZTNA 就不要给整个网络；能只开一个端口就不要开全网。
7. **可观测性**：连接路径、RTT、回落原因、审计日志。

### 3.7 场景选型表

| 场景 | 首选 | 备选 |
|---|---|---|
| 访问家里/公司所有设备、所有端口 | **Tailscale / Netbird / ZeroTier**（WireGuard Mesh） | 自建 WireGuard Hub、Headscale |
| 只暴露一个 Web 服务/开发环境 | **Cloudflare Tunnel + Access** | frp + OAuth2 Proxy、ngrok、Tailscale Funnel |
| 低延迟远程桌面/游戏串流 | **Parsec / Moonlight / RustDesk（P2P + 中继）** | Tailscale + RDP、WebRTC 自建 |
| 临时 SSH/运维 | **SSH over Tailscale** | 直连 SSH、`ssh -R` 反向隧道 |
| 企业员工访问内网应用 | **ZTNA / SASE** | 传统 VPN + MFA、Tailscale + ACL |
| 多分支机构互联 | **SD-WAN / Site-to-Site VPN** | WireGuard 全网状 |
| 只访问 DSH | **DSH Remote** | SSH 主机 |
| 有公网 IPv6 | **IPv6 直连 + 防火墙** | 仍然保留 Overlay/Relay 兜底 |

## 四、三句话总结

1. **DSH Go 本机 Linux**：不是虚拟机，是 PRoot + Ubuntu glibc；CPU 接近原生，I/O 有开销；和 Termux + proot-distro 同类，但更重、更专用。
2. **DSH Go 远程**：SSH 是直连 + 本地端口转发，需要电脑可达；DSH Remote 是应用层 P2P NAT 穿越 + TURN/Relay 兜底 + Noise E2EE，不需要公网 IP。
3. **通用远程访问**：内网穿透开一扇门，虚拟组网修一条路，ZTNA 只发一张门禁卡；现代最佳实践是“身份 + 出站 + P2P 优先 + 中继兜底 + E2EE + 最小权限”。

## 五、术语总表

| 术语 | 大白话 |
|---|---|
| PRoot | 用户态的“假 chroot”：用 ptrace 拦截系统调用、改写路径，不需要 root |
| ptrace | Linux 的进程跟踪接口；PRoot 靠它“偷看/修改”系统调用 |
| seccomp | Linux 的系统调用过滤机制；PRoot 用它加速，只拦需要的调用 |
| Bionic | Android 自己的 C 标准库；Termux 原生程序用它 |
| glibc | 标准 Linux 的 C 标准库；Ubuntu 程序用它 |
| rootfs | 根文件系统；这里指解压到 App 私有目录的 Ubuntu 用户态 |
| 假 root | PRoot 把 guest 的 uid 映射成 0，但真实进程仍是 App UID |
| NAT | 把私有 IP/端口翻译成公网 IP/端口 |
| NAT 穿透 | 让 NAT 后的设备直接建立连接；不是“组网” |
| STUN | 查“我在公网长什么样”，不转发数据 |
| TURN | 打洞失败时的中继服务器，流量经它转发 |
| ICE | 收集候选地址、测试连通性、选最优路径 |
| Hole punching | 双方同时向对方公网地址发包，让 NAT 建立映射 |
| CGNAT | 运营商级 NAT；很多手机/宽带没有独立公网 IP |
| Overlay | 在物理网络之上建的虚拟网络 |
| DERP | Tailscale 的中继，走 HTTPS/WebSocket，转发 WireGuard 密文 |
| ZTNA | 零信任应用访问：身份认证 + 应用级代理 |
| SD-WAN | 企业多链路、策略路由、站点互联 |
| SASE | SD-WAN + 云安全（ZTNA、SWG、CASB、FWaaS） |
| E2EE | 端到端加密；服务器只能看到密文 |
| Noise IK | 一种轻量握手协议；DSH Remote 用 X25519 + ChaChaPoly + SHA256 |
| TOFU | Trust On First Use：首次信任，之后比对指纹 |

## 六、自测

> [!question]- 自测
> 1. PRoot 是虚拟机吗？为什么？
> 2. 为什么 DSH Go 启动比 Termux 原生慢？
> 3. Direct 模式为什么默认关闭？
> 4. SSH 模式为什么不是内网穿透？
> 5. DSH Remote 的 P2P 失败后走什么？
> 6. 为什么说 DSH Remote 不是异地组网？
> 7. 内网穿透和虚拟组网最大的区别是什么？
> 8. 为什么 P2P 打洞不能保证 100% 成功？
>
> 答案：1. 不是。没有 guest 内核/指令模拟，系统调用仍进入 Android 内核，PRoot 只做路径翻译和部分模拟。2. 因为 DSH 启动要发起约 2.8 万次路径类系统调用，每次都被 ptrace 拦下来；I/O 和进程启动是主要开销。3. 部分机型（Android 16 / x86_64）的 seccomp 会杀掉 glibc loader；默认关闭并失败回退 PRoot 更稳。4. 它没有公网端口映射/中继，只是手机直连电脑 SSH，再把远端回环端口转发到手机；跨网需要电脑本身可达。5. 先尝试 TURN 中继，再回落到 WebSocket Relay。6. 它没有虚拟网卡/虚拟 IP，只隧道 DSH API，不能访问电脑上的其他端口。7. 内网穿透通常只暴露单个服务/端口；虚拟组网让设备像在同一 LAN，所有 IP/端口可达。8. NAT 类型（symmetric）、CGNAT、双层 NAT、防火墙封 UDP 都会让打洞失败，所以需要 TURN/Relay 兜底。

## 关联

[[somezhishi/somezhishi|somezhishi]] · [[somezhishi/工具速查/DSHGo-本机Linux与PRoot|DSH Go 本机 Linux 与 PRoot]] · [[somezhishi/工具速查/DSHGo-远程连接机制|DSH Go 远程连接机制]] · [[somezhishi/工具速查/远程访问-内网穿透与虚拟组网|远程访问：内网穿透与虚拟组网]] · [[somezhishi/工具速查/远程访问-内网穿透策略|远程访问：内网穿透策略]] · [[somezhishi/工具速查/远程访问-虚拟组网与ZTNA|远程访问：虚拟组网与 ZTNA]] · [[somezhishi/工具速查/内网穿透-反向SSH远程WSL|内网穿透：反向 SSH 远程 WSL]]
