---
date: 2026-11-04
tags: [指南, 工具]
---

# 远程访问-虚拟组网与ZTNA

> 用途：把“异地组网”讲清楚：WireGuard、Mesh Overlay、Site-to-Site VPN、SD-WAN、ZTNA 各自是什么、怎么选。
> 配套：总览与选型见 [[somezhishi/工具速查/远程访问-内网穿透与虚拟组网|远程访问：内网穿透与虚拟组网]]；内网穿透细节见 [[somezhishi/工具速查/远程访问-内网穿透策略|远程访问：内网穿透策略]]。
> 结论先行：**虚拟组网是“修一条路”，让设备像在同一 LAN；ZTNA 是“只发一张指定门的门禁卡”，只开放单个应用。**

## 一、WireGuard 点对点 / Hub-and-Spoke

```text
手机 ── WireGuard 隧道 ──→ VPS（Hub）──→ 家里电脑
                                    └──→ 公司服务器
```

每台设备装 WireGuard，配置对端公钥和 Endpoint；所有流量走加密隧道。Hub-and-Spoke 就是用一个有公网 IP 的 VPS 做中心节点。

**优点：**

- 快、加密强、配置透明、可路由整个网段（AllowedIPs）。
- 内核态/用户态实现都很快，协议简单，代码量小。
- 适合自建、可控。

**缺点：**

- **本身没有 NAT 穿透/协调能力**：需要公网 Endpoint 或静态配置。
- Hub 是单点/带宽瓶颈。
- 移动端切换网络要重连。
- 需要自己管理密钥、IP 分配、DNS。

**适用：** 有 VPS、固定公网 IP、少量设备、自己掌控一切的场景。

## 二、Mesh Overlay（Tailscale / ZeroTier / Netbird / Nebula）

这是目前个人/中小企业最主流的“异地组网”方案。

```text
设备 A ──┐
设备 B ──┼── 协调服务器（控制面：身份、密钥、ACL、在线状态）
设备 C ──┘        │
                  ├─ 尽量 P2P 直连（NAT 穿越）
                  └─ 失败时走中继（DERP / Relay）
```

**Tailscale：**

- 基于 WireGuard，L3 虚拟网络。
- 协调服务器负责身份、密钥分发、ACL、设备列表。
- 数据面优先 P2P，失败走 DERP 中继。
- 支持 MagicDNS、子网路由（Subnet Router）、出口节点（Exit Node）、SSO/MFA。
- 有 SaaS，也可用 Headscale 自建控制面。

**ZeroTier：**

- L2 虚拟以太网，支持广播/多播，可桥接物理 LAN。
- 自己的协议，根服务器（Planet）做协调；可自建 Moon。
- 适合需要二层广播的旧应用、LAN 游戏、设备发现。

**Netbird：**

- WireGuard + SSO + ACL，开源，可自建。
- 更偏企业/团队，界面和策略管理较完整。

**Nebula：**

- 证书体系 + Lighthouse 节点，轻量、去中心化，适合自建。
- 没有中心化 SaaS 依赖，适合运维/基础设施团队。

**优点：**

- **一次组网，所有端口/IP 可达**：SSH、RDP、SMB、数据库、Web、游戏都能用。
- 设备有稳定虚拟 IP/DNS，管理方便。
- 通常带 SSO、ACL、审计、密钥轮换。
- P2P 直连时延迟接近原生；失败有中继兜底。
- 跨平台好（手机/桌面/服务器）。

**缺点：**

- **每台设备都要装客户端**，不能像 frp 那样只装服务端。
- 依赖协调服务器/控制面；SaaS 有供应商锁定，自建有运维成本。
- 中继（DERP）有带宽/延迟成本。
- 子网路由、DNS、MTU、ACL 配置有一定学习成本。
- 移动端耗电、后台保活、网络切换需要处理。
- 默认是网络级权限，需要额外 ACL 细化。

**适用：** 远程办公、homelab、家里 NAS、多台服务器、IoT；需要访问多个服务/端口，而不只是单个 Web 应用。

## 三、Site-to-Site VPN / SD-WAN

**Site-to-Site VPN：**

- 用 IPsec/WireGuard 把两个固定网络（总部-分支、云上 VPC-办公室）连起来。
- 适合企业分支、数据中心互联。
- 优点：透明、稳定、所有内网 IP 可达。
- 缺点：需要两端网关、静态配置、运维复杂；不适合个人/移动设备。

**SD-WAN：**

- 不是单纯隧道，而是**多链路（MPLS、宽带、5G）+ 应用感知路由 + 策略 + 安全**。
- 常和 SASE 结合（SD-WAN + ZTNA + SWG + CASB + FWaaS）。
- 优点：链路冗余、体验好、集中策略。
- 缺点：贵、复杂、面向企业；个人用不到。

## 四、ZTNA（零信任应用访问）

ZTNA 不是“组网”，但它是现代远程访问的重要替代方案。

```text
用户 → 身份认证（SSO/MFA/设备姿态）
     → ZTNA 云/网关
       → 出站连接器（部署在内网）
         → 只开放被授权的应用
```

**代表：** Cloudflare Access + Tunnel、Twingate、Teleport、Pomerium、Zscaler Private Access、Tailscale Serve。

**优点：**

- **最小权限**：只开放某个应用，不给整个网络。
- 不暴露入站端口，内网连接器只出站。
- SSO/MFA、设备合规、审计日志。
- 比传统 VPN 更适合远程办公/第三方访问。

**缺点：**

- 按应用接入，不适合“任意 TCP/端口”的场景。
- 依赖供应商/云，可能有延迟和费用。
- 需要部署连接器、配置身份源。
- 对非 HTTP 协议支持有限（视产品而定）。

**和 VPN 的区别：**

- VPN：先连入网络，然后能访问很多内网资源（扁平网络）。
- ZTNA：先验证身份/设备，再只放行某个应用（应用级）。

## 五、虚拟组网 vs 内网穿透

| 维度 | 虚拟组网 | 内网穿透 |
|---|---|---|
| 目标 | 让设备像在同一 LAN | 暴露某个服务/端口 |
| 访问范围 | 所有 IP/端口 | 单服务/单端口/单域名 |
| 网络层 | L3/L2 虚拟网络 | 应用层/TCP/HTTP 隧道 |
| 客户端 | 每台设备都要装 | 通常只需内网侧 agent + 公网服务器 |
| 寻址 | 虚拟 IP / MagicDNS | 公网域名/端口 |
| 性能 | 直连时接近原生，失败走中继 | 走中继时受服务器带宽限制 |
| 安全 | 默认私有，需 ACL/SSO | 公网暴露面，需 TLS/认证 |
| 典型 | Tailscale、ZeroTier、Netbird、WireGuard | frp、ngrok、cloudflared、`ssh -R` |
| 适合 | 远程办公、homelab、多服务/多设备 | Webhook、开发联调、单服务远程访问 |

## 六、怎么选

| 场景 | 推荐 | 理由 |
|---|---|---|
| 远程访问家里所有设备/端口 | Tailscale / Netbird / ZeroTier | 一次组网，所有服务可达 |
| 只给一个 Web 应用 | Cloudflare Tunnel + Access | 不开入站端口、可加 SSO |
| 企业员工访问内网应用 | ZTNA / SASE | 最小权限、审计、替代扁平 VPN |
| 多分支机构互联 | SD-WAN / Site-to-Site VPN | 稳定、策略路由、链路冗余 |
| 临时 SSH/运维 | SSH over Tailscale | 不用暴露 SSH 到公网 |
| 低延迟远程桌面/游戏 | P2P（WebRTC/Parsec/Moonlight）+ 中继 | 直连延迟最低 |

## 术语小词典

| 术语 | 大白话 |
|---|---|
| Overlay | 在物理网络之上建的虚拟网络 |
| WireGuard | 轻量、现代的 VPN 协议；快、加密强 |
| Mesh | 网状：设备之间尽量直连，不都绕中心 |
| DERP | Tailscale 的中继，走 HTTPS/WebSocket，转发 WireGuard 密文 |
| Headscale | 自建的 Tailscale 控制面 |
| ZTNA | 零信任应用访问：身份认证 + 应用级代理 |
| SD-WAN | 企业多链路、策略路由、站点互联 |
| SASE | SD-WAN + 云安全（ZTNA、SWG、CASB、FWaaS） |

> [!question]- 自测
> 1. 虚拟组网和 VPN 是什么关系？
> 2. Tailscale 和 ZeroTier 最大的区别是什么？
> 3. ZTNA 和传统 VPN 最大的区别是什么？
> 4. 为什么虚拟组网通常比内网穿透更适合“访问多个服务”？
>
> 答案：1. 虚拟组网常以 VPN 隧道（如 WireGuard）为数据面，但额外有协调、NAT 穿透、ACL 等能力；不是所有 VPN 都等于 Overlay。2. Tailscale 是 L3/WireGuard，简单、SSO/ACL 强；ZeroTier 是 L2 虚拟以太网，支持广播/多播和桥接。3. VPN 先连入网络、权限大；ZTNA 先验证身份/设备，只放行单个应用。4. 虚拟组网给设备虚拟 IP，所有端口/IP 可达；内网穿透通常只映射一个端口/域名。

## 关联

[[somezhishi/somezhishi|somezhishi]] · [[somezhishi/工具速查/远程访问-内网穿透与虚拟组网|远程访问：内网穿透与虚拟组网]] · [[somezhishi/工具速查/远程访问-内网穿透策略|远程访问：内网穿透策略]] · [[somezhishi/工具速查/DSHGo-远程连接机制|DSH Go 远程连接机制]] · [[somezhishi/工具速查/内网穿透-反向SSH远程WSL|内网穿透：反向 SSH 远程 WSL]]
