---
date: 2026-11-04
tags: [入门, 工具, linux, 项目]
---

# DSHGo-本机Linux与PRoot

> 用途：搞清 DSH Go 在安卓手机上提供的 Linux 环境到底是什么，和 Termux 有什么本质区别。
> 配套：Termux 基础见 [[somezhishi/工具速查/Termux-入门与核心概念|Termux 入门与核心概念]]；远程连接见 [[somezhishi/工具速查/DSHGo-远程连接机制|DSH Go 远程连接机制]]。
> 结论先行：**它不是虚拟机，而是 PRoot 系统调用翻译层 + Ubuntu glibc 用户态。**

## 一、一句话结论

| 问题 | 答案 |
|---|---|
| 是虚拟机吗 | 不是。没有 guest kernel、没有 QEMU、没有 KVM |
| 用 Android 的 Linux 内核吗 | 用。系统调用最终都进入同一个 Android 内核 |
| 像 Termux 那样原生吗 | 默认不是。Termux 是 bionic 原生；DSH Go 是 glibc + PRoot 拦截 |
| CPU 性能 | 接近原生（不模拟指令） |
| 文件 I/O / 启动性能 | 有明显开销（每次路径类系统调用多绕一圈） |
| 有真 root 吗 | 没有。`id` 显示 root 是假 root，真实进程仍是 App 的 Android UID |
| 能跑 Docker/systemd/iptables 吗 | 不能。没有 namespace/cgroup/内核模块 |
| 适合做什么 | 零配置跑 DSH；不适合替代通用 Linux 工具箱 |

## 二、本机启动链路

DSH Go 的本机引擎不是“装了一个 Ubuntu 虚拟机”，而是把一套 Ubuntu 根文件系统解压到 App 私有目录，再用 PRoot 启动里面的 Node/DSH：

```text
Flutter UI
  → Kotlin DshProcess
    → libproot.so（ptrace/seccomp 拦截系统调用）
      → Ubuntu 24.04 rootfs 里的 /opt/node/bin/node
        → dsh web（只监听 127.0.0.1）
```

关键事实（来自仓库源码）：

- PRoot 版本是 **5.1.107.98**，直接取自 Termux 的 `proot` deb 包，复制成 `libproot.so` 放进 APK。
- 启动参数包含 `--kill-on-exit --link2symlink --sysvipc -L -0`：
  - `-0`：假 root；
  - `--link2symlink`：模拟硬链接；
  - `--sysvipc`：模拟 System V IPC。
- rootfs 是 Ubuntu 24.04.5 + Node 24 + DSH + pnpm；APK 里压缩后约 151MB，装完建议预留约 1GB。
- `targetSdk = 28`（和 Termux 一样）：绕开 Android 10+ 对 App 私有目录 `exec` 的限制，让 rootfs 里的二进制能执行。这是明确的兼容性取舍，不适合直接上 Google Play。

## 三、PRoot 是什么 / 不是什么

### 1. 它做的事

- 程序执行 `open("/etc/passwd")` 时，PRoot 把路径改写成 `<rootfs>/etc/passwd`；
- `chroot`、`bind mount`、部分 IPC 由 PRoot 在用户态“模拟”；
- 通过 `ptrace`（以及 seccomp 加速）拦截系统调用，修改参数/返回值。

### 2. 它不做的事

- 不启动 guest 内核；
- 不模拟 CPU 指令；
- 不创建 namespace、不用 cgroup；
- 不是 Docker 那种容器隔离；
- **不是安全沙箱**。它只解决路径/root 兼容问题，里面的程序仍然能访问 App 能访问的东西。

> [!warning] 常见误解
> “PRoot 里 `id` 是 root”不等于“拿到了 root 权限”。真实进程仍是 App 的 UID，受 Android SELinux 和沙箱约束；不能挂载内核模块、不能改内核参数、不能绕过 Android 权限。

## 四、性能到底差多少

仓库自己的实测（x86_64 模拟器，同一份 rootfs、同一个 DSH 主目录）：

| 跑法 | warm 启动 | cold 启动 |
|---|---:|---:|
| PRoot | 1.93s | 2.27s |
| 绕开 PRoot，用 rootfs 的 `ld.so` 直接跑同一个 Node | 1.07s | 1.35s |
| 差异 | **-44%** | **-41%** |

原因：DSH 启动要发起约 **28,100 次路径类系统调用**（`statx`、`openat`、`readlink`、`access`），PRoot 下每次约 30µs，累计就是秒级。

项目做的优化：

- **启动期路径缓存**：把 `realpathSync`、`existsSync`、`package.json` 读取缓存起来，启动段 warm 快 14%、cold 快 12%；
- **打开 App 即拉起引擎**：与 Flutter 初始化并行，端到端从 3.02s 降到 2.46s（-18%）；
- **可选 Direct 模式**：主引擎绕过 PRoot，启动段再快约 35%–44%。

> [!note] 性能结论
> CPU 密集型任务几乎不亏；**文件 I/O、进程启动、大量小文件操作**亏得最明显。npm install、编译、git 操作会比 Termux 原生慢。

## 五、Direct 模式：更接近原生，但默认关闭

Direct 模式不启动 PRoot，而是用 rootfs 里的 glibc 动态链接器直接加载 Node：

```text
<rootfs>/usr/lib/aarch64-linux-gnu/ld-linux-aarch64.so.1
  --library-path <rootfs>/usr/lib/...:<rootfs>/lib/...
  <rootfs>/opt/node/bin/node
  --import=<rootfs>/opt/dsh-go/direct/preload.mjs
  <rootfs>/opt/dsh/node_modules/@deepseek-ai/dsh/lib/bin.js web ...
```

它解决的问题：

- 主引擎启动不再经过 ptrace；
- 路径、DNS、模块解析由 `preload.mjs` 等适配层处理。

它的限制：

- **默认关闭**，要在“开发者选项 → 直接启动本机引擎”里手动打开；
- 部分机型（Android 16 / x86_64）的 seccomp 会拦截 glibc loader，启动失败后自动回退 PRoot；
- 只让主引擎绕过 PRoot，agent 的 bash/git/python/apt 等工具子进程仍可能回到 PRoot 兼容路径；
- 不是“零成本”，只是把最重的启动路径从 ptrace 里拿出来了。

## 六、和 Termux 对比

先明确：**Termux 也不是“另一个内核”**。它是 Android 上的终端 App，用户态是 Android 的 bionic libc，直接跑在同一个 Linux 内核上；它不需要 PRoot，所以没有 ptrace 开销。要在 Termux 里跑 Ubuntu/glibc，通常用 `proot-distro`，那时才引入 PRoot，和 DSH Go 是同一类机制。

| 维度 | Termux 原生 | Termux + proot-distro | DSH Go 本机 |
|---|---|---|---|
| 内核 | Android Linux | Android Linux | Android Linux |
| 用户态 | bionic | glibc（Ubuntu/Debian） | glibc（Ubuntu 24.04） |
| 隔离/翻译 | 无，原生系统调用 | PRoot ptrace 翻译 | PRoot ptrace 翻译（可选 Direct） |
| root | 无 | 假 root | 假 root |
| 性能 | 最接近原生 | 系统调用密集场景明显变慢 | 同左，启动有缓存优化 |
| 安装 | 装 Termux + `pkg` | 装 Termux + `proot-distro` | 装 APK 即自带 |
| 体积 | 较小 | 中（再加 rootfs） | 大（APK 约 196MB，装完约 1GB） |
| 通用性 | 强，完整终端/包管理/SSH/X11 | 强，可换发行版 | 弱，围绕 DSH 定制 |
| 终端 | 完整终端 | 完整终端 | DSH 工作区终端（本机/SSH 可用） |
| Android API | `termux-api`（相机、传感器、短信等） | 同左 | 手机操控（无障碍/Shizuku/ADB） |
| 后台保活 | 需自行配置 | 需自行配置 | 前台服务 + 保活引导 |
| 内核模块/iptables/Docker/systemd | 否 | 否 | 否 |

## 七、优缺点与适用场景

### 优点

- 零配置：不用装 Termux、不用 `proot-distro install ubuntu`、不用自己装 Node/DSH。
- 和 DSH App 深度集成：会话、插件、模型配置、手机操控、前台服务、日志、崩溃恢复、安全模式、运行时校验/回滚都在一个 App 里。
- 针对 DSH 启动做了路径缓存、预启动、可选 Direct 模式，实际体验比“裸 proot-distro”更顺。
- 对 glibc 生态兼容更好：Node、Python、JVM、LibreOffice WASM、原生扩展都能直接跑，不用为 bionic 重新编译。
- 运行环境有版本、SHA-256、回滚和安全模式管理，升级由 App 负责。

### 缺点

- **更重**：APK 约 196MB（1.0.8 实测，其中 151MB 是 rootfs），装完约 1GB；Termux 原生轻得多。
- **I/O 性能不如 Termux 原生**：PRoot 的 ptrace 开销在 npm install、大量小文件、编译、git 操作上很明显。
- **不是通用 Linux 环境**：没有 Termux 的包生态、没有 X11/VNC、没有默认 sshd、没有 `termux-api`；终端是 DSH 的工作区终端。
- **控制权少**：不能换发行版、不能真正 root、不能 systemd/Docker/iptables/mount、不能加载内核模块；假 root 下 setuid 等语义也不完整。
- **沙箱能力受限**：PRoot 下 DSH 的 `workspace-write` 沙箱（bwrap/Landlock）可能不可用；它不能当安全隔离用。
- **兼容性取舍**：`targetSdk 28`、仅侧载、Android 8+、仅 arm64/x86_64；Android 16 等新系统上 Direct 模式还可能被 seccomp 拦截。

### 怎么选

- 只想在手机上零配置跑 DSH：DSH Go 本机引擎。
- 想要通用 Linux / 编译 / 终端 / 更好性能：Termux 原生；需要 Ubuntu 时用 Termux + proot-distro。
- 想要接近原生的 Linux 性能：rooted chroot 或 Android AVF（真实 Linux VM），但不是免 root 方案。

## 术语小词典

| 术语 | 大白话 |
|---|---|
| PRoot | 用户态的“假 chroot”：用 ptrace 拦截系统调用、改写路径，不需要 root |
| ptrace | Linux 的进程跟踪接口；PRoot 靠它“偷看/修改”系统调用 |
| seccomp | Linux 的系统调用过滤机制；PRoot 用它加速，只拦需要的调用 |
| Bionic | Android 自己的 C 标准库；Termux 原生程序用它 |
| glibc | 标准 Linux 的 C 标准库；Ubuntu 程序用它 |
| rootfs | 根文件系统；这里指解压到 App 私有目录的 Ubuntu 用户态 |
| 假 root | PRoot 把 guest 的 uid 映射成 0，但真实进程仍是 App UID |
| targetSdk | App 声明的目标 Android 版本；28 可绕开新版对私有目录 exec 的限制 |
| proot-distro | Termux 里管理 PRoot 容器的工具；和 DSH Go 用的是同一类机制 |

> [!question]- 自测
> 1. PRoot 是虚拟机吗？为什么？
> 2. 为什么 DSH Go 启动比 Termux 原生慢？
> 3. Direct 模式为什么默认关闭？
> 4. PRoot 下 `id` 显示 root，是否真的拥有 root 权限？
>
> 答案：1. 不是。没有 guest 内核/指令模拟，系统调用仍进入 Android 内核，PRoot 只做路径翻译和部分模拟。2. 因为 DSH 启动要发起约 2.8 万次路径类系统调用，每次都被 ptrace 拦下来；I/O 和进程启动是主要开销。3. 部分机型（Android 16 / x86_64）的 seccomp 会杀掉 glibc loader；默认关闭并失败回退 PRoot 更稳。4. 没有。只是 guest 里的 uid 显示为 0，真实进程仍是 App 的 Android UID，受 SELinux 和沙箱约束。

## 关联

[[somezhishi/somezhishi|somezhishi]] · [[somezhishi/工具速查/Termux-入门与核心概念|Termux 入门与核心概念]] · [[somezhishi/工具速查/Termux-平板环境搭建清单|Termux 平板环境搭建清单]] · [[somezhishi/工具速查/DSHGo-远程连接机制|DSH Go 远程连接机制]] · [[somezhishi/工具速查/远程访问-内网穿透与虚拟组网|远程访问：内网穿透与虚拟组网]]
