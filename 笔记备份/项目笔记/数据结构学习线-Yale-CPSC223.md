---
date: 2026-10-13
tags:
  - 指南
  - 项目
---

# 数据结构学习线：Yale CPSC 223

> 2026-10-13 主线重构后立项：**W6 起接替原 C 线**（ASCII 立方体取消）的主线课程；C 语言、资料全开源、有真实项目链。本卡是资源与规划的常驻入口，动工时直接看这份，不用重新调研。

> [!warning] 2026-10-18 主线调整
> 数据结构主线已切换为**浙大陈越《数据结构》MOOC + PTA**；Yale CPSC 223 与 K&R C 转为**补充资源/项目包**。当前练习落点与进度见 [[somezhishi/项目笔记/数据结构练习仓库-data_struc|data_struc 练习仓库]]（`c_exercises/data_struc/exercise/`）。本笔记保留为 Yale 资源入口，不再承担主线进度考核。

## 这是什么

Yale 大学 CPSC 223《Data Structures and Programming Techniques》（数据结构与编程技术）——**用 C 语言讲数据结构**：从 C 基本功（指针/字符串/动态内存）一路走到哈希表、树、堆、平衡树、图、动态规划，全程配「可交付的编程作业」。

为什么选它（PK 过的候选）：

| 维度 | Yale CPSC 223 | 说明 |
|---|---|---|
| 语言 | **纯 C** | 与 C/系统线、一生一芯同语言，不用「再学一门语言来学 DS」 |
| 资料开源 | 讲义全文 CC BY-SA + 代码示例 + **历年作业含样例解答** | 没有 autograder 也能对答案自查 |
| 实践项目 | [F2022 项目链 7 个](https://zoo.cs.yale.edu/classes/cs223/f2022/Assignments/index.html) | 有明确 spec、难度逐级递进 |
| 当时的取舍 | 61B（Java+英文双成本）放弃；陈越当时留作中文补充，2026-10-18 升为主线 | — |

## 资源清单

| 资源 | 链接 | 用途 |
|---|---|---|
| 主讲义（Aspnes notes） | <https://www.cs.yale.edu/homes/aspnes/classes/223/notes.html> | 主教材：Ch4=C 语言、Ch5=数据结构主体、§6.5=测试方法学、§6.4=C++ 过渡 |
| 历年作业 + 样例解答 | 同上讲义 Ch8-11（2022/2021/2018/2015 四套） | **做完对答案**的自查闭环 |
| 代码示例目录 | <https://www.cs.yale.edu/homes/aspnes/classes/223/examples/> | 讲义正文内嵌链接的源码 |
| F2022 项目链 | [作业索引](https://zoo.cs.yale.edu/classes/cs223/f2022/Assignments/index.html) | 按序做：Hello → ParseGPX → TSP → Heatmap → Blotto → k-d Tree → 路径 |
| F2022 讲义 PDF + 课堂代码 + 模拟考 | <https://zoo.cs.yale.edu/classes/cs223/f2022/Examples/index.html> | 卡壳时按主题点播（BST/AVL/红黑/trie/堆/k-d/图/Dijkstra…）；含两次模拟考及答案 |
| F2022 课程主页 | <https://zoo.cs.yale.edu/classes/cs223/f2022/> | 官方对「等价背景」的说明；页内含机器环境配置 Google Doc |
| 当前中文主线（2026-10-18 起） | [陈越《数据结构》MOOC](https://www.icourse163.org/course/ZJU-93001) · [hello-algo](https://www.hello-algo.com/) | 陈越 + PTA 为主线；hello-algo 作速查（含 C 代码） |
| 判题补强（可选） | [PTA 拼题A](https://pintia.cn/) | 想要自动判题时刷陈越配套题目集 |

### C 语言参考书（主教材 K&R + 辅读 Modern C）

| 书 | 定位 | 获取渠道 | 读法 |
|---|---|---|---|
| **K&R《C 程序设计语言》第 2 版** | 223 官方教材（Aspnes：「should memorize this book」） | 机工社英文影印版（便宜）/ 中译本（徐宝文等译）；两版章节号一致，可直接对照讲义引用（如 K&R §§5.1-5.4） | **英文原版主读 + 中译兜底**：英文句子短但密度高，卡壳段落对中译；术语对齐 223 讲义/作业/GitHub，顺带当 CET-6 泛读 |
| **Modern C（Jens Gustedt）** | 现代 C 辅读（非替代）：作者为 ISO C 标准委员会（WG14）成员；讲值语义、系统性规避 UB、标准库正确用法、C23 新特性；**不是零基础书** | 官网免费 CC 版（3rd 版/C23）：<https://gustedt.gitlabpages.inria.fr/modern-c/> → 正文 <https://hal.inria.fr/hal-02383654>（代码 MIT 许可）；Manning 印刷/电子：<https://www.manning.com/books/modern-c-third-edition>（官方折扣码 `au35gus`）；中译《现代C：概念剖析和编程实践》（刘红泉 译，机工社 2021，对应旧版 C17，不含 C23） | **只查「这样写算不算 UB / 有没有现代写法」**；223 作业是相对老风格，C23 特性用不上，别在作业里硬塞新特性 |

- K&R 配套答案：《C程序设计语言（第2版·新版）习题解答》（机工社；即 The C Answer Book 中译，汤朵/Tondo 等）——K&R 练习做完对答案，沿用「做完再对」纪律
- 降级策略（对齐计划风险表）：W6-7 若英文查词拖节奏 → 切「中译主读 + 英文对照术语」，原版执念不拖主线

### CPSC 223 历年学生仓库（GitHub）

Yale 专属，语言均为 C（缺件对照 / 做完对答案的补充源）：

| 仓库 | 学期 | 说明 |
|---|---|---|
| [benfoxman/CPSC223](https://github.com/benfoxman/CPSC223) | S2019 | psets 完整解答，首选对照 |
| [rkantamneni/CS223-DataStructures](https://github.com/rkantamneni/CS223-DataStructures) | S2021 | 作业整理 |
| [bluestme/CPSC-223](https://github.com/bluestme/CPSC-223) | S2021 | 作业工程 |
| [2matttu/gpx-parser](https://github.com/2matttu/gpx-parser) | F2019 | Assign1 同类（GPX 解析） |
| [alexichristakis/cs223](https://github.com/alexichristakis/cs223) | 2018 | 作业 |
| [Jack-Huang1/cs223](https://github.com/Jack-Huang1/cs223) | S2017 | 作业 |
| [bicep/CPSC223](https://github.com/bicep/CPSC223) | S2017 | 作业 |
| [hmm29/CPSC223](https://github.com/hmm29/CPSC223) | S2016 | 作业 |
| [isaacpena/CS223-C-Data-Structures](https://github.com/isaacpena/CS223-C-Data-Structures) | S2015 | 作业 |
| [willocov/CPSC223_DiffProject](https://github.com/willocov/CPSC223_DiffProject) | — | diff 实现 |
| [daniel-leibovic/compositor](https://github.com/daniel-leibovic/compositor) | 早年 | PSET5（ASCII 合成器） |

其他对照材料：

- 讲师课堂代码（Weide，GitHub 快照）：<https://github.com/alanweide/cs223-examples>
- 讲师课堂代码（Glenn，Bitbucket）：`cpsc223_distance / cpsc223_arrays_strings / cpsc223_adts / cpsc223_map / cpsc223_split`，入口见上文 Examples 页
- Zoo 环境与课程文档：<https://robb17.github.io/CPSC223Materials/>
- 作业风格指南：<https://zoo.cs.yale.edu/classes/cs223/f2022/Assignments/style.html>

> [!warning] 只认 Yale 版
> GitHub 上大量同名「CPSC223」是其他学校的课（CSUF 的 223C/223J/223P、Gonzaga 等），课程体系不同，别混着做。

### 前置课 CPSC 201 资源（了解用）

官方口径：223 前置为「CPSC 201 **或等价背景**」。现行 201（Slade 版）其实是 **Racket** 版计算导论课（可计算性/布尔/体系结构/编译/运行时），**不教 C**——所以不用"先刷完 201"再学 223：

| 资源 | 链接 |
|---|---|
| 课程主页（F2026） | <https://zoo.cs.yale.edu/classes/cs201/> |
| 授课视频（YouTube 频道） | <https://www.youtube.com/channel/UCTdMb_BAPvnh9T5SrFk9Ffw> |
| 讲义目录（HTML） | <https://zoo.cs.yale.edu/classes/cs201/fall_2026/lectures.html> |
| 学生作业（Racket，围观用） | [jasonlee8600/CPSC-201](https://github.com/jasonlee8600/CPSC-201) · [YangaB7/CS2010HW](https://github.com/YangaB7/CS2010HW) · [le-benjamin/cpsc2010](https://github.com/le-benjamin/cpsc2010) |

「等价背景」的真实门槛 = 能用 C 写出一两百行、含指针与动态内存的程序。快速自测：手写 `my_strdup` / 单链表 push-pop / 用 valgrind 查泄漏 / 多文件 Makefile 编译——哪个卡住就补 K&R（223 官方教材）对应章节。

## 学习规划（2026-10-13 版；2026-10-18 起转为补充线）

总量【估计】100–160h（讲义 40–60h + 项目 60–100h）。

| 阶段 | 预算 | 内容 | 产出 / 判据 |
|---|---|---|---|
| W6-7 | 4h/周 | Ch3-4 选读（指针/字符串/malloc/结构体）+ Assign0 + Assign1 开写 | Assign0 完成；Assign1 骨架跑通 |
| W8-9 | 4h/周 | Assign1 收尾 + notes 5.1-5.2（复杂度/动态数组） | Assign1 完成或 ≥80% → W9-10 门检查点 |
| W10-12 | 2h/周 | notes 5.3-5.4（递归/链表）+ Assign2 开写 | 首个 O(n³) 性能约束作业 |
| W13-15 | 2h/周 | 低密度维持（Assign2 收尾或 5.5 ADT） | 保住连续性（双实验+CET6 期） |
| W16-18 | — | 暂停（期末） | — |
| 寒假 | 50-70h | notes 5.6-5.11（哈希/泛型/树/堆/BST）+ Assign3/4 | 哈希+树落地 |
| 大二下 | 4h/周 | notes 5.13-5.15（平衡树/图/DP）+ Assign5/6 | 主线收尾 → 衔接 PA/ysyx F 段 |

**执行规则**：
- 周节奏：2h 读讲义 + 2h 写代码（周五/周末块）；每作业 README 记「预估 vs 实际耗时」
- **做完再看**历年样例解答核对；AI 只答疑不代写（诚信线）
- 代码落点：陈越主线练习在 `c_exercises/data_struc/exercise/`；Yale 补充作业如启用，另建 `c_exercises/cs223-*` 子目录；二进制/`.o` 进对应 `.gitignore`

## 已知坑与对策

- **Assign2+ 的 Required 文件在 Yale 内网路径**（`/c/cs223/hwN/Required`），校外不可直接下载 → 对策三层：① 按 spec **自行实现等价模块**（首选，等于加练）；② 用 [F2022 讲义 PDF + Bitbucket 课堂工程](https://zoo.cs.yale.edu/classes/cs223/f2022/Examples/index.html) 补 spec 细节；③ 对照历年学生仓库（benfoxman/CPSC223、Jack-Huang1/cs223 等，见「资源清单」节）的接口与行为——**做完再对**，AI 只答疑不代写
- 无视频、无 autograder → 自查 = 历年样例解答 + 自写单元测试（讲义 §6.5）
- 全英文、无 deadline → 纪律靠周复盘与 W9-10 门；Yale 补充线推不动时优先保陈越主线（+PTA）；单本 K&R 卡壳 → 中译对照（见「C 语言参考书」节）

## 关联

- [[2026-2027 大二上学期学习计划|大二上学习计划]]（§二/§四）
- [[somezhishi/项目笔记/数据结构练习仓库-data_struc|数据结构练习仓库：data_struc]]（当前主线练习落点）
- [[somezhishi/项目笔记/一生一芯-项目简介与观望记录|一生一芯：观望记录]]（同生态长线）
- [[somezhishi/项目笔记/ASCII旋转立方体-想做清单|ASCII 旋转立方体]]（原 C 线项目，已取消）
- [[somezhishi/somezhishi|somezhishi]]
