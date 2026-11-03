---
date: 2026-09-23
updated: 2026-10-29
tags:
  - 排障
  - opencode
---

# opencode TUI 中文变透明：宽字符重绘问题

> 2026-09-23。opencode TUI 里约四成汉字渲染成"透明/空白"格子，ASCII 和其余汉字正常；`Ctrl+P` 再 `Esc` 强制整体重绘后暂时恢复。**结论：opencode 上游已知 bug（宽字符宽度计算错误），不是本地编码/字体问题。**（2026-10-29 复核：仍未修复，且多了"浮层压中文整段消失"的形态，见下节）

## 2026-10-29 复核：浮层压住中文时整段消失（opencode v2.2.1）

同一个 bug 的**另一种、更夸张的表现**，根因相同：

- 在中文长回复上打开**浮层**（命令面板、`/sessions`、`/btw` 侧问浮层……）后，背景层的汉字**整段消失**，只剩 ASCII 碎片
- 现场长这样：背景只剩 `1.` `2.` `- A` `- B` `02-` `Lan` 这些窄字符，中文全没了 —— 看着像"画面错位/残影"，其实是被吞掉了
- `Esc` 关掉浮层 → 中文立刻回来；再打开 → 又消失（**100% 复现，30 秒可自证**）
- 环境：`opencode v2.2.1`（2026-10-29 已是最新）、`TERM_PROGRAM=vscode`、`TERM=xterm-256color`、`LANG=C.UTF-8`、WSL 远程
- VS Code `terminal.integrated.gpuAcceleration` 早已是 `"off"`，照样复现 → **终端渲染器已被排除**，别再折腾 GPU / 字体 / 编码

**机制（来自上游 issue 的定位）**：浮层下面压着一层半透明遮罩（`Dialog` 组件用 `RGBA.fromInts(0, 0, 0, 150)`）。遮罩重绘时，2 列宽的汉字单元格被清掉却没有补画，1 列的 ASCII 则被正常重画 → 屏幕上就变成"中文没了、ASCII 还在"。

> [!warning] 这是修复后的回归，值得上报
> 上游在 2026-10-09 关闭了同类 issue [#41274](https://github.com/anomalyco/opencode/issues/41274)（"CJK 和宽 emoji 在半透明弹层背景后消失"），但 v2.2.1 仍能复现。要么修复没覆盖 `/btw` 这类侧问浮层，要么是回归——这条信息本身对上游有用。

## 现象特征

- 一部分**汉字**变成空白格子（看着像"透明"），同屏其他汉字和 ASCII 正常
- 正文（AI 回复）和对话框里都会出现
- 按 `Ctrl+P` 打开命令面板再 `Esc`，或拖动/调整终端窗口大小后，**暂时恢复**，过一会儿又出现
- 选中复制出来的文字是完整的 → **数据没坏，纯显示问题**

## 一句话结论

opencode 的 TUI 渲染引擎在"局部重绘"时把汉字的**显示宽度（2 列）**算错，导致部分宽字符的格子被刷成空白。触发条件与增量刷新有关，强制全量重绘即可暂时恢复。

## 已知上游 issue（状态复核至 2026-10-29）

| Issue | 症状 | 状态 |
|---|---|---|
| [#41274](https://github.com/anomalyco/opencode/issues/41274) | **最贴切**：CJK 与宽 emoji 在半透明弹层背景后消失 | 已关闭 2026-10-09，但 v2.2.1 仍复现 |
| [#29007](https://github.com/anomalyco/opencode/issues/29007) | 同上，已定位到遮罩用半透明黑 `RGBA(0,0,0,150)` 的渲染缺陷 | 已关闭 2026-07-23 |
| [#34021](https://github.com/anomalyco/opencode/issues/34021) | 局部重绘后 CJK 错位/乱码，`Ctrl+P` 整体重绘可临时修复（Win + Linux 都复现） | 已关闭 2026-10-01 |
| [#41678](https://github.com/anomalyco/opencode/issues/41678) | VS Code 集成终端 + 中文输入，单个 CJK 字符空白 | 已关闭 2026-10-12 |
| [#50547](https://github.com/anomalyco/opencode/issues/50547) | 宽字符在行中间被吞掉/截断（Windows conhost） | **open** |
| [opentui#837](https://github.com/anomalyco/opentui/issues/837) | 上游渲染层：弹层下方的 CJK 被隐藏/覆盖 | **open** |

> [!note] 「已关闭」不等于「已修好」
> issue 可能被 stale 机器人扫掉。**判据只看实际表现**：v2.2.1 + WSL + VS Code 终端，2026-10-29 仍可复现。

## 可以排除的原因

| 猜测 | 为什么不是 |
|---|---|
| 终端字体缺 CJK 字形 | 缺字形会显示"豆腐块"（□）或问号，不是"透明"；本项目用的是 JetBrainsMono Nerd Font Mono（无 CJK，靠系统回退字体），显示其余汉字完全正常 |
| 编码/locale 问题 | `LANG=C.UTF-8` 正常；编码坏了会整段乱码成 `锟斤拷`，不是"部分空白" |
| VS Code 终端独有 | 官方 issue 在 Windows Terminal / WezTerm / GNOME Terminal 都复现，说明在 opencode 自身渲染层 |

## 临时缓解（即刻可用）

1. **关掉浮层**（`Esc`）——最快，中文立刻回来；浮层别长期开着压中文
2. **强制整体重绘**：`Ctrl+P` → `Esc`；或拖动窗口边缘改变终端尺寸
3. 高频出现时可以养成习惯：滚动看旧消息前先刷一次
4. 备选：换到 Windows Terminal 跑 `opencode`（不影响根治，但可对比验证）

## 本地已做的实验（2026-10-29 已有结论）

- 2026-09-23：VS Code 用户设置加 `"terminal.integrated.gpuAcceleration": "off"`（禁用终端 GPU 渲染，排除 WebGL 字形图集丢字）
  - 改完需**新开终端/重载窗口**才生效；备份在 `settings.json.bak-20260923`（Windows 侧 VS Code 用户设置目录）
- **结论（2026-10-29 复核）：无效。** 设置一直是 `"off"`，v2.2.1 照样复现 → 根因确认在 opencode 渲染层，不在终端。这个实验只起到"排除变量"的作用；留着无害，想回退就改回 `"auto"`

## 上报模板（可直接贴到 #41274 / #50547 / opentui#837）

```text
Confirming on VS Code integrated terminal (WSL remote), still present in 2.2.1:

- opencode: 2.2.1 (latest as of 2026-10-29); previously reproduced on 1.18.32
- Terminal: VS Code integrated terminal, TERM_PROGRAM=vscode, TERM=xterm-256color, LANG=C.UTF-8
- terminal.integrated.gpuAcceleration = "off", so the xterm.js renderer is not the variable
- Font: JetBrainsMono Nerd Font Mono (no CJK coverage, falls back to system CJK font)

Two distinct symptoms, same root:

1) ~40% of CJK cells render blank/transparent in the message body (partial repaint).
2) With a dialog/overlay open (e.g. the /btw side-question overlay), ALL CJK text in
   the background layer disappears while ASCII fragments remain. Closing the overlay
   (Esc) restores them instantly; reopening hides them again. 100% reproducible.

This looks like the same class as #41274 (closed 2026-10-09) - either the fix does not
cover the /btw aside overlay, or this is a regression in 2.x.

Workaround: Esc to close the overlay, or Ctrl+P twice / resize the terminal to force
a full repaint.
```

## 术语小词典

| 术语 | 白话解释 |
|---|---|
| TUI | Terminal UI，跑在终端里的图形界面（opencode 的界面） |
| 宽字符 / CJK | 汉字、日文、韩文在终端里占 **2 个字符格**，ASCII 占 1 格 |
| 局部重绘 / 全量重绘 | 终端只重新画"变了的那几行" vs 整屏重画；宽字符宽度算错就出在局部重绘 |
| 豆腐块（tofu） | 字体里没有该字形的占位方块，和"透明空白"是两回事 |
| 字形图集（glyph atlas） | GPU 渲染时把字形预烘焙成纹理贴图；贴图管理出错会造成个别字丢失 |
| 浮层 / overlay | 画在正文上层的弹窗（命令面板、侧问框），底下常垫一层半透明遮罩 |
| 遮罩（backdrop） | 浮层与正文之间那层"变暗的玻璃"，本 bug 就出在它和宽字符的合成上 |

## 自测

> [!question]- 1. 怎么区分"字体缺字"和"opencode 渲染 bug"？
> 缺字显示为 □/� 且**永远**缺；渲染 bug 表现为"透明空白"，且 `Ctrl+P`→`Esc` 强制重绘后会**暂时恢复**——因为文字数据本身是好的。

> [!question]- 2. 为什么复制粘贴出来是完整的，屏幕上却是空白？
> 终端里有两层：底层的字符缓冲区（复制时读的是它）和屏幕上的像素。bug 出在"把缓冲区画到屏幕"的渲染环节，所以数据完好、显示残缺。

> [!question]- 3. 为什么浮层一关，中文就全回来了？
> 因为文字数据一直都在，缺的只是"重画"这一步。浮层关闭时整块区域被强制重绘，汉字就跟着补回来了——这也反证不是字体或编码问题。

## 相关

- [[somezhishi/somezhishi|somezhishi 库索引]]
- [[somezhishi/工具速查/opencode-TUI-速查表|opencode TUI 速查表]]
- [[somezhishi/环境排障/opencode-自签证书报错-排查与禁用校验风险|opencode 自签证书报错]]（同日另一问题）
