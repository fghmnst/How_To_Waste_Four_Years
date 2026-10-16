---
date: 2026-08-06
tags:
  - 速查
  - 工具
---

# Translate Pro 快捷键速查表

**日期**：2026-08-06
**场景**：VS Code 插件 Translate Pro（`yxw007.vscode-translate-next` v1.8.0）划词 / 悬停翻译快捷键。

## 快捷键

| 功能 | Windows/Linux | macOS | 用途 |
|------|---------------|-------|------|
| 替换翻译选中文本 | `Shift+Alt+T` | `Shift+Alt+T` | 翻译选中的文本并原地替换 |
| 切换目标语言 | `Ctrl+Alt+Shift+L` | `Cmd+Alt+Shift+L` | 切换译文语言（如简体中文） |
| 切换默认翻译引擎 | `Alt+Shift+E` | `Alt+Shift+E` | 在 Google / Bing / 百度 / DeepL 等引擎间切换 |
| 查看插件日志 | `Ctrl+Alt+Shift+O` | `Cmd+Alt+Shift+O` | 打开翻译输出面板 |
| 清理插件日志 | `Ctrl+Alt+C` | `Cmd+Alt+C` | 清空翻译输出面板 |
| 翻译终端选中文本 | ``Ctrl+Alt+` `` | ``Cmd+Alt+` `` | 翻译终端里选中的文本 |
| 清理终端翻译日志 | `Alt+C` | `Alt+C` | 清空终端翻译记录 |
| 打开终端翻译面板 | `Alt+Shift+O` | `Alt+Shift+O` | 打开活动栏的终端翻译视图 |
| 启用/禁用 Hover 翻译 | `Ctrl+Alt+E` | `Cmd+Alt+E` | 开关「悬停即译」 |
| 切换语言检测 | `Ctrl+Alt+T` | `Cmd+Alt+T` | 开关语言自动检测 |

> [!note] 规律
> Windows/Linux 用 `Ctrl` 的组合键，macOS 基本是 `Cmd` 同位置；单键组合（如 `Alt+Shift+E`）两平台一致。

## 无需快捷键的操作

| 操作 | 入口 |
|------|------|
| 翻译插件详情（外文 README 中英对照） | 扩展面板 → 右键插件 → 「翻译插件详情」 |
| Markdown 预览沉浸式翻译 | 打开 `.md` 文件 → 编辑器标题栏按钮 |
| 一键替换所有注释 | 编辑器右键菜单 / 命令面板 |

## 使用提示

- 默认翻译引擎为 **Bing**：免费、免配置、无网络限制；Google 需能访问谷歌才可用。
- **百度翻译**每月 100 万字符免费、速度快，**已接入**（2026-08-06）。接入教程见 [配置百度翻译引擎](https://github.com/yxw007/vscode-translate-next/blob/HEAD/course/zh/config-engine/baidu.md)。
- 首次使用需在命令面板执行 `Translate Pro: Login`，到 translate.yanxuewen.cn 注册登录。
- Hover 翻译默认覆盖 `c/h/cpp/py` 等主流语言文件（配置 `Translate-next.hover.extensions`，设 `*` 可对所有文件生效）。

相关：[[归档/30天学习/30天学习 Index]] · [[归档/30天学习/术语表/术语表|术语表]]