---
date: 2026-09-15
tags:
  - 入门
  - 工具
---

# 构建工具链：CMake、make、Ninja 与 arm-none-eabi-gcc

> [!note] 缘起
> 本文整理自一次真实报错：VS Code 配置 STM32 工程 `DDM_test` 时，CMake 报
> `CMake was unable to find a build program corresponding to "Ninja". CMAKE_MAKE_PROGRAM is not set.`
> 排查结论：代码没问题，只是环境里缺一个叫 `ninja` 的命令。借此把整条构建工具链的概念理清楚。

## 一、前置知识：apt 装工具时要知道的两件事

### 1. `apt update` 与 `apt upgrade` 的区别

| | `sudo apt update` | `sudo apt upgrade` |
|---|---|---|
| 做什么 | 从软件源下载最新的**包索引**（有哪些包、什么版本、从哪下载），写入 `/var/lib/apt/lists/` | 对照本地索引，把**已安装的包**升级到新版本（下载并安装 `.deb`） |
| 是否改动系统 | 不改。只刷新「目录」 | 会改。实际替换系统上的程序与库 |
| 典型场景 | 装包前刷新索引；报 `Unable to locate package` 时 | 日常维护、安全更新 |

- 顺序关系：先 `update` 刷新目录，再 `upgrade` 按目录换货；`sudo apt update && sudo apt upgrade` 是最常见搭配。
- 只 `upgrade` 不 `update`：拿旧目录换货，看不到新版本。
- `upgrade` 默认不删除包，也不为满足依赖而新增/删除包；需要动用增删才能完成升级时用 `apt full-upgrade`。
- 安装单个包前如果索引太旧，可能报 `E: Unable to locate package xxx`——先 `sudo apt update` 再装。

### 2. 包名 ≠ 命令名

apt 装的「包名」和装完后能敲的「命令名」经常不一样，这正是本次踩坑的直接原因：

| 工具 | 包名 | 装完后的命令 |
|---|---|---|
| Ninja 构建工具 | `ninja-build` | `ninja` |
| ARM 交叉编译器 | `gcc-arm-none-eabi` | `arm-none-eabi-gcc` |

> [!warning] 踩坑记录
> `sudo apt install ninja` 装不上：Debian 体系里 `ninja` 这个包名曾被另一个无关软件（网络入侵检测系统）占用，当前 Ubuntu 软件源中干脆没有叫 `ninja` 的包。构建工具的包名必须写 `ninja-build`。

### 3. 本机为这条工具链执行的安装命令

```bash
sudo apt update
sudo apt install -y ninja-build gcc-arm-none-eabi libnewlib-arm-none-eabi
```

- `-y`：对 apt 的确认询问自动回答 yes，适合非交互执行（sudo 密码仍需人工输入）。
- `gcc-arm-none-eabi` 是编译器主包，会自动带上 `binutils-arm-none-eabi`（链接器 ld、objcopy 等）。
- `libnewlib-arm-none-eabi`：C 标准库 newlib；链接参数 `--specs=nano.specs` 需要它。

验证安装结果：

```bash
ninja --version              # 期望输出 1.11.1
arm-none-eabi-gcc --version  # 期望输出 13.2.1
```

装好后在 VS Code 执行 `CMake: Delete Cache and Reconfigure`，或删掉 `DDM_test/build/Debug` 重新配置。

## 二、CMake、make、Makefile 的区别与联系

三者在同一条流水线上，各管一段，**不是同类东西**。

### Makefile 是什么

**写给 make 的构建配方**（文本文件，不是程序）。基本结构是「目标: 依赖」加缩进的「命令」：

```make
main.o: main.c
	gcc -c main.c -o main.o
```

含义：`main.o` 依赖 `main.c`，当 `main.c` 比 `main.o` 新时执行这条命令。Makefile 本身没有逻辑能力，只是规则清单。

### make 是什么

GNU 的构建执行器：读 Makefile，比较**目标与依赖的时间戳**，只执行需要更新的命令——这就是「增量构建」（改一个文件只重编它，而不是全量重编）。make 不懂 C 语言、不探测环境，你写什么命令它执行什么。

### cmake 是什么

**构建系统的生成器**（meta-build，不亲自编译）。读 `CMakeLists.txt`，探测平台/编译器/第三方库，生成某个底层构建工具能直接执行的文件：

| 生成器参数 | 生成的文件 | 由谁执行 |
|---|---|---|
| `-G "Unix Makefiles"` | Makefile | make |
| `-G Ninja` | build.ninja | ninja |
| Visual Studio（Windows） | `.sln` 工程 | MSBuild / VS |

CMake 解决的是「手写 Makefile 跨平台、拆分模块、找库太痛苦」的问题，并提供 `find_package`、target 依赖模型等能力。`CMakePresets.json` 则是把「生成器 + 构建类型 + 工具链文件」等配置组合成命名预设（Debug/Release），供命令行和 VS Code 直接选用。

### 链路图（以 DDM_test 为例）

```
CMakeLists.txt + CMakePresets.json + cmake/gcc-arm-none-eabi.cmake
   │  cmake configure（生成）
   ▼
build/Debug/build.ninja（构建文件）
   │  ninja（真正调度编译器）
   ▼
arm-none-eabi-gcc → .o 文件 → 链接 → 固件 .elf → objcopy → .bin/.hex
```

### 使用场景

- **小项目 / 学习阶段手写 Makefile**：能真正理解编译链接过程，`C-tools/` 阶梯的约定就是纯 C + Makefile。
- **多文件、多平台、需要探测库**：用 CMake；工业界与 STM32CubeMX 生成的工程都用它。

## 三、Ninja 是什么，ninja-build 又是什么

- **Ninja**：一个构建工具，和 make **同类**（读构建文件、调度编译命令、增量构建），但为速度设计：
  - 启动开销小、默认并行、依赖图一次性读入内存，擅长大量文件的快速增量构建（Chromium、LLVM 这类大工程用它）。
  - `build.ninja` 语法极简且**不给人手写**，基本只由 CMake / Meson 等生成器产出。
  - 同一份 CMake 工程，Ninja 通常比 make 快，所以 CubeMX 生成的预设默认 `"generator": "Ninja"`。
- **ninja-build**：只是 Debian/Ubuntu 的**包名**（见「包名 ≠ 命令名」），装出来的命令仍然是 `ninja`（位于 `/usr/bin/ninja`）。

## 四、arm-none-eabi-gcc 是什么

它是 GNU 交叉编译工具链里的 C 编译器。名字拆开就是定义：

| 片段 | 含义 |
|---|---|
| `arm` | 目标 CPU 架构是 ARM |
| `none` | 目标上没有操作系统（裸机 bare-metal） |
| `eabi` | Embedded Application Binary Interface：规定函数调用约定、寄存器用法、栈对齐等底层规则 |
| `gcc` | GNU Compiler Collection（GNU 编译器集合） |

### 交叉编译

它运行在 x86_64 Linux（宿主）上，产出的却是 ARM 机器码（目标）——「在这台电脑上为另一台设备编译」。因此不能和桌面 gcc 混用：桌面 gcc 编出来的程序 STM32 不认。

### 一家子工具

| 命令 | 作用 |
|---|---|
| `arm-none-eabi-gcc` | 编译 C |
| `arm-none-eabi-g++` | 编译 C++ |
| `arm-none-eabi-ld`（通常经 gcc 驱动） | 按链接脚本 `.ld` 把 .o 排布成固件 |
| `arm-none-eabi-objcopy` | `.elf` → `.bin` / `.hex`（烧录格式） |
| `arm-none-eabi-size` | 查看 Flash / RAM 占用 |
| `arm-none-eabi-gdb` | 调试 |

### 在本仓库的对应

- `DDM_test/cmake/gcc-arm-none-eabi.cmake` 把工具链前缀写死为 `arm-none-eabi-`，并要求它在 PATH 中；
- 该文件里 `-mcpu=cortex-m3` 指定 CPU 核心（STM32F103）、`-T STM32F103xx_FLASH.ld` 指定 Flash/SRAM 布局、`--specs=nano.specs` 使用精简版 C 库以省空间。

### 使用场景

STM32 全系、树莓派 Pico（Cortex-M0+）、nRF 等 Cortex-M 裸机开发。桌面 Linux 程序编译用系统 gcc（`x86_64-linux-gnu-gcc`），两者目标不同、不可互换。

## 五、速查表

| 目的 | 命令 / 位置 |
|---|---|
| 判断 ninja 是否可用 | `which ninja`、`ninja --version` |
| 判断交叉编译器是否可用 | `arm-none-eabi-gcc --version` |
| 安装两者（含 C 库） | `sudo apt install -y ninja-build gcc-arm-none-eabi libnewlib-arm-none-eabi` |
| 刷新包索引 | `sudo apt update` |
| 查看生成器预设（Ninja） | `DDM_test/CMakePresets.json` 的 `"generator": "Ninja"` |
| 查看工具链配置 | `DDM_test/cmake/gcc-arm-none-eabi.cmake` |
| 重新配置工程 | VS Code：`CMake: Delete Cache and Reconfigure` |

## 关联

- [[somezhishi/somezhishi|somezhishi]]
- [[somezhishi/项目笔记/STM32工具链-CubeMX与CubeCLT详解|STM32 工具链：CubeMX 与 CubeCLT 详解]]
- [[somezhishi/环境排障/VSCode-C++插件-IntelliSense配置与gcc-g++区别|VS Code C/C++ 插件：IntelliSense 配置与 gcc/g++ 区别]]
