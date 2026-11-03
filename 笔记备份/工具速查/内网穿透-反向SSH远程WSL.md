---
date: 2026-10-10
tags: [指南, ssh, wsl, 工具]
---

# 内网穿透-反向SSH远程WSL

> 用途：在外网（校园/移动网络）用 SSH 直连家里电脑的 WSL——无公网 IP、零服务端改动、手工开关。
> 配套：平板侧的 Termux 基础配置见 [[somezhishi/工具速查/Termux-平板环境搭建清单|Termux 平板环境搭建清单]]。
> 状态：2026-10-10 实装并通过端到端验证（隧道：`wsl-revtunnel` 用户级 systemd 服务）。

## 一、目标与总体架构

```
平板(任意网络) --①ssh 密钥--> server2 --②回环转发 127.0.0.1:2222--> WSL sshd:22
```

- **server2** = 有公网入口的中转机（别名 `server2`，连接信息见 `~/.ssh/config`，不落文）；
- **隧道由 WSL 主动拨出**：不存在"外面连进来"的动作，全部连接都是"从里往外"建立的；
- 平板用 `ProxyJump` 先登 server2，再经它的回环端口穿透到 WSL。

## 二、实现原理（四个"为什么"）

### 1. 为什么需要"反向"隧道

家庭宽带通常没有公网 IP（或在运营商 NAT/CGNAT 后面），外网**无法主动拨入**你家网络。唯一的方向是反过来：让 WSL 主动连到有公网入口的 server2，并在**这条已经建立的 SSH 连接上**开一个转发端口（`ssh -R`）——把"外面连不进来"变成"里面送出去"。

### 2. 为什么隧道端口必须绑回环（127.0.0.1）

`ssh -R 2222:localhost:22` 会让 **server2 上真实打开一个监听端口**：

- 若绑 `0.0.0.0`（即 `GatewayPorts yes`）：等于把你 WSL 的 SSH 直接摆在公网上，扫描器几分钟内就会来爆破；
- 绑 `127.0.0.1`（默认）：只有**已经能操作 server2** 的流量才够得到它（平板经 ProxyJump 也算"能操作 server2"），公网碰不到。

一句话：**隧道解决可达性，回环解决安全性**。这台 server2 实测未开启 GatewayPorts，天然就是回环绑定。

### 3. "server2 访问自己"是怎么回事

`-R` 的监听 socket 生在 **server2 本机**（由 server2 的 sshd 持有）。任何使用它的连接，最终都由 server2 本机发起：

- 平板 `ProxyJump server2` 时：平板 ssh 客户端在会话里发 `direct-tcpip` 请求——"请帮我连 127.0.0.1:2222"——**由 server2 的 sshd 在本机发起这个 TCP 连接**，接上隧道口后，数据沿隧道回传，最终由 WSL 端转交 `localhost:22`；
- 这**不是**额外绕路，而是"只绑回环"安全设计的直接结果：入口开在 server2 上，谁用都得从 server2 本机够到它。

### 4. mirrored 模式与 NAT 的关系（本机是 mirrored）

- 本机 WSL 网络模式实测为 **mirrored**（`wslinfo --networking-mode`）：WSL 与 Windows **共享同一套网络接口和 IP**，**没有 NAT**，localhost 双向互通；
- 所以隧道的原因**不是**"WSL 在 NAT 后"，而是"家里网络整体没有对外入口"；
- mirrored 的两个影响：
  1. sshd 若监听 `0.0.0.0` 会挂在 Windows 的接口上（局域网理论上可达，靠 Windows 防火墙默认拦入站兜底）→ 建议收紧到 `127.0.0.1`（对隧道无影响）；
  2. 因为是 **socket 激活**（`ssh.socket` 在管），收紧要用 socket drop-in，改 `sshd_config` 的 `ListenAddress` 不生效。

## 三、落地全过程（2026-10-10 实测）

### 前置侦察（决定方案的事实）

- WSL：`ssh.socket` 已启用且 22 在听（无需装/启 sshd）；host key 齐全；`~/.ssh/authorized_keys` 原本不存在；
- server2：2222/22022 端口空闲；未限制 `AllowTcpForwarding`（默认允许）；未开 `GatewayPorts`（默认仅回环）。

### Step 0 · 平板生成密钥

```bash
ssh-keygen -t ed25519 -C tablet-hw     # 一路回车（不设口令）；-t 密钥类型，-C 注释
cat ~/.ssh/id_ed25519.pub              # 打印公钥（公开信息），整行复制
```

> 坑：贴/用时**必须带 `ssh-ed25519 ` 类型前缀**（见"常见坑"第 1 条）。

### Step 1 · WSL 侧

**1a 写入平板公钥**（我执行）：

```bash
mkdir -p ~/.ssh && chmod 700 ~/.ssh
echo '<平板公钥整行>' >> ~/.ssh/authorized_keys   # 先查重再追加
chmod 600 ~/.ssh/authorized_keys                    # 权限过宽 sshd 直接拒用
```

**1b 加固（需 sudo，推荐）**：

```bash
sudo mkdir -p /etc/systemd/system/ssh.socket.d
sudo tee /etc/systemd/system/ssh.socket.d/10-listen.conf >/dev/null <<'EOF'
[Socket]
ListenStream=
ListenStream=127.0.0.1:22
EOF
sudo tee /etc/ssh/sshd_config.d/99-local.conf >/dev/null <<'EOF'
PasswordAuthentication no
KbdInteractiveAuthentication no
EOF
sudo systemctl daemon-reload && sudo systemctl restart ssh.socket
ss -tln | grep ':22'    # 期望：只剩 127.0.0.1:22
```

- `ListenStream=`（空行）清掉继承的 `0.0.0.0:22`，下一行只加回环；
- 第二段 drop-in 全面禁密码，只认密钥。

**1c 用户级 systemd 隧道单元**（我执行，路径 `~/.config/systemd/user/wsl-revtunnel.service`）：

```ini
[Unit]
Description=Reverse SSH tunnel (server2 loopback -> WSL:22)

[Service]
ExecStart=/usr/bin/ssh -N -o BatchMode=yes -o ExitOnForwardFailure=yes -o ServerAliveInterval=30 -o ServerAliveCountMax=3 -o ControlMaster=no -o ControlPath=none -R 2222:localhost:22 server2
Restart=always
RestartSec=5

[Install]
WantedBy=default.target
```

参数逐项：`-N` 不开远程 shell；`-R 2222:localhost:22` 在 server2 监听回环 2222 转到 WSL 22；`BatchMode` 禁交互（systemd 无终端）；`ExitOnForwardFailure` 转发失败立即退出（避免"假在线"）；`ServerAlive*/CountMax` 90 秒判定断线触发重连；`ControlMaster=no/ControlPath=none` 独立连接、不与日常 ssh 复用互踩；`Restart=always` 掉线 5 秒自动拉起。

**1d 启动与验证**：

```bash
systemctl --user daemon-reload
systemctl --user start wsl-revtunnel
systemctl --user status wsl-revtunnel          # 期望 active (running)
ssh server2 'ss -tln | grep 2222'              # 期望 127.0.0.1:2222（与 [::1]:2222）
```

### Step 2 · server2 侧验证（零改动）

```bash
ssh server2 'ss -tln | grep 2222'                                   # 看监听
ssh server2 "timeout 3 bash -c '</dev/tcp/127.0.0.1/2222' && echo open"   # 纯 TCP 连通测试
```

### Step 3 · 平板配置与首连

`~/.ssh/config`：

```text
Host server2
    HostName <从 WSL 的 ~/.ssh/config 抄，值不落仓库>
    User <同上>
    IdentityFile ~/.ssh/id_ed25519

Host wslbox
    HostName 127.0.0.1
    Port 2222
    User <WSL 用户名，与 WSL 上 whoami 一致>
    ProxyJump server2
    IdentityFile ~/.ssh/id_ed25519
    ServerAliveInterval 30
```

```bash
chmod 600 ~/.ssh/config
ssh wslbox        # 首连会问 WSL 的 host key（不是 server2 的）→ 核对指纹后 yes
```

WSL sshd 的 ed25519 指纹（首连核对用，2026-10-10）：
`SHA256:57gwJPMcCjDETqRLTOk1fTsTSVLjIsF1syWi+WM/Flo`

### 端到端验证记录

在 WSL 上跑 `ssh -J server2 -p 2222 <WSL 用户名>@127.0.0.1 true`，报 `Permission denied (publickey)` = **已抵达 WSL sshd 的认证阶段，整条链路通**（失败只是因为 WSL 本机没有自己的授权密钥，属预期）。

## 四、常见坑

1. **公钥缺 `ssh-ed25519 ` 前缀**（本机实际遇到）：直接贴 base64 部分会变成无效行被 sshd 静默忽略——`authorized_keys` 每行必须是"类型 + base64 + 注释"完整格式；
2. **权限**：`~/.ssh` 必须 700、`authorized_keys` 600，否则 sshd 拒用（且往往不报明显错误）；
3. **私钥有口令**：会卡在 `BatchMode` 预检——隧道用**无口令专用密钥**或改走 ssh-agent；
4. **ControlMaster 冲突**：日常 ssh 配置里的连接复用会和新隧道互踩——unit 里显式 `ControlMaster=no -o ControlPath=none`；
5. **socket 激活的坑**：本机 sshd 由 `ssh.socket` 管，"改 sshd_config 的 ListenAddress"不生效，必须用 `ssh.socket.d` drop-in（且要 `ListenStream=` 先清空再重设）；
6. **mirrored 模式**：`0.0.0.0` 监听会挂到 Windows 接口上，防火墙"默认拦"不等于"绝对安全"——收紧回环一劳永逸；
7. **鸡生蛋**：手动模式下必须"出门前开好"；电脑重启/WSL 关闭后隧道不会自己回来（续命方案见"可选升级"）；
8. **电脑睡眠/关机 = 不可达**（设计使然，不是故障）；
9. **systemd degraded**：动手前先 `systemctl --failed` / `systemctl --user --failed` 看看有没有别的失败单元在捣乱；
10. **端口冲突**：2222 被占就换 22022（unit 与平板 config 同步改）；
11. **首连指纹**：`ssh wslbox` 首次的 host key 是 **WSL** 的（流量经两跳但认证终点在 WSL）——和 server2 的指纹不同是正常的；
12. **`Host key verification failed`**：在 BatchMode 下第一次连隧道端口时，因为本地 known_hosts 没有 `[127.0.0.1]:2222` 条目——正常现象，交互式连接里核对指纹输 yes 即可。

## 五、常用指令速查

### 平板侧（日常入口）

```bash
ssh wslbox                      # 进 WSL
ssh wslbox 'uname -a; whoami'   # 单发命令
scp 本地文件 wslbox:~/          # 拷文件去 WSL
scp wslbox:~/笔记.md .          # 从 WSL 拷回来
# 断线自动重连：config 里已配 ServerAliveInterval 30
```

### WSL 侧（隧道开关与体检）

```bash
systemctl --user start wsl-revtunnel     # 出门前开
systemctl --user stop  wsl-revtunnel     # 回来后关
systemctl --user status wsl-revtunnel    # 看状态（active 才算通）
journalctl --user -u wsl-revtunnel -n 50 # 看日志排障
ssh server2 'ss -tln | grep 2222'        # 从 WSL 验 server2 回环端口
ssh-keyscan -t ed25519 127.0.0.1 | ssh-keygen -lf -   # 取 WSL 主机指纹
```

### server2 侧（只读验证）

```bash
ss -tln | grep 2222                                        # 期望 127.0.0.1:2222
timeout 3 bash -c '</dev/tcp/127.0.0.1/2222' && echo open  # TCP 连通测试
```

## 六、可选升级

- **开机自启**：`sudo loginctl enable-linger <WSL 用户名>` + `systemctl --user enable wsl-revtunnel` + Windows 任务计划（登录时 `wsl.exe -d <发行版> -e true`）；
- **frp 升级**（触发信号：断连频繁 / 想复用多服务 / 想要管理面板）：frps 放 server2 + frpc 放 WSL，均 systemd 托管，安全模型不变（只绑回环 + ProxyJump）；
- **家里局域网直连**：mirrored 模式下 WSL 与 Windows 同网段，防火墙放行后同 Wi-Fi 设备可直连（本方案暂不需要）。

## 术语小词典

| 术语 | 大白话 |
|---|---|
| 反向隧道 | 由内网机器主动拨出到公网机器、并在对方机器上"开一个口子"的转发方式（`ssh -R`） |
| 回环 / 127.0.0.1 | 只在本机内部可用的地址；绑回环 = 这个端口不对公网开放 |
| GatewayPorts | sshd 的开关：控制 `-R` 的监听端口绑所有网卡（yes）还是仅回环（no） |
| ProxyJump | "跳板"：先连 A，再从 A 出去连 B（`-J`） |
| direct-tcpip | SSH 协议里的"帮我连某地址"通道请求，ProxyJump 全靠它落地 |
| socket 激活 | 由 systemd 的 `.socket` 监听端口、有人连接才拉起服务的机制 |
| mirrored 模式 | WSL 与 Windows 共享网络接口/IP（无 NAT）；相对的是默认的 NAT 模式 |
| 跳板机 | 有公网入口、用于中转的服务器（这里是 server2） |

> [!question]- 自测
> 1. 为什么隧道端口要绑 127.0.0.1 而不是 0.0.0.0？
> 2. 平板 `ProxyJump server2` 时，"连 127.0.0.1:2222"这个 TCP 连接是谁发起的？
> 3. mirrored 模式下还需要反向隧道吗？为什么？
> 4. 平板首连 `ssh wslbox` 看到的 host key 是谁的？
>
> 答案：1. 绑 0.0.0.0 等于把 WSL 的 SSH 暴露给公网扫描器；回环保证只有能操作 server2 的流量可达。2. server2 的 sshd 在本机发起（direct-tcpip 请求），"server2 连自己"是链路内生环节。3. 需要。原因不是 WSL 的 NAT，而是家里网络整体没有对外入口（无公网 IP/CGNAT）。4. WSL 的（认证终点在 WSL，server2 只是跳板）。

## 关联

[[somezhishi/somezhishi|somezhishi]] · [[somezhishi/工具速查/Termux-平板环境搭建清单|Termux 平板环境搭建清单]] · [[somezhishi/工具速查/Termux-入门与核心概念|Termux 入门与核心概念]] · [[somezhishi/工具速查/远程访问-内网穿透与虚拟组网|远程访问：内网穿透与虚拟组网]] · [[somezhishi/工具速查/DSHGo-远程连接机制|DSH Go 远程连接机制]]
