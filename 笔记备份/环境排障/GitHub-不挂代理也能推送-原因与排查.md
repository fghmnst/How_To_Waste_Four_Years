---
date: 2026-09-25
tags: [入门, git, ssh]
---

# GitHub 不挂代理也能推送：原因与排查

> [!note] 一句话结论
> GitHub 在大陆**不是被完全封锁，而是"间歇性干扰"**——时通时不通。这次不挂代理 `git push` 成功，说明恰好赶上了"通"的窗口，而且是走的 SSH 通道（22 端口），和浏览器访问 GitHub 的 HTTPS 通道（443 端口）受干扰的情况不一样。**能用就用，不用特意挂代理；哪天卡住再按文末方案处理。**

## 先建立一个画面：墙不是铁板，是雾霾

很多人以为"被墙"= 一扇关死的铁门，谁都过不去。实际更像**雾霾天看远处**：

- 有的日子能见度不错，肉眼（直连）就能看到对面的楼；
- 有的日子突然糊了，就得戴上"防霾口罩"（代理）。

所以"昨天不通、今天通了"或"手机不通、电脑通了"都很正常——不是错觉，也不是玄学，只是干扰强度在时间和空间上都不均匀。

## 为什么这次能直连成功

**原因一：GitHub 是"间歇性干扰"，不是彻底封锁。**
干扰主要发生在特定时段和特定网络环境下，表现多为连接被重置（`Connection reset`）或超时。赶上一个好窗口，一次 push（数据量小、几秒钟完成）就够了。

**原因二：你走的是 SSH，不是浏览器走的 HTTPS。**

| 通道 | 端口 | 谁在用 | 受干扰情况 |
|---|---|---|---|
| HTTPS | 443 | 浏览器开 GitHub 网页、`https://` 开头的仓库 | 干扰更常见（网页要长时间稳定连接） |
| SSH | 22 | `git@github.com:` 开头的仓库（你的就是这种） | 多数时候、多数地区能直连 |

`git remote -v` 看到 `git@github.com:fghmnst/projects.git` 就说明走的是 SSH。两种协议像两扇不同的门，安检强度不同——HTTPS 那扇更容易被拦，SSH 22 这扇通常宽松。

**原因三：校园网/教育网对 GitHub 相对友好。**
不少高校的出口网络访问国际站点比家宽稳定，直连 GitHub 在教育网环境是相当常见的现象。

**另外一个关键点：push 和刷网页的要求不同。**
刷 GitHub 网页需要**持续稳定**的连接（图片、脚本一个个加载），稍有抖动页面就残了；而 `git push` 只需要**一条 TCP 连接建立成功**、传完数据就收工。所以"网页打不开但 push 能成功"完全可能同时发生。

## 排查：这次真的没走代理吗

想确认自己是"真直连"而不是"隐形代理"，按下面三步查（每步都只读、无副作用）：

**① 看 SSH 有没有配代理**：

```bash
grep -iA5 github ~/.ssh/config
```

- 命令意思：在 SSH 配置文件里找 github 相关条目（`-i` 忽略大小写，`-A5` 顺带显示后 5 行）。
- 有输出时看有没有 `ProxyCommand` / `ProxyJump` 字样——有就说明 SSH 被配置成强制走代理；没有任何输出说明没有针对 github 的特殊配置。

**② 看终端有没有代理环境变量**：

```bash
env | grep -i proxy
```

- 命令意思：列出所有环境变量，筛出名字带 proxy 的（如 `http_proxy`、`https_proxy`）。
- 没输出 = 当前终端没有设置代理。

**③ 看 Windows 侧有没有 TUN/全局模式代理**（WSL 流量可能被无感接管）：

WSL2 的网络流量默认经过 Windows 转发。如果 Windows 上的代理软件（Clash 系等）开了 **TUN 模式**，它会建一张虚拟网卡接管所有流量——此时你以为"没挂代理"，实际流量已经绕道了。看一眼 Windows 托盘/代理软件界面确认即可。

三条都干净，那就是纯直连成功，别怀疑。

## 哪天 push 卡住了：备用方案

按顺序试，通常第 1 步就能解决：

**方案一：先重试几次。**
间歇性干扰意味着"等一会再试"经常有效，别急着改配置。

**方案二：换 SSH 备用通道（443 端口）。**
GitHub 官方提供了 `ssh.github.com:443`——把 SSH 包装在 443 端口里传输，借 HTTPS 专用通道的"绿灯"。编辑 `~/.ssh/config` 加入：

```
Host github.com
  HostName ssh.github.com
  Port 443
  User git
```

- 意思：以后连 `github.com` 时，实际连到 `ssh.github.com` 的 443 端口，以 git 用户身份登录。
- 验证：`ssh -T git@github.com`，看到 `Hi 用户名!` 即通。

**方案三：走代理。**
给 git 单独配代理（不影响其他程序），或让代理软件 TUN 模式接管。配了 443 通道后一般用不到这步。

## 另一类推送失败：GH007 邮箱隐私保护（2026-09-26 补充）

> [!warning] 先分清两类失败
> 上面的排查和备用方案都针对**网络类**失败；还有一类失败**与网络无关**，报错里会出现 `GH007` / `email privacy` 字样——本文 2026-09-26 亲历（当时网络、密钥、分支全正常，`git ls-remote` 秒回，唯独 push 被拒）。

**现象**：网络排查全过，但 `git push` 被服务端拒收，典型报错：

```
remote: error: GH007: Your push would publish a private email.
! [remote rejected] main -> main (push declined due to email privacy restrictions)
```

**背景机制**：GitHub 的「Keep my email address private」（保持邮箱私密）设置自带一个勾选项 **"Block command line pushes that expose my email"**：一旦开启，任何**作者邮箱没有登记在你 GitHub 账号下**的 commit，都会在推送时被 GitHub 直接拒收——目的是保护你的真实邮箱不进入公开历史。

**这次踩的坑：多台电脑的 gitconfig 各自独立。** 2026-09-15 只把主力机的 `user.email` 换成了 noreply，另一台电脑（8 月装机时配置的）没有同步；9-26 在那台提交的 commit 带真实邮箱 → push 被拦。教训：**noreply 切换不是「改一次全局生效」，每一台电脑都要改。**

### 判据：网络类 vs 邮箱类

| 报错关键词 | 类别 | 处理 |
|---|---|---|
| `Connection reset` / `timed out` / `Could not resolve host` | 网络类 | 上方「备用方案」：重试 → 443 通道 → 代理 |
| `GH007` / `email privacy restrictions` / `would publish a private email` | 邮箱类 | 按下方三步修 |

### 修复三步（适用于 commit 尚未推送）

```bash
# ① 统一本机邮箱为 GitHub noreply（每台电脑都要做；noreply 地址在 GitHub Settings → Emails 里找）
git config --global user.email "数字+用户名@users.noreply.github.com"

# ② 修正还没推送的 commit 的作者邮箱（--reset-author 用当前配置重写作者）
git commit --amend --no-edit --reset-author

# ③ 验证后推送
git log -1 --format='%ae'   # 应显示 noreply 地址
git push
```

- 若 commit **已经推送过**：不能 amend（会改写公开历史），只能把旧邮箱加进 GitHub 账号来消除影响（见 [[somezhishi/工具速查/GitHub贡献计算-规则与排查|GitHub 贡献计算]]）；
- 判断本机正在用哪个文件里的邮箱：`git config --show-origin user.email`，输出里 `file:` 后面的路径就是来源（本仓库没有仓库级配置时，来源就是 `~/.gitconfig`）。

### 多机自查清单（换电脑 / 重装系统后各跑一次）

```bash
git config --show-origin user.email    # 来源文件是否为 ~/.gitconfig、值是否为 noreply
git log -5 --format='%h %ae'           # 回顾最近几条提交实际用的邮箱
```

## 术语小词典

| 术语 | 大白话解释 |
|---|---|
| 代理（proxy） | 中转站：你的流量先发给它，由它代为访问目标，再原样带回结果 |
| TUN 模式 | 代理软件建一张虚拟网卡，把系统**所有**流量都拦下来代理，不需要逐个软件设置 |
| SSH | 加密的远程连接协议，git 用它跟 GitHub 通信；默认走 22 端口 |
| HTTPS | 加密的网页协议，浏览器和 `https://` 链接用的就是它，默认走 443 端口 |
| 端口 | 一台服务器上的"门牌号"，不同服务在不同门口接待：22 是 SSH 的门，443 是 HTTPS 的门 |
| `Connection reset` | 连接刚建立就被对方掐断——干扰的典型表现之一 |
| `GH007` | GitHub 拒收推送的错误代码：要推的 commit 会暴露你的私密邮箱，被服务端拦下（详见上文「另一类推送失败」） |

> [!question]- 自测：不看正文，能答上来吗？
> 1. GitHub 在大陆是"完全封锁"还是"间歇性干扰"？这两种状态对"昨天失败今天成功"的解释有什么不同？
> 2. 为什么浏览器打不开 GitHub 网页，但 `git push` 却可能成功？（两个原因：协议不同 + 成功标准不同）
> 3. `git remote -v` 输出 `git@github.com:...` 说明走哪个通道、哪个端口？
> 4. 三步自查"隐形代理"分别查什么？
> 5. `ssh.github.com:443` 备用通道的原理是什么？（提示：把 SSH 塞进哪扇"门"里）

## 关联

[[somezhishi/编程基础/Git多远程-切换与双仓库同步|Git 多远程]] · [[somezhishi/编程基础/SSH密钥-修改口令与ssh-keygen参数详解|SSH 密钥]] · [[somezhishi/工具速查/GitHub贡献计算-规则与排查|GitHub 贡献计算]] · [[somezhishi/somezhishi|somezhishi]]
