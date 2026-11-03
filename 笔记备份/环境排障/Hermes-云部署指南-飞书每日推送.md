---
date: 2026-08-10
tags:
  - 指南
  - hermes
  - 工具
---

# Hermes 云部署指南（飞书每日推送）

**日期**：2026-08-06 初版（QQ）→ 2026-08-10 迁移至飞书 → 2026-09-13 升级（课表 + 待办 + 建议；推送规范迁入 `.hermes.md` §9）→ 2026-09-13 增补「今日训练」（§9 概要 + §10 完整列表双推）→ 2026-09-14 训练日改周三/五/日，daily-training 详细推送下线并删除 cron（只留 7:00 daily-digest）
**场景**：在云服务器上配置 Hermes Agent，让它每天 7:00 通过飞书机器人推送「今日课表 + 今日训练 + 昨日小结 + 今日待办 + 今日建议」，数据来自 GitHub 仓库的课程表、训练执行表、每日日志与周待办。原训练日 7:05 的完整动作列表推送（daily-training）已于 2026-09-14 下线，cron 任务已删除（如需重建见文末命令）。

## 平台选型：QQ/微信 → 飞书

**迁移理由（2026-08-10 决策，QQ/微信均已停用）**：

| 平台 | 问题 | 结论 |
|------|------|------|
| QQ | markdown 仅支持**受限子集**：`#`/`##` 两级标题、`**加粗**`、`_斜体_`、`~~删除线~~`、`-`/`1.` 列表、`>` 引用、`***` 分割线；**不支持 `###`+ 三级标题、表格、代码块**。复杂推送（多级标题+表格）直接渲染失败（08-09 实测：cron prompt 用了 `###`/表格，客户端原样显示语法字符） | 放弃 |
| 微信（iLink） | 机器人**24 小时内用户未主动发消息则无法主动推送**——每日 07:00 的定时推送必受影响；且个人微信自动化有腾讯风控风险 | 放弃 |
| 飞书 | markdown 支持完善（标题/加粗/列表/引用/行内代码/代码块）；无主动消息限制；websocket 长连接免公网 IP；官方开放平台稳定 | **采用** |

## 最终效果

```
每天 07:00（Asia/Shanghai）→ 服务器 Hermes cron 触发（daily-digest）
  → git pull 仓库 projects
  → 读课程表 + 训练执行表 + 昨日日志 + 当周未完成待办（规范见 .hermes.md §9）
  → 生成【今日课表】【今日训练】【昨日小结】【今日待办】【今日建议】
  → 推送到你的飞书（机器人私聊）
训练日 07:05 → daily-training cron（周三/五/日）【已下线：2026-09-14 删除，不再推送】
  → 读训练执行表 + 训练计划 → 推送【完整动作列表】（规范见 .hermes.md §10）
```

## 架构

```
本地 WSL ──ssh server2──> 云服务器（RainYun, Ubuntu）
                           ├─ Hermes Agent（~/.hermes，provider: minimax-cn）
                           ├─ Hermes Gateway（systemd 服务，24h 常驻）
                           │    ├─ 飞书适配器（WebSocket 长连接 ↔ 飞书开放平台）
                           │    └─ Cron 调度器（每 60s tick）
                           └─ ~/projects（GitHub 私有仓库 clone）
```

## 前置准备

| 项目 | 说明 |
|------|------|
| 云服务器 | Ubuntu（本例 RainYun 小 VPS，1.9G 内存够用），Hermes 已安装可对话 |
| 飞书开放平台应用 | 在 [open.feishu.cn](https://open.feishu.cn) 创建**企业自建应用**，添加「机器人」应用能力，**发布版本**（管理员审核通过后应用才生效），拿到 **App ID** 和 **App Secret** |
| GitHub 账号 | 用于授权服务器读取私有仓库 |
| 本地 ssh | WSL 与 Windows 共用同一对 ed25519 密钥（已在服务器 authorized_keys） |

> 飞书应用形态是「企业自建应用 + 机器人能力」，部署时以 `websocket` 长连接模式接入（Hermes 配置 `FEISHU_CONNECTION_MODE=websocket`），**无需公网 IP 或 webhook 回调地址**，RainYun 这类无公网端口的 VPS 也可用。

## 步骤

### 1. 本地 ssh 别名（WSL）

Windows 侧的 `C:\Users\<你>\.ssh\config` 可能已配好别名，但 WSL 不共享它，需在 `~/.ssh/config` 写一份：

```
Host *
    ServerAliveInterval 60
    ServerAliveCountMax 10

Host server2
    HostName  <服务器IP，见 ~/.ssh/config>
    User <你的用户名>
    Port 22
```

验证：`ssh server2` 能免密登录即 OK。

### 2. 服务器基础准备

```bash
ssh server2
sudo timedatectl set-timezone Asia/Shanghai   # 关键！cron 按服务器本地时区走
```

生成 GitHub 专用密钥（与登录用的密钥分开，最小授权）：

```bash
ssh-keygen -t ed25519 -N "" -C "server2-fghmnst" -f ~/.ssh/id_ed25519
cat ~/.ssh/id_ed25519.pub   # 复制公钥
```

去 GitHub → Settings → SSH and GPG keys → New SSH key 粘贴，然后：

```bash
ssh -T git@github.com   # 出现 "Hi <用户>! You've successfully authenticated" 即成功
git clone git@github.com:<用户>/projects.git ~/projects
```

### 3. Hermes 飞书配置

**凭据**：追加到 `~/.hermes/.env`（密钥类配置都放这里，非密钥的放 config.yaml）：

```
FEISHU_APP_ID=cli_xxxxxxxx
FEISHU_APP_SECRET=xxxxxxxx
FEISHU_DOMAIN=feishu
FEISHU_CONNECTION_MODE=websocket
FEISHU_ALLOWED_USERS=ou_xxxxxxxxxxxxxxxxxxxxxxxx
FEISHU_GROUP_POLICY=open
```

**启用平台**：`~/.hermes/config.yaml`：

```yaml
platforms:
  feishu:
    enabled: true
    home_channel:
      platform: feishu
      chat_id: oc_xxxxxxxx
      user_id: ou_xxxxxxxx
```

> `home_channel` 是 cron 投递的默认目标。可在飞书对话里用 `/sethome` 命令自动写入，无需手填。

### 4. 启动 Gateway（必须用 systemd）

Gateway 是 Hermes 的常驻后台进程：负责飞书长连接 + cron 调度。**不要用 nohup 裸跑**（ssh 断开就死，见踩坑 3）。

```bash
sudo ~/.local/bin/hermes gateway install --system   # 注意：sudo 下必须写全路径
sudo ~/.local/bin/hermes gateway start --system
sudo ~/.local/bin/hermes gateway status --system
```

验证连接：`tail -f ~/.hermes/logs/gateway.log` 应出现 `✓ feishu connected`。

### 5. 拿到你的 open_id 并收紧白名单

**open_id** 是飞书 API 给你的用户匿名 ID（机器人看不到你的手机号/姓名），同一用户对同一应用永远不变，cron 推送就靠它定向。

1. 在飞书给机器人发一条任意消息（如"测试连接"）
2. 服务器日志里抓 open_id：
   ```bash
   grep "inbound message" ~/.hermes/logs/gateway.log
   # inbound message: platform=feishu user=ou_xxxx...(真实值脱敏) chat=oc_... user_id=ou_...
   ```
3. 写入 `.env` 白名单（`FEISHU_ALLOWED_USERS`，多个用逗号分隔）+ 在飞书对话中执行 `/sethome` 设置 home_channel

### 6. 创建每日 7 点 cron 任务

```bash
hermes cron create "0 7 * * *" "$(cat /tmp/cron_prompt.txt)" \
  --deliver feishu --workdir /home/fghmnst/projects --name daily-digest
```

- `--deliver feishu`：agent 的最终回复自动推到飞书（home_channel 指定）
- `--workdir`：任务在该目录运行，自动加载仓库上下文文件（`.hermes.md`，优先级高于 AGENTS.md）
- 完整 prompt 见文末附录；推送规范唯一事实源为仓库 `.hermes.md` §9（git pull → 课表 + 日志 + 待办 → 推飞书）

验证：

```bash
hermes cron list          # 看 Next run 是否为 07:00 (+08:00)
hermes cron run daily-digest   # 手动触发一次端到端测试
grep "delivered to feishu" ~/.hermes/logs/agent.log   # 确认投递成功
```

## 踩坑记录（新手必看）

1. **`sudo hermes` 找不到命令**：sudo 用系统 secure_path，不含 `~/.local/bin`。解法：`sudo ~/.local/bin/hermes ...` 写全路径。
2. **cron 时间不对**：服务器默认 UTC（+0），7 点变 15 点。先 `timedatectl set-timezone Asia/Shanghai`。
3. **nohup 起 gateway 一断 ssh 就死**：后台进程挂在 ssh 会话的 cgroup 里，会话关闭即被清理。长驻进程必须走 systemd。
4. **无 sudo 重启 systemd 服务**：服务是 `Restart=on-failure`，`pkill -f "hermes_cli.main gateway"` 杀掉进程后 systemd 会自动拉起（~30s），新进程会读到改过的配置。
5. **ssh 非交互 shell 没有 hermes**：PATH 里没有 `~/.local/bin`，脚本里用全路径 `~/.local/bin/hermes`。
6. **本地 `ssh server2` 解析失败**：WSL 与 Windows 的 ssh 配置不互通，WSL 侧要自己写 `~/.ssh/config`（密钥可共用）。
7. **飞书机器人"不存在/无响应"**：应用没生效——open.feishu.cn 后台创建应用后**必须发布版本**（走管理员审核），且应用需开启「机器人」能力；另外确认 gateway.log 有 `feishu connected`。
8. **飞书 markdown 渲染规范**（cron prompt 与对话输出都适用）：支持 `#`/`##` 标题、`**加粗**`、`-` 列表、`> 引用`、`` `行内代码` ``、代码块；**禁表格、嵌套列表超 2 层、超长代码块**；推送总量控制在 500 字内（`###` 三级标题飞书也支持，但统一用 `##` 保持层级简洁）。
9. **历史坑（QQ 时代，留档）**：QQ dm_policy open 时必须设 `QQ_ALLOW_ALL_USERS=true` 否则拒绝启动（安全设计）；QQ markdown 子集不支持 `###`/表格（已随平台弃用，不再适用）。

## 日常运维

| 事项 | 说明 |
|------|------|
| 每日写日志（非强制，缺日直接忽略、不补写）并当天 `git commit`；周日晚写并 commit 周复盘 | cron 每次先 `git pull`，不提交就读不到最新内容；昨日无日志时晨报标注「昨日无日志」，不回溯 |
| 改推送规范 | 改仓库 `.hermes.md` §9/§10（或 `obsidian/课程表.md`、`obsidian/健身/训练计划.md` 等数据文件）→ commit+push，cron 下次运行 `git pull` 后自动生效 |
| 查推送是否成功 | `grep "delivered to feishu" ~/.hermes/logs/agent.log` |
| 健康检查 | `hermes doctor`；`hermes gateway status --system` |
| 换 provider | `hermes model`，与部署无关，随时可改 |

## 附录：cron 任务 prompt（2026-09-13 起：瘦指针版）

> 推送规范已迁入仓库 `.hermes.md`（§9 daily-digest、§10 daily-training），随 git 同步；改推送内容只需改仓库并 push，无需再动服务器。

daily-digest prompt（`0 7 * * *`）：

```
你是用户的学习助理。请严格按仓库 .hermes.md 第 9 节「daily-digest 每日推送规范」执行，并将最终报告作为最终回复输出。
```

daily-training prompt（`5 7 * * 0,3,5,6`，训练日 7:05 追加推送；**当前已下线，任务已删除**）：

```
你是用户的训练助理。请严格按仓库 .hermes.md 第 10 节「daily-training 每日训练详情推送规范」执行，并将最终报告作为最终回复输出。
```

首次创建/重建时在服务器执行（`hermes cron edit` 支持用任务名引用）：

```bash
cd ~/projects && git pull --quiet
~/.local/bin/hermes cron edit daily-digest --prompt "你是用户的学习助理。请严格按仓库 .hermes.md 第 9 节「daily-digest 每日推送规范」执行，并将最终报告作为最终回复输出。"
~/.local/bin/hermes cron create "5 7 * * 0,3,5,6" "你是用户的训练助理。请严格按仓库 .hermes.md 第 10 节「daily-training 每日训练详情推送规范」执行，并将最终报告作为最终回复输出。" --deliver feishu --workdir /home/fghmnst/projects --name daily-training
```

验证：`~/.local/bin/hermes cron list` 能看到 daily-digest（daily-training 已于 2026-09-14 下线）；`~/.local/bin/hermes cron run daily-digest` → 飞书收到推送；`grep "delivered to feishu" ~/.hermes/logs/agent.log`。

daily-training 已于 2026-09-14 执行 `~/.local/bin/hermes cron remove daily-training` 彻底下线（当前仅保留 daily-digest）；如需重建，用上方 create 命令。

## 术语

| 术语 | 一句话解释 |
|------|-----------|
| Gateway | Hermes 的常驻后台进程：平台连接器（飞书等）+ cron 调度器，必须 24h 运行 |
| open_id | 飞书 API 给你的用户匿名 ID（机器人看不到手机号/姓名），用于定向推送和授权 |
| chat_id | 飞书会话 ID（私聊 `oc_` 开头），cron 推送的目标频道 |
| Cron | 定时任务；Hermes 的 cron 由 gateway 每 60 秒 tick 检查触发 |
| systemd | Linux 的服务管理器，保证进程开机自启、崩溃自动拉起 |

相关：[[归档/30天学习/30天学习 Index]] · [[归档/30天学习/术语表/术语表|术语表]] · [[somezhishi/工具速查/opencode-TUI-速查表]]
