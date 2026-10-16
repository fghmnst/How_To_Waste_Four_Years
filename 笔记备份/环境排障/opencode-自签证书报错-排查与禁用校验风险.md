---
date: 2026-09-23
updated: 2026-10-05
tags:
  - 排障
  - opencode
---

# opencode 自签证书报错：排查与禁用校验风险

> [!note] 一句话结论（2026-10-05 修正）
> 报 `self signed certificate in certificate chain` 的根因是**本机卡巴斯基的「加密连接扫描」**：它对 `api.deepseek.com` 做 HTTPS 解密（MITM），用自签的 `Kaspersky Anti-Virus Personal Root Certificate` 重新签发证书，opencode（Bun/Node 系）的信任库里没有这张证书 → 校验失败。**修复：卡巴斯基 设置 → 网络设置 → 加密连接扫描 → 排除项，添加 `api.deepseek.com`（2026-10-05 亲测有效）**。2026-09-23 首次排查时误判为"上游 WAF 占位证书"，复盘见文末。

## 现象

**2026-10-05（稳定复现，本文主线）**：

- opencode 把模型切到 DeepSeek 后**必现**报错：`Error: self signed certificate in certificate chain`
- 同机的 GLM（`api.z.ai`）完全正常——"只有 deepseek 一个域中招"是最重要的线索
- `NODE_TLS_REJECT_UNAUTHORIZED=0 opencode` 能"绕过"（**错误做法，见下文**）

**2026-09-23（首次遭遇，结论已被推翻）**：同样报错，约 95 秒后自愈，当时误判为上游 WAF 占位证书（见文末复盘）。

opencode 日志位置：`~/.local/share/opencode/log/opencode.log`（找 `stream error` / `providerID=deepseek` 的行）。

## 排查套路（可复用）

按「先定位谁在报错 → 再看连接走哪条路 → 最后沿实际路径验证证书链」的顺序：

### 1. 看 opencode 日志：确认是哪个 provider/域名

看 `stream error` 行 → 本次是 `providerID=deepseek`，说明问题在模型 API 连接上。

### 2. 看连接实际走了哪条路

```bash
env | grep -i proxy   # 本次：http(s)_proxy=http://127.0.0.1:7892（本地 Clash 系代理）
```

代理软件（Clash/mihomo）本身只转发、不解密 TLS，但**它转发出去的路径上可能有别的中间人**（本机安全软件、公司防火墙）。2026-09-23 那次还看过 Clash 日志确认出口方式：

```
[TCP] 127.0.0.1:xxxxx --> api.deepseek.com:443 match GeoIP(cn) using DIRECT
```

### 3. 沿实际路径验证证书链（核心命令，决定性一步）

```bash
openssl s_client -connect api.deepseek.com:443 -servername api.deepseek.com </dev/null 2>/dev/null \
  | grep -E 's:|i:|Verify return code'
```

参数解释：

- `-connect 域名:443`：连目标服务器
- `-servername 域名`：TLS SNI，告诉服务器要访问的域名（一台服务器上可能挂很多网站）
- `</dev/null`：连上后立刻关闭输入，不进入交互模式
- `-proxy 127.0.0.1:7892`：通过 HTTP 代理建连。**要测"走代理路径"上的真实证书链必须加它**，否则测的只是直连
- `grep`：只看证书链（`s:` = 证书主体，`i:` = 签发者）和最终校验结果

本次测量（2026-10-05）：

```bash
# ① 直连 deepseek：正常
openssl s_client -connect api.deepseek.com:443 -servername api.deepseek.com </dev/null 2>/dev/null \
  | grep -E 's:|i:|Verify return'
# ② 走代理 deepseek：出事
openssl s_client -proxy 127.0.0.1:7892 -connect api.deepseek.com:443 -servername api.deepseek.com </dev/null 2>/dev/null \
  | grep -E 's:|i:|Verify return'
# ③ 对照组：走代理访问 GLM
openssl s_client -proxy 127.0.0.1:7892 -connect api.z.ai:443 -servername api.z.ai </dev/null 2>/dev/null \
  | grep -E 's:|i:|Verify return'
```

| 测量路径 | 域名 | 签发者（`i:` 行） | Verify return |
|---|---|---|---|
| 直连 | api.deepseek.com | TrustAsia → DigiCert（正规链） | `0 (ok)` |
| 走代理 | api.deepseek.com | **AO Kaspersky Lab（自签根证书，自己签自己）** | **`19 (self-signed certificate in certificate chain)`** |
| 走代理 | api.z.ai | Sectigo → USERTrust（正规链） | `0 (ok)` |

读表口诀：**直连正常 + 走代理冒出第三方自签证书 = 有中间盒在解密重签；同一代理下别的域名没事 = 按域名选择性拦截。**

健康链路应类似 `Verify return code: 0 (ok)`，链条是 `网站证书 → 中间证书 → 公共根证书`。若 `Verify return code` 非 0，或链里出现"自己签自己"的证书 → 证书被动了手脚，接着看签发者是谁。

### 4. 排除本地干扰项

| 检查 | 命令/位置 | 本次结果 |
|---|---|---|
| hosts 劫持 | `cat /etc/hosts` | 无异常 |
| 自定义 CA | `echo $NODE_EXTRA_CA_CERTS` | 未设置 |
| 代理变量 | `env \| grep -i proxy` | 走 `127.0.0.1:7892` |
| **本机安全软件的 HTTPS 扫描** | 托盘/安全软件设置里找「加密连接扫描」 | **卡巴斯基 = 元凶** |

### 5. 换客户端交叉验证

同一个域名，用不同工具测（排除"只有 opencode 有问题"）：

```bash
# curl：-I 只看响应头，不下载正文
curl -sI https://api.deepseek.com -o /dev/null -w 'http=%{http_code} ssl_verify=%{ssl_verify_result}\n'
# ssl_verify_result=0 表示证书校验通过（http 401 是因为没带 API key，正常）
```

```bash
# Node（opencode 的 Bun 同为 JS 运行时，参考价值高）
node -e "fetch('https://api.deepseek.com/',{method:'HEAD'}).then(r=>console.log('OK',r.status)).catch(e=>console.log('ERR:',e.message))"
```

## 根因（2026-10-05 确认）

- 本机卡巴斯基开启**「加密连接扫描」**（HTTPS 解密审计）：对被扫描的域名，它先与真实服务器建立 TLS、拿到真数据，再用**本地自签根证书**（`Kaspersky Anti-Virus Personal Root Certificate`）**现签一张同域名证书**发给客户端——你收到的"api.deepseek.com 证书"其实是它伪造的副本
- 拦截**按域名/分类选择性进行**：`api.deepseek.com` 命中扫描名单，`api.z.ai` 未命中 → 这就是"切 DeepSeek 必现、切 GLM 正常"的原因
- opencode 跑在 Bun（Node 系）运行时上，信任库只有公共 CA、没有卡巴斯基根证书 → 链里出现"自己签自己"的根 → `Verify return code: 19 (self-signed certificate in certificate chain)`

> [!warning] 关键判断
> 报"证书错误"说明 **TCP 已经连上了**——问题出在"验身份证"环节；如果连都连不上，报的会是超时/连接被重置。本次卡巴斯基正是插在"已连上"的那条路中间。

## 正确解法（2026-10-05 亲测）

1. **首选：给卡巴斯基加排除项**——设置 → 网络设置 → 加密连接扫描 → 排除项（"不扫描加密连接的网站"）→ 添加 `api.deepseek.com`。**加完立即生效，本次亲测恢复**。以后其他 API 域名被拦，照此办理
   - 好处：只放开一个域名，其余 HTTPS 流量仍受扫描保护，信任面最小
2. 备选：整体关闭「加密连接扫描」。所有 HTTPS 回到不被解密的状态，安全防护整体下降——除非排除项方式不可行，否则不建议
3. 备选（企业固定 CA 场景才用）：从卡巴斯基导出根证书 → 装入系统信任库（`/usr/local/share/ca-certificates/` + `sudo update-ca-certificates`），或 `export NODE_EXTRA_CA_CERTS=/path/to/kaspersky.pem` 只给 Node/Bun 系程序用。注意：这等于**信任卡巴斯基解密你全部 HTTPS**。2026-09-23 旧文说"导入没有意义"，错在当时的"占位证书"不固定；本次是固定 CA，导入有效——但依然不如排除项干净
4. 依旧**禁止**把 `NODE_TLS_REJECT_UNAUTHORIZED=0` 写进 `.bashrc` 或任何配置（原因见下节）

## ⚠️ 为什么 `NODE_TLS_REJECT_UNAUTHORIZED=0` 很危险

它让 Node/Bun 系程序（opencode 属于此类）**不再检查服务器证书**，等于：

> HTTPS 本来是"核验对方身份证后再进屋"，这个变量相当于把门卫撤了——**谁都能进屋**。

| 风险 | 说明 |
|---|---|
| 中间人攻击（MITM） | 任何人都能伪装成 `api.deepseek.com`，读写你的请求 |
| 凭据泄露 | 请求头里的 API key、对话内容全部明文可见 |
| 与"修好"无关 | 它没有修复任何问题，只是把"报警器"关掉了 |
| 隐蔽复发 | 将来真遇到攻击时也不会再报错，你不会察觉 |

范围提醒：该变量只影响 Node/Bun 程序，**不影响浏览器和 curl**——所以"浏览器正常"不代表安全。它只该用于临时调试，用完立刻移除，更不该 `export` 到 shell 配置里。

### 中间人攻击（MITM）是什么

**画面**：你给朋友寄信，途中有人拆开信件，抄一份、改几个字再封好转发——你和朋友都以为信件直达，其实中间一直有只手。

技术版：攻击者插在你和服务器之间，和两边分别建立"看似正常"的连接：

```
你 ←→ 攻击者（假装是服务器） ←→ 真服务器
```

- 对服务器，攻击者假装是你（能读到你的 API key）
- 对你，攻击者假装是服务器（**需要一张伪造证书**）
- 证书校验就是"识破伪造证书"的唯一防线；关掉校验 = 中门大开

真实世界里的三类情况：

1. **本机安全软件的「加密连接扫描」**（卡巴斯基、360、火绒等）——本文实证：不算恶意，但同样报"自签证书"
2. **公司/学校防火墙**做 TLS 解密审计（如 ZScaler）——同上，良性中间人
3. **公共 Wi-Fi 的恶意热点 / 代理/加速软件**做流量改造——典型攻击场景

判断口诀：**报自签错 → 先看签发者（`i:` 行）是谁。本机安全软件/公司 CA = "可信的中间人"，加排除项或导入其根证书；陌生网络里冒出的无名签发者 = 换网络，绝不关校验；都排除后仍偶发 → 才怀疑上游瞬时问题，重试即可。**

## 2026-09-23 那次"95 秒自愈"的误判复盘

当时的证据与结论：

- 报错约 95 秒后自行恢复，"瞬时、自愈"的表象与「上游占位证书」叙事高度吻合
- Clash 日志显示 `DIRECT`，排除代理节点动手脚
- 直连 `openssl s_client` 测到的证书链正常

于是写下「华为云 WAF 占位证书」的结论——**方向错了**。

错在哪：

1. **只测了直连一条路径**：opencode 实际流量带着 `https_proxy` 走 `127.0.0.1:7892`，这条路径上的证书链当时根本没测——而卡巴斯基的拦截恰恰作用在这条路上
2. **没看签发者就归因**：签发者（`i:` 行）若当时多看一眼，`Kaspersky Lab` 会直接指向安全软件
3. 被自愈表象带偏：事后看，"自愈"更可能是卡巴斯基扫描规则/状态的变化，与 WAF 无关

2026-10-05 的翻案过程：稳定复现（不再自愈）→ 直连/走代理双路径对比 → 走代理路径的签发者暴露 Kaspersky → 加排除项后立即恢复，因果闭环。

> [!tip] 教训
> 证书类报错必须把「实际走过的完整路径」都测一遍：直连、代理、VPN、安全软件，一环不漏。**单一路径正常，不能给整条链路无罪。**

## 术语小词典

| 术语 | 白话解释 |
|---|---|
| TLS / HTTPS | 在 HTTP 外面套一层加密 + 身份核验的协议 |
| 证书 | 服务器的"身份证"，由权威机构签发 |
| 证书链 | 身份证的"担保链"：网站证书 ← 中间证书 ← 根证书 |
| 根证书 | 系统/浏览器预装的"最终担保人"（如 DigiCert、Let's Encrypt） |
| 自签证书 | 自己给自己签的"身份证"，没有公认机构担保；出现在链里且不在信任列表 → 校验失败 |
| CA | Certificate Authority，签发证书的权威机构 |
| 加密连接扫描 | 卡巴斯基等安全软件的功能：解密检查 HTTPS 流量再加密回去，代价是用自签根证书重签被扫网站的证书 |
| 排除项 | 告诉安全软件"这些域名别扫"的名单；被排除的域名恢复真实证书链 |
| NODE_EXTRA_CA_CERTS | Node/Bun 的环境变量：指向额外的受信根证书文件，让程序信任企业/安全软件的私有 CA |
| MITM | Man-in-the-Middle，中间人攻击 |
| WAF | Web Application Firewall，网站前面的防护层（本文 9-23 误判的"元凶"，复盘见上文） |
| DIRECT | Clash 术语：不经过代理节点、直接连接 |

## 自测

> [!question]- 1. opencode 报"self signed certificate"，第一步该做什么？
> 三步定位：① 看 opencode 日志确认是哪个域名/provider；② 看代理环境变量/日志确认连接走哪条路；③ 沿**实际路径**用 `openssl s_client` 看证书链——重点看签发者（`i:` 行）和 `Verify return code`，必要时直连、走代理各测一次对比。

> [!question]- 2. 为什么这次只有 DeepSeek 报错，GLM 正常？
> 卡巴斯基的加密连接扫描**按域名/分类选择性拦截**：`api.deepseek.com` 命中扫描名单被重签证书，`api.z.ai` 未命中、链路原样透传。"只在一个域名报错"本身就是"有针对性拦截"的强线索。

> [!question]- 3. 为什么"关了校验就正常"不等于"问题解决了"？
> 校验的作用是识破伪造证书。关掉校验只是不再检查，伪造证书依然在那里。它拆了安全报警器，而不是修好了故障。

> [!question]- 4. 看到自签报错，怎么判断是"可信的中间人"还是真攻击？
> 看签发者：`Kaspersky`、公司名等认识的安全软件/企业 CA → 可信环境，加排除项或导入其根证书；陌生网络里冒出的无名签发者 → 提高警惕，断网/换网络，绝不用禁用校验硬闯。

## 相关

- [[somezhishi/somezhishi|somezhishi 库索引]]
- [[somezhishi/工具速查/opencode-TUI-速查表|opencode TUI 速查表]]
- [[somezhishi/环境排障/GitHub-不挂代理也能推送-原因与排查|GitHub 不挂代理也能推送]]（同为"代理/网络路径"排障）
