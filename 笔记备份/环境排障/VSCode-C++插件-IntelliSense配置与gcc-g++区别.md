---
date: 2026-09-14
tags:
  - 排障
  - 指南
  - 工具
  - wsl
---

# VS Code C/C++ 插件：IntelliSense 配置与 gcc/g++ 区别

> [!note] 背景
> 2026-09-14，在 WSL 远程窗口打开 C/C++ 文件时，C/C++ 插件（`ms-vscode.cpptools`）输出日志：
> - 对于 C 源文件，IntelliSenseMode 已根据编译器参数和查询 compilerPath 从 `windows-gcc-x64` 更改为 `linux-gcc-x64`：`/usr/bin/gcc`
> - 对于 C++ 源文件，同上
>
> 看着像报错，其实是插件的**自动纠正提示**（信息级日志）。本文记录根因、修复方式，以及顺手要搞清的 gcc / g++ 区别。

## 一、根因：设置层级串了

- Windows 侧**用户设置**（`%APPDATA%\Code\User\settings.json`）把默认 IntelliSenseMode 设成了 `windows-gcc-x64`，includePath 指向 Windows 本地的 MinGW。
- VS Code 的「用户设置」在 WSL 远程窗口里**照常生效**。插件一开始按 Windows 模式工作，随后查询了 WSL 里的编译器 `/usr/bin/gcc`，发现模式不匹配，于是自动换成 `linux-gcc-x64` 并打日志。
- 所以这不是故障，真正的问题是：Windows 本地的配置管到了 WSL 窗口。

远程窗口内的设置优先级（高 → 低）：

```
工作区 Workspace  >  远程 Remote（Machine）  >  用户 User
```

修法：**在「远程」层写一份 Linux 配置，覆盖 Windows 用户设置**；Windows 本地那份保持原样，两边互不影响。

## 二、三个概念（是什么、为什么）

### IntelliSense

编辑器里的补全、跳转、参数提示、错误波浪线。**只影响编辑体验，不参与实际编译**——编译由 Makefile / CMake 决定。

### compilerPath 与 compiler query

C/C++ 插件不是猜头文件在哪，而是**运行 compilerPath 指向的编译器**（`-v`、`-E -dM` 等），从输出里拿到系统头文件搜索路径和预定义宏。这解释了为什么填错编译器时，`#include <stdio.h>` 会报找不到。

### intelliSenseMode

告诉插件目标平台 / 编译器家族，例如 `linux-gcc-x64` = Linux + x86_64 + GCC。它决定 `sizeof`、类型宽度、内置宏等 IntelliSense 语义。**必须与 compilerPath 的编译器匹配**——不匹配时插件会自动纠正，就是开头那条日志。

## 三、gcc 与 g++ 的区别

两者是**同一套 GCC 工具链的两个前端入口**（Ubuntu 上 `/usr/bin/gcc` → gcc-13，`/usr/bin/g++` → g++-13），共享后端，差异在默认行为：

| | gcc | g++ |
|---|---|---|
| 语言判定 | 按扩展名（`.c` → C，`.cpp` → C++） | 一律按 C++，连 `.c` 也当 C++ 编译 |
| 链接阶段 | 只链 `libgcc`，**不链 C++ 标准库** | 自动链 `libstdc++` |
| C++ 标准库头路径 | C 模式不包含 | 自动包含 `/usr/include/c++/13` 等 |
| 适合 | 纯 C 项目 | C/C++ 混用；作 IntelliSense 查询编译器 |

> [!warning] 常见坑
> `gcc foo.cpp` 编译能过，链接时报 `undefined reference to std::...`——因为 gcc 不会自动链 `libstdc++`（手动加 `-lstdc++` 可以，但正确做法是改用 g++ 链接）。

安装上两者是**分开的包**：装了 `gcc` 不等于有 `g++`。本机 WSL 之前只有 gcc（`/usr/bin/g++` 不存在），所以日志里 C++ 文件也在查询 gcc；这样 C++ 标准库头路径查不到，`<iostream>` 之类会画红波浪线。补齐：

```bash
sudo apt install g++
```

## 四、修复：写远程（Machine）设置

目标文件（WSL 侧，不存在会自动创建）：

```
~/.vscode-server/data/Machine/settings.json
```

内容：

```json
{
    "C_Cpp.default.intelliSenseMode": "linux-gcc-x64",
    "C_Cpp.default.compilerPath": "/usr/bin/g++"
}
```

**为什么 compilerPath 选 g++**：C 文件查询照常（C 系统头路径同样在 g++ 的查询结果里），C++ 文件又能拿到 `libstdc++` 头路径，一个路径同时覆盖 C / C++，寒假 C++ 线不用再改。

> [!note] 为什么写「远程」层
> 这两个设置的 scope 是 `machine-overridable`，即机器级可覆盖。写在远程层会覆盖 Windows 用户设置，从而日志消失；Windows 本地那份 MinGW 配置不受影响。

### GUI 操作步骤

1. 确认 VS Code **左下角是绿色 `><` + 「WSL: Ubuntu」**（远程窗口）；不是就先 `Ctrl+Shift+P` → `WSL: Reopen Folder in WSL`，或在 WSL 终端里 `code .` 重开
2. 按 `Ctrl+,` 打开设置面板
3. 顶部标签切到 **「远程 [WSL: Ubuntu]」**——关键一步，否则改的是 Windows 本地设置
4. 搜索 `intelliSenseMode`，找到 **C_Cpp > Default: IntelliSense Mode**，下拉选 `linux-gcc-x64`
5. 搜索 `C_Cpp.default.compilerPath`，输入框填 `/usr/bin/g++`
6. 改过的设置项左侧会出现蓝色竖线（已修改标记）

### 或 JSON 直达

`Ctrl+Shift+P` → **`Preferences: Open Remote Settings (JSON)`**（中文：「首选项: 打开远程设置(JSON)」）→ 粘贴上面的 JSON → 保存。

## 五、验证

1. `Ctrl+Shift+P` → `Developer: Reload Window`
2. 打开任意 `.c`（如 `C_test/hello.c`）
3. `Ctrl+Shift+U` 打开输出面板，右上角下拉选 **C/C++**：日志应直接显示 `linux-gcc-x64`，不再有「已更改」条目
4. `#include <stdio.h>` 无红波浪线，F12 能跳进头文件

## 六、注意与延伸

- **别按日志提示把 compilerPath 设为 `""`**——那会禁用系统头检测，补全全废。
- 这是 IntelliSense 配置，**不改变实际构建**：Makefile / CMake 里 C 用 `gcc`、C++ 用 `g++`（或 CMake 自动选择）。
- 单个项目也可用 `.vscode/c_cpp_properties.json` 覆盖（工作区级优先级最高），`C_test` 已有一份值为 `linux-gcc-x64` 的配置；但远程级配置一次到位，适合当默认。
- STM32 工程（`DDM_test`、未来 `gimbal2`）的编译器是 **arm-none-eabi-gcc**，应在各自项目配置里单独指定，不跟这份默认混用。

---

关联：[[somezhishi/somezhishi|somezhishi 索引]]
