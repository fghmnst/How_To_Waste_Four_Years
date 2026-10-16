---
date: 2026-09-15
tags:
  - 速查
  - git
---

# GitHub 贡献计算：规则与排查

> [!note] 缘起
> 排查「有些提交不计入 contribution」时整理的官方规则；另含「隐藏邮箱」的两处配置（GitHub 开关 + 本地 git config）。

## 一、计入规则

**算贡献的行为**：提交 commit、开 Issue、提 PR、PR review、开/答 Discussion、新建仓库、fork 仓库。

commit 计入需**同时满足**：

| 条件 | 说明 |
|---|---|
| 邮箱已关联账号 | commit 的作者邮箱在 GitHub 账号的已关联邮箱列表中（含 noreply） |
| 非 fork 仓库 | fork 里的提交不计，需合并进上游 |
| 在默认分支或 `gh-pages` | 特性分支要合并后才计 |
| 与仓库有关系 | 协作者 / 组织成员 / fork 过 / 提过 PR 或 Issue（自己的仓库天然满足） |

其他口径：

- 时间用 commit 时间戳里的作者日期 + 时区；Issue/PR 用浏览器时区。
- 图谱只显示**滚动最近一年**。
- push 后**最多 24 小时**才刷新。
- 私有仓库贡献只显示数字（需开启 publicize）。
- rebase 时原作者与 rebase 者都得分；账号合并后 Issue/PR 不追溯归属。

## 二、push 与邮箱的分工

| 环节 | 发生什么 | 邮箱的角色 |
|---|---|---|
| `git commit` | 作者名/邮箱作为**文本**写入 commit 对象 | git 不做任何校验 |
| `git push` | 把 commit 传到远程 | **完全不参与**，邮箱是什么都能推 |
| GitHub 后台 | 读作者邮箱，与账号已关联邮箱比对 | 决定这条提交算在谁头上 |

结论：不 push 一定不计；push 了但邮箱未关联，也不计入你的贡献图——这是「有时候没有 contribution」的最常见原因。

## 三、邮箱必须真实存在吗

- **Git 层面：不需要**。任意字符串都能提交、能推送，不影响代码。
- **GitHub 贡献层面：要能被归属**，两条路径：
  1. 邮箱已加进 GitHub 账号（常规流程需验证邮件，基本要求可收信）；
  2. 使用 GitHub 发放的 **noreply 地址**，格式：`ID+用户名@users.noreply.github.com`（旧格式 `用户名@users.noreply.github.com`）。它不是真实邮箱、收不到信，但 GitHub 认它、贡献照常计入。
- GitHub 明确说明：通用/不可注册的邮箱（如 `jane@computer.local`）无法加进账号，这类邮箱的提交不计入。
- 真正需要「真实存在」的场景是账号主邮箱（收通知、找回密码），与 commit 里的邮箱无关。

## 四、邮箱隐私配置（隐藏真实邮箱）

### GitHub 网页（Settings → Emails）

| 开关 | 作用 |
|---|---|
| Keep my email addresses private | 移除资料页公开邮箱；网页端 Git 操作（编辑/合并）与代发邮件改用 noreply 地址 |
| Block command line pushes that expose my email | **命令行推送的提交若含你的真实邮箱则直接被拒**——防泄漏保险，推荐开启 |

注意：这两个开关**管不到本地命令行**。要让本地提交也用 noreply，必须自己设置 git config（见下）。

### 本地 git config

```bash
git config --global user.email "ID+用户名@users.noreply.github.com"
git config --global --get user.email      # 验证
```

- 优先级：仓库级 > 全局 > 系统级；只设全局时对所有仓库生效。
- 查看当前来源与是否有仓库级覆盖：

```bash
git config --list --show-origin | grep -i user
git config --local --get user.email        # 有输出=本仓库有覆盖
```

### 验证闭环

```bash
git log -1 --format='%ae'                  # 新提交的作者邮箱
git push origin main                       # 若开了拦截开关，错邮箱会被 GitHub 拒收
```

### 已知限制

历史提交的邮箱已写死，**无法自动隐藏**；只能改写历史 + force push（公开仓库风险大、commit hash 全变、第三方缓存未必清除），不建议。

## 五、排查清单（按可能性排序）

| 原因 | 自查方法 |
|---|---|
| 邮箱未关联账号 | 提交页 URL 末尾加 `.patch`，看 `From:` 行的邮箱；对照 Settings → Emails |
| 提交没 push | `git log --branches --not --remotes --oneline` |
| 24h 延迟未到 | 等待并刷新 |
| 不在默认分支 | `git branch` 确认；特性分支合并进 `main` |
| 时区导致日期漂移 | 用提交时间戳里的时区判断，而非本地直觉 |
| 超过一年 | 图谱只显示滚动 12 个月 |

## 关联

- [[somezhishi/somezhishi|somezhishi]]
- [[somezhishi/环境排障/Hermes-云部署指南-飞书每日推送|Hermes 云部署指南：飞书每日推送]]（日志不 commit/push，云端就读不到——同一原则）
