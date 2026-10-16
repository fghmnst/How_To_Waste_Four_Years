---
date: 2026-09-24
tags:
  - 速查
  - 工具
---

# sqlite3 单文件数据库命令行

**日期**：2026-09-24
**场景**：排查 opencode session 时发现本机没有 `sqlite3` 命令，改用 Python 兜底读取 `opencode.db`——顺手把「SQLite 是什么、sqlite3 CLI 怎么用」整理成一篇。

## 零、30 秒理解

你手机里的「备忘录」App，数据其实存在**一个文件**里——不需要一个「备忘录服务器」在后台跑，App 直接读写那个文件就行。

**SQLite 就是这种「单文件数据库」**：整个数据库（表、数据、索引）全部装在一个文件里，拷文件 = 拷走整个库。

**sqlite3 CLI** 是官方配的**命令行操作台**：敲 `sqlite3 某文件.db`，你就坐到操作台前，可以直接输入 SQL 指令查数据。典型例子——opencode 的全部会话记录就存在：

```
~/.local/share/opencode/opencode.db
```

## 一、基本用法

```bash
sqlite3 ~/.local/share/opencode/opencode.db   # 打开数据库（进入交互模式）
```

进入后常用指令（点开头的 `.` 是 sqlite3 CLI 自己的命令，分号结尾的大写是标准 SQL）：

| 指令 | 作用 | 读法 |
|---|---|---|
| `.tables` | 列出库里所有表 | 「看看里面有哪些工作表」 |
| `.schema 表名` | 看某表有哪些列 | 「这张表的表头长什么样」 |
| `SELECT * FROM 表名 LIMIT 3;` | 看某表前 3 行 | 「挑出某表前 3 行给我看」 |
| `.headers on` / `.mode column` | 让输出带列名、对齐 | 「查询结果排版好看点」 |
| `.quit` | 退出 | 「下班」 |

## 二、只读打开（推荐习惯）

排查别人/程序的数据库时，一律**只读打开**，避免手滑改坏：

```bash
sqlite3 "file:/path/to/xxx.db?mode=ro"          # URI 只读方式打开
sqlite3 -readonly /path/to/xxx.db                # 或用 -readonly 旗标（较新版本支持）
```

读法：`?mode=ro` = read only，数据库只进不出，查询随便、写入报错。

## 三、本机没装怎么办：Python 兜底

WSL Ubuntu 默认不带 sqlite3 CLI，但 **Python 自带 `sqlite3` 模块**，能力等价：

```python
import sqlite3
db = sqlite3.connect("file:~/.local/share/opencode/opencode.db?mode=ro", uri=True)  # uri=True 才认 file:... 只读写法
for row in db.execute("SELECT name FROM sqlite_master WHERE type='table'"):          # 等价于 .tables
    print(row[0])
```

| 对比 | sqlite3 CLI | Python `sqlite3` 模块 |
|---|---|---|
| 安装 | 需 `sudo apt install sqlite3` | 开箱即用（Python 内置） |
| 用法 | 交互式敲 SQL | 写脚本批量处理 |
| 适合 | 快速逛库、肉眼查数据 | 自动化提取、接进工作流 |

## 四、想装 CLI（用户自行执行）

```bash
sudo apt install sqlite3
```

- **验证**：`sqlite3 --version` 能打印版本号（如 `3.45.x`）即成功
- **可能的问题**：报 `Unable to locate package` → 先 `sudo apt update` 刷新软件源列表再装
- **不装也行**：Python 兜底功能等价，只是少了「亲自进操作台逛」的便利

## 术语小词典

| 术语 | 大白话解释 |
|---|---|
| SQLite | 嵌入式数据库引擎，整个库就是一个文件，不需要后台服务 |
| sqlite3 CLI | 随 SQLite 发布的命令行操作台，交互式敲 SQL |
| SQL | 操作数据库的专用语言：`SELECT` 查、`INSERT` 增、`UPDATE` 改、`DELETE` 删 |
| 表（table） | 库里的一张「表格」，有列名和一行行数据 |
| `sqlite_master` | 每个 SQLite 库自带的「目录表」，记录库里有哪张表、什么结构 |
| `mode=ro` | 只读模式（read only），防手滑写坏排查对象 |

> [!question]- 自测
> 1. SQLite 和 MySQL 最大的区别？——MySQL 要先启动数据库服务器进程，SQLite 不用，数据就是一个随文件走的库
> 2. `sqlite3` 命令报 `command not found` 说明什么？——CLI 工具没装，但 Python 的 `sqlite3` 模块仍可用（语言内置）
> 3. `.tables` 和 `SELECT` 的区别？——`.tables` 是 sqlite3 CLI 自己的快捷命令；`SELECT` 是标准 SQL，任何支持 SQL 的工具都认
> 4. 为什么排查别人的库要用 `mode=ro`？——只读打开杜绝手滑写入，坏不了原始数据
