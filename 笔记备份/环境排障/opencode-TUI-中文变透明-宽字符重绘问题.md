---
date: 2026-09-23
tags:
  - 排障
  - opencode
---

# opencode TUI 中文变透明：宽字符重绘问题

> 2026-09-23。opencode TUI 里约四成汉字渲染成"透明/空白"格子，ASCII 和其余汉字正常；`Ctrl+P` 再 `Esc` 强制整体重绘后暂时恢复。**结论：opencode 上游已知 bug（宽字符宽度计算错误），不是本地编码/字体问题。**

## 现象特征

- 一部分**汉字**变成空白格子（看着像"透明"），同屏其他汉字和 ASCII 正常
- 正文（AI 回复）和对话框里都会出现
- 按 `Ctrl+P` 打开命令面板再 `Esc`，或拖动/调整终端窗口大小后，**暂时恢复**，过一会儿又出现
- 选中复制出来的文字是完整的 → **数据没坏，纯显示问题**

## 一句话结论

opencode 的 TUI 渲染引擎在"局部重绘"时把汉字的**显示宽度（2 列）**算错，导致部分宽字符的格子被刷成空白。触发条件与增量刷新有关，强制全量重绘即可暂时恢复。

## 已知上游 issue（截至 2026-09-23 均未修复）

| Issue | 症状 |
|---|---|
| [#34021](https://github.com/anomalyco/opencode/issues/34021) | 局部重绘后 CJK 错位/乱码，`Ctrl+P` 整体重绘可临时修复（Win + Linux 都复现） |
| [#50547](https://github.com/anomalyco/opencode/issues/50547) | 宽字符在行中间被吞掉/截断（Windows conhost） |
| [#41274](https://github.com/anomalyco/opencode/issues/41274) | 弹层（命令面板等）的遮罩会让背景层 CJK 变空白，ASCII 正常 |
| [#29007](https://github.com/anomalyco/opencode/issues/29007) | 同上，已定位到遮罩用半透明黑 `RGBA(0,0,0,150)` 的渲染缺陷 |
| [#41678](https://github.com/anomalyco/opencode/issues/41678) | VS Code 集成终端 + 中文输入，单个 CJK 字符空白 |

## 可以排除的原因

| 猜测 | 为什么不是 |
|---|---|
| 终端字体缺 CJK 字形 | 缺字形会显示"豆腐块"（□）或问号，不是"透明"；本项目用的是 JetBrainsMono Nerd Font Mono（无 CJK，靠系统回退字体），显示其余汉字完全正常 |
| 编码/locale 问题 | `LANG=C.UTF-8` 正常；编码坏了会整段乱码成 `锟斤拷`，不是"部分空白" |
| VS Code 终端独有 | 官方 issue 在 Windows Terminal / WezTerm / GNOME Terminal 都复现，说明在 opencode 自身渲染层 |

## 临时缓解（即刻可用）

1. **强制整体重绘**：`Ctrl+P` → `Esc`；或拖动窗口边缘改变终端尺寸
2. 高频出现时可以养成习惯：滚动看旧消息前先刷一次
3. 备选：换到 Windows Terminal 跑 `opencode`（不影响根治，但可对比验证）

## 本地已做的实验（结果待观察）

- 2026-09-23：VS Code 用户设置加 `"terminal.integrated.gpuAcceleration": "off"`（禁用终端 GPU 渲染，排除 WebGL 字形图集丢字）
  - 改完需**新开终端/重载窗口**才生效
  - 若无改善，可回退（改回 `"auto"` 或删除该行）；备份在 `settings.json.bak-20260923`（Windows 侧 VS Code 用户设置目录）
- 结论备注：官方 issue 表明根因在 opencode 渲染引擎，此实验大概率只能排除变量、不能根治

## 上报模板（可直接贴到 #34021 / #41274）

```text
Confirming this on VS Code integrated terminal:
- opencode: 1.18.32 (latest as of 2026-09-23)
- Terminal: VS Code integrated terminal (TERM_PROGRAM=vscode, TERM=xterm-256color, VS Code 1.138.0, WSL remote)
- LANG=C.UTF-8; terminal font JetBrainsMono Nerd Font Mono (no CJK coverage, falls back)
- Symptom: a large fraction (~40%) of CJK characters render as blank/transparent cells
  in both message body and dialogs; ASCII is unaffected; copy/paste text is intact.
- Confirmed workaround: Ctrl+P then Esc (full re-render), or resizing the terminal,
  restores the characters temporarily.
- Currently testing terminal.integrated.gpuAcceleration=off; will report back.
```

## 术语小词典

| 术语 | 白话解释 |
|---|---|
| TUI | Terminal UI，跑在终端里的图形界面（opencode 的界面） |
| 宽字符 / CJK | 汉字、日文、韩文在终端里占 **2 个字符格**，ASCII 占 1 格 |
| 局部重绘 / 全量重绘 | 终端只重新画"变了的那几行" vs 整屏重画；宽字符宽度算错就出在局部重绘 |
| 豆腐块（tofu） | 字体里没有该字形的占位方块，和"透明空白"是两回事 |
| 字形图集（glyph atlas） | GPU 渲染时把字形预烘焙成纹理贴图；贴图管理出错会造成个别字丢失 |

## 自测

> [!question]- 1. 怎么区分"字体缺字"和"opencode 渲染 bug"？
> 缺字显示为 □/� 且**永远**缺；渲染 bug 表现为"透明空白"，且 `Ctrl+P`→`Esc` 强制重绘后会**暂时恢复**——因为文字数据本身是好的。

> [!question]- 2. 为什么复制粘贴出来是完整的，屏幕上却是空白？
> 终端里有两层：底层的字符缓冲区（复制时读的是它）和屏幕上的像素。bug 出在"把缓冲区画到屏幕"的渲染环节，所以数据完好、显示残缺。

## 相关

- [[somezhishi/somezhishi|somezhishi 库索引]]
- [[somezhishi/工具速查/opencode-TUI-速查表|opencode TUI 速查表]]
- [[somezhishi/环境排障/opencode-自签证书报错-排查与禁用校验风险|opencode 自签证书报错]]（同日另一问题）
