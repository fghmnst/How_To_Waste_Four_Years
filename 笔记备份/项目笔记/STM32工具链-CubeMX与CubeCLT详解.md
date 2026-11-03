---
date: 2026-09-15
tags:
  - 入门
  - 项目
---

# STM32 工具链：CubeMX 与 CubeCLT 详解

> [!note] 缘起
> 配置 `DDM_test` 工程时理清了 STM32 工具链的分工：谁负责「配芯片」，谁负责「编代码、进芯片」。顺带把 `arm-none-eabi-gcc` 的几种下载方式和本机工具现状一并盘点。

## 一、两者定性

两者**不在同一环节**：CubeMX 是「芯片怎么配、代码骨架长什么样」的**配置与代码生成器**（GUI）；CubeCLT 是「代码怎么变成固件、怎么进芯片」的**命令行工具集**（CLI）。

| 维度 | STM32CubeMX | STM32CubeCLT |
|---|---|---|
| 全称 | Cube Microcontroller eXpander | Cube Command Line Tools |
| 形态 | 图形界面程序（Java 编写，依赖 JRE，见 [[somezhishi/编程基础/Linux权限-chmod详解\|Linux 权限：chmod]] 的 JRE 案例） | 一组命令行工具打包，无界面 |
| 核心输入 | `.ioc` 文件（引脚、时钟树、外设、中间件配置） | 源码 + 构建配置（CMakeLists / Makefile / 工具链文件） |
| 核心输出 | HAL 初始化代码（`Core/`）、启动文件、链接脚本 `.ld`、CMake 工程骨架 | `.elf/.bin/.hex` 固件、烧录进芯片、GDB 调试会话 |
| 包含什么 | 外设/时钟配置器、代码生成模板、固件包（HAL 库）管理 | arm-none-eabi GCC、CMake、Ninja、STM32CubeProgrammer CLI、ST-LINK GDB server 等 |
| 会不会编译 | 不会 | 会（本职工作） |
| 面向谁 | 人（图形化配置） | 脚本 / CI / VS Code 等外部工具（命令行调用） |
| 一句话 | 「生成工程」 | 「构建 + 烧录 + 调试」 |

## 二、联系

1. **上下游衔接**：CubeMX 生成的 CMake 工程（`CMakeLists.txt`、`CMakePresets.json`、`cmake/gcc-arm-none-eabi.cmake`）正是 CLT 里那套 gcc + cmake + ninja 的输入；`DDM_test` 就是这个结构。构建概念见 [[somezhishi/编程基础/构建工具链-CMake与make与Ninja详解\|构建工具链：CMake、make 与 Ninja]]。
2. **闭环工作流**：CubeMX 配 `.ioc` → Generate Code → 构建 → 烧录调试 → 要改外设 → 回 CubeMX 重新生成；用户代码写在 `USER CODE BEGIN/END` 区内可避免被覆盖。
3. **替代关系**：CubeCLT 是「无 IDE 纯命令行」场景的全家桶；VS Code 的 STM32 扩展 3.10 已用 **bundle 管理器按需下载工具，取代 all-in-one 的 CubeCLT**——相当于「拆散版 CLT」。

## 三、使用场景

- **只用 CubeMX**：配引脚/时钟/外设、生成初始化代码、学习 HAL 骨架。
- **只用 CLT**：已有工程、无 GUI 环境（服务器/CI）、想一条命令装齐 gcc + cmake + ninja + 烧录器。
- **两者配合**（官方推荐的 VS Code 流）：CubeMX 管配置，CLT（或 bundle 工具）管构建烧录。
- **都不用**：STM32CubeIDE 一体化 IDE——与本仓库已定工具链（vscode + CubeMX + CMake）不符。

## 四、arm-none-eabi-gcc 的下载方式

### 方式 1：apt 发行版包（推荐，最省事）

```bash
sudo apt update
sudo apt install -y gcc-arm-none-eabi libnewlib-arm-none-eabi
```

- Ubuntu 24.04 源里是 **13.2.1**，编译 F103 完全够用；装到 `/usr/bin`，天然在 PATH 里，CMake 直接能找到。
- 验证：`arm-none-eabi-gcc --version`；`ls /usr/lib/arm-none-eabi/newlib/nano.specs`（DDM_test 链接参数需要）。
- 报 `Unable to locate package` 时先 `sudo apt update`。

### 方式 2：ARM 官方 ARM GNU Toolchain（要最新官方原版）

- 官方下载页（选 x86_64 Linux hosted / arm-none-eabi target）：<https://developer.arm.com/downloads/-/arm-gnu-toolchain-downloads>
- 装法（手动，多版本可共存）：

```bash
mkdir -p ~/opt && tar -xf ~/Downloads/arm-gnu-toolchain-*-x86_64-arm-none-eabi.tar.xz -C ~/opt
echo 'export PATH=$HOME/opt/arm-gnu-toolchain-*-x86_64-arm-none-eabi/bin:$PATH' >> ~/.bashrc && source ~/.bashrc
```

- 注意：不加入 PATH，CMake 同样找不到（与缺 Ninja 是同类坑）。

### 方式 3：STM32 生态 bundle（VS Code 扩展内）

- 命令面板 → **「STM32Cube: 打开系统 STM32Cube Bundles 管理器」** → 安装 `gnu-tools-for-stm32`（ST 定制版，如 14.3.1+st.2），装到 `~/.local/share/stm32cube/bundles/`。
- 坑：该 bundle 目录默认不在 PATH，要么走扩展自己的流程，要么手动把 `bin` 加进 PATH。

### 方式 4：STM32CubeCLT（ST 官方全家桶）

- ST 官网搜索 STM32CubeCLT 下载（需注册登录），内含 GCC + GDB + ninja + cmake + CubeProgrammer CLI + ST-LINK server。
- 适合「绕过 apt、一步到位 ST 官方工具链」；缺点是与已有 apt 工具功能重叠、体积大。

### 附：`gcc-arm-none-eabi` 与 `libnewlib-arm-none-eabi` 的分工

两者**不是竞争关系**，是链接同一条命令时互补的两件东西：

| | `gcc-arm-none-eabi` | `libnewlib-arm-none-eabi` |
|---|---|---|
| 角色 | 编译器：`.c/.S` → `.o`，并驱动链接 | 裸机 C 标准库 + 数学库 + 启动/桩代码 |
| 关键内容 | `arm-none-eabi-gcc/g++/cpp`、`libgcc.a` | `libc.a`、`libm.a`、`libg.a`、精简版 `libc_nano.a`/`libg_nano.a`、`nano.specs`、`crt0.o`、`libnosys.a` |
| 依赖 | 依赖 `binutils-arm-none-eabi`；**推荐** newlib | 依赖 `libnewlib-dev`（头文件）；**推荐** gcc 与 `libstdc++-arm-none-eabi-newlib` |
| 类比 | 翻译官 | 词典 + 常用语手册 |

- 编译阶段不需要 newlib；**链接阶段**才需要：`printf/memcpy/sin` 的实现、`crt0` 启动代码、`--specs=nano.specs`（该文件由 newlib 包提供，位于 `/usr/lib/arm-none-eabi/newlib/nano.specs`）。
- apt 里 newlib 对 gcc 只是 Recommends，不强制装；**只装 gcc 能编译、一链接就报错**（`undefined reference to memcpy` 或找不到 `nano.specs`）。
- 用 C++ 还要第三个包 `libstdc++-arm-none-eabi-newlib`；纯 C 项目（如 DDM_test）用不到。
- 对比：ARM 官方 tar.xz 与 ST bundle 是**整体打包**，内部同时含编译器与库，不必单独理解这两个 apt 包。

## 五、本机现状盘点与建议

| 组件 | 状态 |
|---|---|
| CubeMX | 已装 6.18.1，位于 `~/STM32CubeMX`，入口 `~/.local/bin/cubemx-gui` |
| CMake | 系统 3.28.3 + ST bundle 4.3.1 |
| Ninja | ST bundle 有 1.13.2，但**不在 PATH**（DDM_test 构建报错的原因） |
| STM32_Programmer_CLI | 已有：`~/.local/share/stm32cube/bundles/programmer/2.23.0/bin/` |
| ST-LINK GDB server / server | 已有（7.14.0+st.2 / 2.1.1+st.8） |
| arm-none-eabi-gcc | **缺**（newlib 同样缺） |

建议：

1. `sudo apt install -y ninja-build gcc-arm-none-eabi libnewlib-arm-none-eabi` 补齐两块拼图（进 PATH，最顺）。
2. 烧录无需额外安装，走 VS Code 扩展的 ST-LINK 调试 + usbipd 直通；终端手工烧录可直接写全路径调用 bundle 里的 STM32_Programmer_CLI。
3. **不必装 CubeCLT**：bundle 管理器已提供同等能力，且与现有工具重叠；将来想把构建/烧录完全脚本化（脱离 VS Code 扩展）再考虑。
4. CubeMX 保持现状，只作为 `.ioc` 配置与代码生成入口；日常改代码不去动生成区文件。

## 六、速查表

| 目的 | 命令 / 位置 |
|---|---|
| 启动 CubeMX | `~/.local/bin/cubemx-gui` |
| 验证 GCC / newlib | `arm-none-eabi-gcc --version`；`ls /usr/lib/arm-none-eabi/newlib/nano.specs` |
| 安装编译工具链 | `sudo apt install -y ninja-build gcc-arm-none-eabi libnewlib-arm-none-eabi` |
| bundle 存放位置 | `~/.local/share/stm32cube/bundles/` |
| 手工烧录入口 | `~/.local/share/stm32cube/bundles/programmer/2.23.0/bin/STM32_Programmer_CLI` |
| bundle 管理器 | VS Code 命令面板 → 「STM32Cube: 打开系统 STM32Cube Bundles 管理器」 |

## 关联

- [[somezhishi/somezhishi|somezhishi]]
- [[somezhishi/编程基础/构建工具链-CMake与make与Ninja详解|构建工具链：CMake、make 与 Ninja]]
- [[somezhishi/编程基础/Linux权限-chmod详解|Linux 权限：chmod 与文件权限基础]]
