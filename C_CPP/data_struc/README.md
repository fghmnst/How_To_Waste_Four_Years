# 数据结构练习：陈越《数据结构》MOOC + PTA

浙大陈越《数据结构》的刷题记录。一题一个 `.c` 文件，单文件编译，不引入 Makefile / CMake。

这个目录的定位是**练习场**，不是工程：目标是「今天这一讲能写一题、能跑通、能回看」，不追求三行命令变一行。

## 目录

| 文件夹 | 主题 | 对应讲次 |
|---|---|---|
| `01-maxsubseq/` | 算法复杂度、最大子列和 | 第 1 章（1.1-1.3） |
| `02-recursion/` | 递归 vs 循环（printN） | 第 1 章（1.1） |
| `03-linkedlist/` | 线性表 · 单链表 | 第 2 章（2.1-2.2） |

## 文件清单

### 01-maxsubseq —— 同一个问题的四种解法

| 文件 | 解法 | 复杂度 | 一句话 |
|---|---|---|---|
| `1_MaxSubseqSum.c` | 暴力穷举 | $O(n^3)$ | 枚举所有起点终点，第三层循环求和 |
| `2_MaxSubseqSum.c` | 优化穷举 | $O(n^2)$ | 固定起点往后累加，省掉第三层 |
| `3_MaxSubseqSum.c` | 分治 | $O(n\log n)$ | 左半、右半、跨中线，三选一 |
| `4_MaxSubseqSum.c` | 在线处理 | $O(n)$ | 边走边丢「拖后腿」的前缀 |
| `duibi.c` | 多项式求值计时 | — | 朴素 `pow` 与秦九韶（Horner）对比 |
| `PTA_1MSS.c` | PTA 最大子列和 | $O(n)$ | 判题版：边读边算，不存数组 |

> 四份 `*_MaxSubseqSum.c` 都是自带 `main` 的独立小演示，数组写死在代码里，方便直接跑着看输出。想做规模实验，见下面的「已知问题」和 [思路与坑清单](01-maxsubseq/思路与坑清单.md)。

### 02-recursion —— 递归 vs 循环

| 文件 | 内容 |
|---|---|
| `for_printN.c` | 用 `for` 打印 `1..N` |
| `recursion_printN.c` | 用递归打印 `1..N`（先递归到底，再逐层打印） |

两段代码输出完全一样，但内存里的过程不同：循环只占一个变量，递归会压 N 层栈。`N` 从标准输入读，可以试试输 `100000` 看递归会发生什么。

### 03-linkedlist —— 单链表

| 文件 | 内容 |
|---|---|
| `LL_test.c` | 最小链表：栈上 3 个结点手工串联，输出 `3 / 4 / 5` |
| `LL_test_malloc_ver.c` | 同上，但三个结点改成 `malloc` 在堆上申请，用完 `free` |
| `LL_test_malloc_easymerge_ver.c` | 两条链表（3,4,5 与 6,7,8）首尾相接，最朴素的「合并」 |
| `mergeLL.c` | 按课件风格实现 `Merge`：带头结点 + `Read` / `Print`，真正按值归并 |
| `PTA_mergeTwoLists.c` | PTA 02-线性结构1 两个有序链表序列的合并，本地可跑版 |

`PTA_mergeTwoLists.c` 把判题机隐藏的 `Read` / `Print` / `main` 按题面还原了出来，方便本地调试；交 PTA 时只需要贴最后那个 `Merge()`。

## 编译运行

**在本目录（`data_struc/`）执行**，产物统一丢进 `output/`（`.gitignore` 已忽略任意层级的 `output/`）：

```bash
mkdir -p output

# 普通题
gcc -Wall 03-linkedlist/LL_test.c -o output/LL_test && ./output/LL_test

# 用到 <math.h> 的题要加 -lm，否则链接阶段报「未定义引用」
gcc -Wall 01-maxsubseq/duibi.c -o output/duibi -lm && ./output/duibi
```

带输入的程序用管道喂数据：

```bash
printf '3\n1 3 5\n5\n2 4 6 8 10\n' | ./output/PTA_mergeTwoLists
```

- `-Wall`：把编译器当第一道错题本，警告不要无视。
- `-lm`：`pow` 这类数学函数在单独的库里，不写 `-lm` 链接不过。
- `output/`：编译产物统一放这里，不会误提交。

## 命名约定

- 新文件用 `exampleN_<主题>.c` 或 `PTA_<题名>.c`
- 旧文件保持原名不改，保 git 历史和回看习惯
- 源码入库，可执行文件 / `*.o` / `output/` 不入库

## 已知问题（故意留着）

有两处 bug 没修，留作「踩过」的证据，也方便回看当时的思路：

1. `1_MaxSubseqSum.c` 内层循环写成 `k < j`，应该是 `k <= j`——`A[j]` 永远没被加进去，等价于只对前 `N-1` 个数求和。用 `{-1, 5}` 一测就露馅（返回 0，正确答案 5）。
2. `2_MaxSubseqSum.c` 的函数名还是复制来的 `MaxSubseqSum1`，没改成 `MaxSubseqSum2`。

原因、影响和修法都写在 [`01-maxsubseq/思路与坑清单.md`](01-maxsubseq/思路与坑清单.md) 里。

## 为什么按主题分文件夹

按知识点找题（「链表那题在哪」）比按时间找更符合复习习惯。文件夹名用 ASCII，避免不同工具链在中文路径上的兼容问题；中文含义见上面的目录表。
