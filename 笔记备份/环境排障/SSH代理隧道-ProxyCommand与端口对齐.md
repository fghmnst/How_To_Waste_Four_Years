---
date: 2026-11-11
tags: [排障, ssh, wsl]
---

# SSH 代理隧道：ProxyCommand 与端口对齐

> [!note] 一句话结论
> `ProxyCommand` 的作用是**把"怎么连上目标服务器"这一步外包给一条本地命令**。它最容易出问题的地方不是语法，而是**端口对不上**——配置里写的端口必须等于代理客户端实际监听的端口，差一位就是 `Connection refused`。

## 一、先建立画面：SSH 只要一根"能读能写的管子"

SSH 是加密的远程协议。它干活前只做一件事：**在本地和目标服务器之间拉一根双向管道**，然后把加密字节灌进去。默认这根管子由 SSH 自己开（自己建 TCP 连接）。

`ProxyCommand` 就是把"拉管子"这个动作**换人做**：

```
ssh  ──写──▶  [本地程序（如 nc）的标准输入]
          [本地程序] ──自己想办法──▶ 目标服务器:22
ssh  ◀─读──  [本地程序的标准输出]
```

- SSH 只看到"一条能读能写的管子"，**完全不关心中间经过了谁**；
- 那个本地程序的 **stdout 当作发、stdin 当作收**；
- 所以任何"能把字节送到目标端口"的命令都能塞进来——nc、ssh 自己、甚至 `openssl s_client`。

> [!tip] 这个设计的好处
> 代理、跳板机、特殊协议封装，全都变成"换一条命令"，SSH 本身不用改。代价是这条命令**会真的被执行**，所以别人给的 config 不能盲信。

## 二、逐段拆解典型配置

```
ProxyCommand nc -X connect -x 127.0.0.1:<代理端口> %h %p
```

| 片段 | 含义 |
|---|---|
| `ProxyCommand` | 告诉 SSH："别自己连，跑后面这条命令，用它的输入输出当通道" |
| `nc` | netcat，一把"手动接管管道"的瑞士军刀 |
| `-X connect` | 用 **HTTP CONNECT** 方式跟代理说话（像浏览器过公司代理："请帮我把这条连接转给 X:22"） |
| `-X 5` | 换成 SOCKS5 方式（另一种常见值） |
| `-x <地址:端口>` | **代理本身**在哪。这里填代理程序监听的地址，**不是**目标服务器的地址 |
| `%h` | SSH 自动替换成该 Host 块的 `HostName`（目标主机） |
| `%p` | SSH 自动替换成目标的 `Port`（没写就是 22） |

一句话读法：**不要直连服务器，先连本机代理，让代理替我建立到目标 22 端口的连接。**

> [!warning] 版本坑：`nc` 有两副面孔
> Debian/Ubuntu 默认装的是 `netcat-traditional`，**没有 `-X` / `-x`**，跑起来报 `nc: invalid option -- X`。
> 解法：`sudo apt install netcat-openbsd`，装完 `nc -h` 能看到 `-X` 才对。

## 三、SSH 提供的替换符（不用背）

| 符号 | 会变成 |
|---|---|
| `%h` | 目标主机名（`HostName`） |
| `%p` | 目标端口（`Port`，默认 22） |
| `%r` | 远端用户名（`User`） |
| `%n` | 你在 `ssh` 后面敲的原始别名 |

关键区别：`%p` 是**占位符**，会跟着配置里的 `Port` 自动变；而 `-x` 后面那个代理端口是**写死的字面量**，SSH 不会去猜代理客户端监听在哪。这就是"目标端口能自动跟随、代理端口必须手改"的原因。

## 四、三种"过中间人"的写法

| 写法 | 适用场景 | 特点 |
|---|---|---|
| `ProxyCommand nc -X connect -x <代理> %h %p` | 本地跑着 HTTP/SOCKS 代理（Clash 系等） | 依赖本地端口，端口一变就断 |
| `ProxyCommand ssh -W %h:%p <跳板机>` | 经跳板机/堡垒机中转 | 老写法，`-W` 表示把流量原样转出去 |
| `ProxyJump <跳板机>`（或 `ssh -J`） | 同上，OpenSSH ≥ 7.3 | **推荐**：一行搞定，少一层 shell |

现阶段没有跳板机，就只能用第一种：**手动保证配置端口 = 代理监听端口**。

## 五、为什么 WSL 里非要用它

### 原因 1：SSH 不读 `http_proxy`

`http_proxy` / `https_proxy` 只有 HTTP 客户端（curl、浏览器、pip、npm）会读。**SSH 协议完全不看这些环境变量**，git 走 SSH 时也一样。所以会出现"浏览器和 curl 都能开外网，`git pull` 却挂死"的怪象——它们走的根本不是同一条路。

### 原因 2：WSL 和 Windows 不是同一个网络栈

`127.0.0.1` 的含义是"**我自己这台机器**"——而 WSL 里的"我自己"是 WSL，Windows 里的"我自己"是 Windows。两者能否互通取决于 WSL 网络模式：

| 模式 | WSL 里的 `127.0.0.1` 能到 Windows 的代理吗 | 配置里该填什么 |
|---|---|---|
| `NAT`（旧默认） | ❌ 不能，`127.0.0.1` 只指 WSL 自己 | Windows 主机 IP（网关地址），或另做端口转发 |
| `mirrored`（新默认） | ✅ 能，两边共享 loopback | `127.0.0.1` 即可 |

### 本机实测（只读）

```bash
ss -ltn | grep <代理端口>       # 看 WSL 内有没有程序在监听这个端口
wslinfo --networking-mode       # 看当前 WSL 网络模式
```

结果：**WSL 内无监听**，网络模式是 **mirrored**。说明代理跑在 **Windows 侧**，WSL 靠 mirrored 共享 loopback 才连得上。

> [!warning] 这条依赖很脆
> 如果在 Windows 的 `.wslconfig` 里把网络模式改回 `nat`，`127.0.0.1:<端口>` 会立刻失效——但报错表现和"代理没开"一模一样，极易误判。

## 六、改代理端口会发生什么

| 你改了什么 | 结果 |
|---|---|
| **只改 config 的端口**，代理还在原端口 | 连一个没人监听的端口 → `nc: connect to 127.0.0.1 port X failed: Connection refused` → SSH 报 `ProxyCommand ... exited with status 1` 或 `kex_exchange_identification: Connection closed by remote host` |
| **只改代理的端口**，config 还是旧的 | 症状完全一样，方向相反 |
| **两处改成同一个新端口** | 正常，`-X connect` 只要求对端是 HTTP CONNECT 代理，不关心端口号 |

不会发生的事：改这个端口**不影响**目标服务器地址和 22 端口（那是 `HostName` / `Port`），**不影响**其他 Host 块，也**不影响**密钥认证。

### 最坑的一点：端口和协议是绑定的

Clash 系客户端通常同时开三个口：Mixed、HTTP、SOCKS5。把端口改成一个**纯 SOCKS5 口**，却不同时把 `-X connect` 改成 `-X 5`，照样连不上——报文格式对不上，代理直接掐断。这是"端口明明对了却不通"的经典原因。

另外两个现实风险：

- **客户端升级会换默认端口**（Clash Verge Rev 与老版 Clash for Windows 的默认值不同）。"昨天还好好的"多半来自这里。
- **端口可能被占用**，那报错在代理侧：`address already in use`，SSH 这边只会看到连不上。

### 安全边界：别把 `127.0.0.1` 改成 `0.0.0.0`

`127.0.0.1` = 只有本机能连。改成 `0.0.0.0` = **对整个局域网开放代理**，谁扫到都能拿你的出口上网。这常和"改端口"被一起改掉，所以提前说清。

## 七、排障四步（每步都只读）

```bash
# ① 看代理命令到底被执行成什么
ssh -v <别名> 2>&1 | grep -i "proxy command"

# ② 单独测代理那扇门开没开（只要不是 Connection refused 就算通）
timeout 3 nc -vz 127.0.0.1 <代理端口>

# ③ 临时绕开代理做对比：直连能通 → 问题在代理侧
ssh -o ProxyCommand=none <别名>

# ④ 确认测的是配置本身，而不是复用了旧连接
ssh -o ControlMaster=no -o ControlPath=none <别名>
```

按报错定位：

| 现象 | 多半是 |
|---|---|
| `nc: invalid option -- X` | 装的是 netcat-traditional，换 netcat-openbsd |
| `Connection refused` | 代理没开，或端口写错 |
| 静默卡住 / `timeout` | 代理开着但出口规则不允许该目标；或网络模式被改成了 `nat` |
| 换网络后突然不通 | 代理地址原本属于另一侧（Windows ↔ WSL），地址需随之改 |

> [!tip] 改配置前的保险动作
> `cp ~/.ssh/config ~/.ssh/config.bak-$(date +%Y%m%d-%H%M%S)`，出问题直接拷回去。

## 八、术语小词典

| 术语 | 大白话 |
|---|---|
| 代理（proxy） | 中转站：流量先发给它，由它代你访问目标，再把结果带回来 |
| 隧道（tunnel） | 把一种流量装进另一条连接里搬运，外观上看是一条直连 |
| `ProxyCommand` | "建立连接这件事，交给这条命令去做" |
| 跳板机（jump host） | 一台你可登录的中间服务器，由它替你连内网目标 |
| `ProxyJump` | 跳板机的现代写法，等价于老式 `ProxyCommand ssh -W` |
| `%h` / `%p` | 目标主机 / 目标端口的占位符，SSH 执行时自动填 |
| HTTP CONNECT | 代理协议里"请帮我把这条连接转给某地址"的请求方式 |
| SOCKS5 | 另一种更底层的代理协议，能转发任意 TCP/UDP |
| loopback / `127.0.0.1` | "本机自己"，但**是哪台机器取决于进程在哪运行** |
| `mirrored` 网络模式 | WSL 与 Windows 共享网络栈（含 loopback），`127.0.0.1` 两边等价 |

> [!question]- 自测：不看正文，能答上来吗？
> 1. `ProxyCommand` 把 SSH 的哪一步替换掉了？替换后的程序靠什么和 SSH 交换数据？
> 2. `-x` 后面填的是目标服务器地址，还是代理地址？
> 3. 为什么改了服务器端口 `%p` 会自动跟着变，代理端口却必须手改？
> 4. 把 `-X connect` 配在一个纯 SOCKS5 端口上，会发生什么？
> 5. WSL 从 `mirrored` 改回 `nat`，这个配置会以什么症状失败？
> 6. 为什么"curl 能上网"不能证明 SSH 也能上网？

## 关联

[[somezhishi/环境排障/GitHub-不挂代理也能推送-原因与排查|GitHub 不挂代理也能推送]] · [[somezhishi/工具速查/内网穿透-反向SSH远程WSL|内网穿透：反向 SSH 远程 WSL]] · [[somezhishi/编程基础/SSH密钥-修改口令与ssh-keygen参数详解|SSH 密钥]] · [[somezhishi/somezhishi|somezhishi]]
