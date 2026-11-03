---
date: 2026-11-04
tags: [指南, ssh, 工具]
---

# DSHGo-远程连接机制

> 用途：搞清 DSH Go 怎么远程连电脑，以及它到底算不算“内网穿透 / 异地组网”。
> 配套：本机环境见 [[somezhishi/工具速查/DSHGo-本机Linux与PRoot|DSH Go 本机 Linux 与 PRoot]]；通用策略见 [[somezhishi/工具速查/远程访问-内网穿透与虚拟组网|远程访问：内网穿透与虚拟组网]]。
> 结论先行：**两条路——SSH 直连 + 本地端口转发；DSH Remote 是“应用层远程访问 + NAT 穿越 + 中继回退 + 端到端加密”。它不叫 NAT 组网。**

## 一、两条远程路径

| 路径 | 本质 | 是否经过第三方 | 适合 |
|---|---|---|---|
| SSH 主机 | 手机直连电脑 SSH，在远端启动 `dsh web`，再把远端回环端口转发到手机 | 否 | 同 LAN、有公网 IP、已有 VPN/异地组网 |
| DSH Remote | 电脑插件出站连服务器，手机登录同账号；WebRTC P2P/TURN 或 WebSocket Relay 承载 | 是（默认托管服务器；可自建） | 电脑在 NAT 后、没有公网 IP |

## 二、SSH 主机：直连 + 本地端口转发

### 1. 流程

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

### 2. 适用与限制

- 同 Wi-Fi、家里/公司 LAN：最直接。
- 有公网 IP 或 VPS：直接连。
- 已有 Tailscale/ZeroTier/WireGuard：在其上跑 SSH，等于借了对方的组网能力。
- **没有内建 NAT 穿透**：电脑在 CGNAT/双层 NAT 后、又没有公网入口时，SSH 模式默认连不上。
- 不是“内网穿透”：没有公网端口映射，也没有中继；它就是“能连上 SSH 就能用”。

## 三、DSH Remote：账号制、应用层 P2P/中继

### 1. 流程

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

### 2. 适用与限制

- 电脑在 NAT 后、没有公网 IP：首选。
- 手机用蜂窝网络、电脑在家：P2P 失败时自动 TURN/Relay。
- 只想远程用 DSH：功能刚好；要远程桌面/文件共享则不够。
- 限制：一次只连一台主机；Relay 模式带宽/大小有限；App 内终端在 DSH Remote 下不可用；只支持图片附件等。

## 四、SSH 与 DSH Remote 对比

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

## 五、它到底算哪一类？

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

## 六、安全与验证边界

### 安全模型

- SSH：主机指纹 TOFU pin；密码/私钥存 Android Keystore；不经过第三方。
- DSH Remote：Noise IK E2EE；服务器只看到连接元数据和密文；Relay 票据只走数据 socket；Host 身份指纹首次确认后 pin。
- **首次连接依赖服务器分发的 Host 公钥**：恶意服务器理论上可在首次接触做中间人；指纹只有 48 bit；之后 pin 失败关闭。
- 账号被盗等于 Host 被访问；这是上游模型本身的边界。
- 自建 Server 时，最小开源版没有 WebRTC/TURN，且需要自己配 HTTPS/WSS 反向代理。

### 验证边界（仓库自己写明）

- LAN/P2P 在 Android 35 x86_64 模拟器上真实跑通过；
- **真实公网 NAT、真实 TURN、移动网络、OEM 真机尚未验证**；
- “P2P”在模拟器同宿主环境下的成功，不等于所有公网 NAT 都能打洞；
- Relay 模式是确定能用的兜底，但带宽/大小有限。

## 七、怎么选

- 电脑在同一 Wi-Fi 或有公网 IP/VPS：**SSH 主机**，最直接，不经过第三方。
- 电脑在 NAT 后、没有公网 IP：**DSH Remote**。
- 要访问电脑上的所有服务（SMB/RDP/数据库）：**另装 Tailscale/ZeroTier/WireGuard，再在其上跑 SSH/RDP**；DSH Remote 不做通用网络。
- 只远程用 DSH：DSH Remote 足够；接受它的功能限制。

## 术语小词典

| 术语 | 大白话 |
|---|---|
| NAT 穿透 | 让 NAT 后面的两台设备直接建立连接；不是“组网” |
| STUN | 告诉客户端“你在公网看起来是什么 IP:端口”，不转发数据 |
| TURN | 打洞失败时的中继服务器；流量经它转发 |
| ICE | 收集候选地址、逐一测试连通性、选最优路径的框架 |
| WebRTC DataChannel | WebRTC 里的任意数据通道；DSH Remote 用它承载密文 |
| Relay | 应用层中继；DSH Remote 的最终兜底 |
| E2EE | 端到端加密；服务器只能看到密文 |
| Noise IK | 一种轻量握手协议；这里用 X25519 + ChaChaPoly + SHA256 |
| TOFU | Trust On First Use：首次信任，之后比对指纹 |
| 控制面 / 数据面 | 控制面负责信令/协调；数据面负责真正传输 |

> [!question]- 自测
> 1. SSH 模式为什么不是内网穿透？
> 2. DSH Remote 的 P2P 失败后走什么？
> 3. 为什么说它不是异地组网？
> 4. DSH Remote 的服务器能看到聊天内容吗？
>
> 答案：1. 它没有公网端口映射/中继，只是手机直连电脑 SSH，再把远端回环端口转发到手机；跨网需要电脑本身可达。2. 先尝试 TURN 中继，再回落到 WebSocket Relay。3. 它没有虚拟网卡/虚拟 IP，只隧道 DSH API，不能访问电脑上的其他端口。4. 不能。业务数据用 Noise IK 端到端加密，服务器只看到元数据和密文；但首次连接信任依赖服务器分发的公钥，存在 TOFU 边界。

## 关联

[[somezhishi/somezhishi|somezhishi]] · [[somezhishi/工具速查/DSHGo-本机Linux与PRoot|DSH Go 本机 Linux 与 PRoot]] · [[somezhishi/工具速查/远程访问-内网穿透与虚拟组网|远程访问：内网穿透与虚拟组网]] · [[somezhishi/工具速查/内网穿透-反向SSH远程WSL|内网穿透：反向 SSH 远程 WSL]]
