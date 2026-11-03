---
date: 2026-10-30
tags:
  - 指南
  - 项目
---

# 数据结构练习仓库：data_struc

> 一句话：`c_exercises/data_struc/` 是我跟**浙大陈越《数据结构》**刷题的专用练习场——每讲尽量落一题，能编译、能跑、能回看。

## 它是什么

- 位置：`~/SomeThingFunny/projects/c_exercises/data_struc/`（git 根是 `projects`，这里**没有独立 git 仓库**）。
- 用途：存放数据结构课程练习；当前主线是**浙大陈越《数据结构》MOOC + PTA**，Yale CPSC 223 只作补充资源。
- 来历：2026-10-20 由独立仓库 `~/SomeThingFunny/data_struc` 并入 `projects`，去掉独立 `.git`，原仓库留在原位置作历史归档，避免两份代码漂移。
- 定位：**练习场，不是生产项目**。优先“今天这一讲能写一题并跑通”，不追求工程化。

## 目录长什么样

```text
c_exercises/data_struc/
└── exercise/                  # 所有练习都放这里
    ├── .gitignore             # 忽略任意层级的 out/ 与二进制
    ├── README.md              # 目录地图 + 编译命令
    ├── 01-maxsubseq/          # 复杂度 & 最大子列和
    │   ├── 1_MaxSubseqSum.c
    │   ├── 2_MaxSubseqSum.c
    │   ├── 3_MaxSubseqSum.c
    │   ├── 4_MaxSubseqSum.c
    │   ├── duibi.c
    │   ├── PTA_1MSS.c
    │   └── plan.md
    ├── 02-recursion/          # 递归 vs 循环
    │   ├── for_printN.c
    │   └── recursion_printN.c
    ├── 03-linkedlist/         # 线性表 · 链表
    │   ├── LinkedList.c
    │   └── PTA_mergeTwoLists.c
    └── out/                   # 编译产物，已忽略
```

## 当前题目与状态

| 文件 | 练的是什么 | 状态 |
|---|---|---|
| `01-maxsubseq/1_MaxSubseqSum.c` | 最大子列和：暴力穷举，$O(n^3)$ | 可运行；`k < j` 应为 `k <= j`（已知 bug，见 `plan.md` §三） |
| `01-maxsubseq/2_MaxSubseqSum.c` | 最大子列和：优化穷举，$O(n^2)$ | 可运行 |
| `01-maxsubseq/3_MaxSubseqSum.c` | 最大子列和：分治，$O(n \log n)$ | **已完成**：`Max3` + `DivideAndConquer` 已接进 `MaxSubseqSum3` |
| `01-maxsubseq/4_MaxSubseqSum.c` | 最大子列和：在线处理，$O(n)$ | **已完成** |
| `01-maxsubseq/duibi.c` | 多项式求值计时：朴素 `pow` 与秦九韶（Horner）对比 | 可运行；需 `-lm` |
| `01-maxsubseq/PTA_1MSS.c` | PTA 最大子列和 | 已完成 |
| `02-recursion/for_printN.c` / `recursion_printN.c` | 循环 vs 递归打印 `1..N` | 可运行；`N` 从标准输入读取 |
| `03-linkedlist/LinkedList.c` | 最小链表：3 个结点手工串联 | 可运行，输出 `3 / 4 / 5` |
| `03-linkedlist/PTA_mergeTwoLists.c` | PTA 02-线性结构1 两个有序链表合并（本地可跑版） | 样例 + 3 组边界通过；待交 PTA |

> [!note] 命名与分组
> 2026-10-30 起按**主题分子文件夹**（`01-maxsubseq` / `02-recursion` / `03-linkedlist`），**旧文件保持原名不改**，保 git 历史与回看习惯。
> 新增练习统一 `exampleN_<主题>.c` 或 `PTA_<题名>.c`；产物一律进 `out/`。

## 怎么编译运行

**一律在 `exercise/` 根目录执行**，产物统一放 `out/`（`.gitignore` 已按任意层级忽略）：

```bash
cd ~/SomeThingFunny/projects/c_exercises/data_struc/exercise
mkdir -p out

# 普通题
gcc -Wall -Wextra 03-linkedlist/LinkedList.c -o out/LinkedList
./out/LinkedList

# 需要 <math.h> 的题，记得加 -lm
gcc -Wall -Wextra 01-maxsubseq/duibi.c -o out/duibi -lm
./out/duibi

# 带输入的程序用管道喂数据
printf '3\n1 3 5\n5\n2 4 6 8 10\n' | ./out/PTA_mergeTwoLists
```

- `-Wall -Wextra`：把编译器当第一道错题本，警告不要无视。
- `-O2`：计时类练习要注意优化可能改变相对耗时，结论看数量级即可。
- `out/`：编译产物统一放这里，已被 `exercise/.gitignore` 忽略，不会误提交。
- 用到 `pow` 等数学函数时必须加 `-lm`，否则链接阶段会报未定义引用。

## 约定

- 一题一个 `.c` 文件，源码入库；可执行文件、`*.o`、`out/` 不入库。
- 按主题分文件夹；同一讲的题放同一个文件夹，新主题新建文件夹。
- 不引入 Makefile / CMake；一两条 `gcc` 命令能解决的事，不搭构建系统。
- 保留失败尝试和调试过程；旧练习不删，方便回看思路变化。
- git 操作一律在 `projects` 根执行；提交前先看 `git status` / `git diff`。
- 当前练习不依赖第三方库，Termux + clang 也能直接跑。

## 下一步

1. **交 PTA 02-线性结构1**：`03-linkedlist/PTA_mergeTwoLists.c` 本地已通过，直接提交函数部分即可。
2. **推进第 2 章**：陈越 2.2-2.4（堆栈、队列、应用），每讲落一题进 `04-*` 新文件夹。
3. **补边界用例**：全负数、只有一个正数、最大值在开头/结尾、空输入等，至少各跑一次。

## 术语小词典

| 术语 | 大白话 |
|---|---|
| 最大子列和 | 在一串数字里找一段**连续**区间，让这段的和最大；全负数时按“允许空子列”返回 0 |
| 暴力穷举 | 把所有起点、终点都试一遍，慢但最好写 |
| 分治 | 把区间从中间劈开，分别算左半、右半、跨中线，再取最大 |
| 在线处理 | 从左到右扫一遍，边扫边丢“拖后腿”的前缀，$O(n)$ |
| 秦九韶 / Horner | 把多项式改写成“一层层乘 $x$ 再加系数”，少算很多次幂 |
| 编译产物 | `gcc` 生成的可执行文件或 `.o`，不属于源码，不进 git |
| 带头结点 | 头结点不存数据，只为让空表也有东西可指，省掉大量判空 |

> [!question]- 自测
> 1. 为什么 `duibi.c` 编译要加 `-lm`，而 `for_printN.c` 不用？
> 2. 最大子列和的全负数数组，为什么很多教材规定返回 0 而不是最大的负数？
> 3. 分治算法里“跨中线”的最大和，为什么只要从中间向左右各扫一遍？
> 4. 如果直接把 `out/duibi` 提交进 git，会带来什么问题？

## 关联

- [[2026-2027 大二上学期学习计划|大二上学习计划]]（§四 数据结构）
- [[课程笔记/数据结构/数据结构|数据结构笔记]]（链表 / typedef / 双指针归并）
- [[somezhishi/项目笔记/数据结构学习线-Yale-CPSC223|数据结构学习线：Yale CPSC 223（补充资源）]]
- [[2026秋 Index]]
- [[somezhishi/somezhishi|somezhishi]]
