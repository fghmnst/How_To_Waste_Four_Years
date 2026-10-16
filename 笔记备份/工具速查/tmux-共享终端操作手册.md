---
date: 2026-08-07
tags:
  - 速查
  - 工具
  - tmux
---

# tmux 共享终端操作手册

**日期**：2026-08-07
**场景**：opencode agent 与用户在云服务器（server2）上共用一个常驻终端（tmux 会话 `work`）的日常操作手册。

## 零、使用 tmux 的目的 · 能解决什么问题

**背景问题**：opencode 的 bash 工具是后台非交互执行——你只能看对话里的命令记录，**无法实时观看服务器终端，也无法中途亲手操作**；而且每次连接都是独立的新 shell，工作目录、环境变量全部从零开始。

**tmux 解决了什么**：

| 痛点 | tmux 的解法 |
|------|-------------|
| 看不到 agent 在服务器上实时干什么 | 你 `attach` 同一会话，屏幕实时同步 |
| 想亲手改配置 / 跑一条命令，agent 却占着终端 | attach 后直接打字（配合锁机制防冲突） |
| 长任务被断连打断、进度丢失 | 会话常驻服务器，脱离（detach）不消失 |
| 每条命令的环境从零开始（目录/环境丢失） | pane 里工作目录和环境持续保留 |
| 不知道 agent 执行到哪一步 | `capture-pane` 抓屏 / attach 看实况 |

**一句话总结**：把服务器终端从「agent 的盲操作间」变成「你和 agent 共用的透明工作台」——你看得见它在干什么，也能随时插手上场。

## 一、30 秒理解（概念）

| 术语 | 一句话 |
|------|--------|
| tmux | 终端复用器：一个程序开多个虚拟终端，断开后不消失 |
| session（会话） | 最外层容器；我们的会话叫 `work`，常驻服务器 |
| pane（窗格） | 会话里的一块屏幕，跑着一个 bash；我们只用单窗格 |
| attach（附着） | 把会话"贴"到你的终端上实时观看 |
| detach（脱离） | 撕下来回到普通终端，**会话还在后台继续跑** |
| send-keys | agent 向 pane 注入按键（打字） |
| capture-pane | agent 读取 pane 屏幕内容（抓屏成文字） |

## 二、进入 tmux 的三种方式

**方式 A · 本地一条龙**（推荐日常用）：
```bash
ssh -t server2 tmux attach -t work
```
`-t` 必须保留（分配伪终端，tmux 界面才能工作）。

**方式 B · 已经 ssh 登录了服务器**（你现在的情况）：
```bash
tmux attach -t work
```
进入后如果显示 `Pane is dead`，见第三节恢复。

**方式 C · 会话丢失时重建**（仅服务器重启后）：
```bash
tmux new-session -d -s work -c /home/fghmnst/projects
tmux set-option -t work history-limit 10000
tmux set-option -t work remain-on-exit on
tmux attach -t work
```

## 三、Ctrl+D 意外处理方案

**发生了什么**：`Ctrl+D` 是 EOF（文件结束符），等于对 shell 说"关闭"→ bash 退出 → 显示 `logout` → 面板变 `Pane is dead`。这不是故障；`remain-on-exit on` 只是保留了死前画面方便排查。

**恢复方案 1 · attach 内操作**（如果还在会话里）：
按 `Ctrl-b`，再按 `:`（进入 tmux 命令模式），输入 `respawn-pane`，回车。

**恢复方案 2 · 外部一条命令**（没在 attach）：
```bash
ssh server2 'tmux respawn-pane -t work'
```
`respawn-pane` = 在同一个 pane 里重新启动一个 shell，历史缓冲保留。

**以后怎么避免**：
- `Ctrl-b d`（detach）＝ 下车：离开但会话活着，**日常离开用这个**
- `Ctrl-d`（EOF）＝ 熄火：关掉 shell，**只有想彻底关闭时才用**
- 这俩一个"下车"一个"熄火"，别混了

## 四、需要你手动执行的内容（日常全表）

| 操作 | 命令 | 何时 | 在哪执行 |
|------|------|------|----------|
| 附着观看/交互 | `ssh -t server2 tmux attach -t work` | 想看 agent 操作或亲手操作时 | 本地任意终端 |
| 脱离（不关会话） | `Ctrl-b d` | attach 状态下想离开时 | attach 窗口内 |
| 占锁（打字前） | `ssh server2 'touch ~/.tmux-hold'` | **每次想打字前**，agent 会停手等你 | 本地终端 |
| 释放锁（打完字） | `ssh server2 'rm ~/.tmux-hold'` | 打完字，agent 恢复工作 | 本地终端 |
| 只读查看屏幕 | `ssh server2 'tmux capture-pane -t work -p -S -20'` | 不想 attach 只想瞄一眼 | 本地终端 |
| 复活死掉的 pane | `ssh server2 'tmux respawn-pane -t work'` | 出现 `Pane is dead` 时 | 本地终端 |
| 重建会话 | 第三节方式 C 的 3 条命令 | **仅服务器重启后** | 服务器上 |
| 彻底销毁会话 | `ssh server2 'tmux kill-session -t work'` | 确定不再需要时 | 本地终端 |

## 五、并发锁机制

agent 和你同时在 pane 里打字会互相踩。约定：**打字前占锁，agent 检测到锁就停手**。

```bash
ssh server2 'touch ~/.tmux-hold'          # 占锁（打字前）
ssh server2 'test -f ~/.tmux-hold && echo HELD || echo FREE'   # agent 检查（HELD=停手）
ssh server2 'rm ~/.tmux-hold'             # 释放（打完字）
```

## 六、故障排查速查表

| 现象 | 原因 | 处理 |
|------|------|------|
| `Pane is dead` | 有人按了 Ctrl+D / shell 退出 | `respawn-pane`（见第三节） |
| `no server running` / `no sessions` | 服务器重启过，会话丢了 | 方式 C 重建 3 条命令 |
| agent 不动 / 一直等待 | 锁没释放 | `test -f ~/.tmux-hold` 检查；确定没人打字就 `rm` |
| attach 进去黑屏无提示符 | shell 死过或刚 respawn | 按回车；或再 respawn-pane 一次 |
| 看不到长命令的完整输出 | capture-pane 只抓屏幕范围 | 用 `-S -100` 加大抓取行数；或命令尾部加 `echo __DONE__` 等哨兵 |

## 七、关联

- [[somezhishi/环境排障/Hermes-云部署指南-飞书每日推送|Hermes 云部署指南]]
- [[归档/30天学习/30天学习 Index]]
- [[归档/30天学习/术语表/术语表|术语表]]
