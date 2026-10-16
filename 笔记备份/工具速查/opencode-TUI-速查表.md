---
date: 2026-08-05
tags:
  - 速查
  - 工具
  - opencode
---

# opencode TUI 速查表

**日期**：2026-08-05
**场景**：opencode TUI 常用斜杠指令与快捷键。

## 斜杠指令

leader 键默认 `ctrl+x`，多数指令有快捷键。

| 指令 | 快捷键 | 作用 |
|------|--------|------|
| `/new`（`/clear`） | `ctrl+x n` | 新会话 |
| `/sessions`（`/resume`/`/continue`） | `ctrl+x l` | 切换历史会话 |
| `/exit`（`/q`） | `ctrl+x q` | 退出 |
| `/compact`（`/summarize`） | `ctrl+x c` | 压缩上下文（省 token） |
| `/undo` | `ctrl+x u` | 撤销消息+文件改动（需 git 仓库） |
| `/redo` | `ctrl+x r` | 重做刚撤销的内容 |
| `/connect` | — | 添加 provider / API key |
| `/models` | `ctrl+x m` | 查看/切换模型 |
| `/themes` | `ctrl+x t` | 切换主题 |
| `/init` | — | 生成/更新 AGENTS.md |
| `/editor` | `ctrl+x e` | 外部编辑器写长消息（`$EDITOR`） |
| `/export` | `ctrl+x x` | 导出对话为 Markdown |
| `/details` | — | 显示/隐藏工具执行细节 |
| `/thinking` | — | 显示/隐藏思考块 |
| `/share` `/unshare` | — | 分享 / 取消分享会话 |
| `/help` | — | 帮助 |

## 输入语法

- `@文件`：模糊搜索并附加文件内容到对话
- `!命令`：执行 shell 命令，输出注入对话

## 其他按键

- `Tab`：plan（只读）/ build（可改）模式切换
- `ctrl+t`：循环切换模型推理变体
- `alt+p`：命令面板（本机已改，原为 `ctrl+p`，可搜 "hide username" 等）

## 本机自定义键位

配置文件：`~/.config/opencode/tui.json`，`keybinds` 与默认值自动合并，只写被改动的键。

| 键位 | 默认 | 本机现值 | 原因 |
|------|------|----------|------|
| 命令面板 `command_list` | `ctrl+p` | `alt+p` | 避让 VS Code 快速打开文件 |
| 收藏模型 `model_favorite_toggle` | `ctrl+f` | `alt+f` | 避让 VS Code 查找 |
| 提交消息 `input_submit` | `enter` | `enter` | 改回 Enter 发送，适配 Windows 自带 bash 终端 |
| 换行 `input_newline` | `shift+enter,ctrl+enter,alt+enter,ctrl+j` | `shift+enter,ctrl+enter,alt+enter,ctrl+j` | 恢复默认 |


> [!note] 发送键说明
> 因windows自带的bash终端有多开的特性，后期会遇到需要频繁使用win端bash中opencode的情况，故将发送的快捷键改回enter，但其他改为alt系的快捷键不做更改。
> 备注：配置文件里按键名写作 `return`（如 `input_submit: "return"`），文档中统一用可读写法 `enter`，两者等价。

## Fork Session（分叉会话）

类似 git 分支：从已有会话复制出副本再继续，新会话共享历史但从此分道扬镳，原会话保持不变。适合同一段上下文上并行尝试多条路线（如方案 A vs B）。

- 不带 fork：`opencode -c` / `-s <id>` 在原会话上继续（原会话被改动）
- 带 fork：`opencode -c --fork` / `-s <id> --fork` 复制后继续，原会话不受影响
- TUI 内有 `session_fork` 动作但默认无快捷键（`none`），需配置绑定或走 CLI

## 备注

- `/undo`、`/redo` 内部靠 Git 回滚文件改动，项目必须是 git 仓库。
- `/editor`、`/export` 依赖 `EDITOR` 环境变量，GUI 编辑器需加 `--wait`（如 `export EDITOR="code --wait"`）。
- TUI 行为配置在 `tui.json`（独立于 `opencode.json`），可调主题、快捷键、滚动、通知。

相关：[[归档/30天学习/30天学习 Index]] · [[归档/30天学习/术语表/术语表|术语表]]
