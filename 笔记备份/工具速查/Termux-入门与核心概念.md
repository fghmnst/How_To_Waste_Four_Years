---
date: 2026-10-09
tags: [入门, 工具, linux]
---

# Termux-入门与核心概念

> 用途：搞清 Termux 是什么、不是什么，以及几个必须先懂的概念（Bionic/glibc、仓库与签名、pkg 与 apt）。
> 配套操作清单见 [[somezhishi/工具速查/Termux-平板环境搭建清单|Termux 平板环境搭建清单]]。

## 一、Termux 是什么

**Termux 是一个安卓 App**，官方定义：An Android terminal emulator and Linux environment app（安卓终端模拟器 + Linux 环境 App）。

它给平板/手机装了一整套"类 Linux 用户态"：

- 一个终端界面（你看到的黑窗口）
- 一整套命令（bash、ls、gcc、python……）
- 自己的包管理器（`pkg`）和软件仓库

三层结构：

| 层 | 内容 | 说明 |
|---|---|---|
| 内核层 | Linux 内核 | 安卓系统本身就在跑，开机就有 |
| 用户态（宿主） | Android 系统 | 提供 App 运行环境、权限沙盒 |
| 用户态（客人） | Termux + 它的软件 | 装在 App 私有目录里的一套"独立工具间" |

> [!note] 一句话
> Termux 不提供内核，是"寄生"在 Android 内核之上的用户态环境；所有文件都在 App 私有目录里（无 root）。

## 二、它不是发行版（与 Ubuntu/Arch 的区别）

"发行版"三件套：自带内核、自己的 init（PID 1 开机服务）、完整用户态 + 包体系，能独立启动。

| | Ubuntu / Arch | Termux |
|---|---|---|
| 内核 | 自带，可独立开机 | 没有，借 Android 的 |
| init（PID 1） | systemd / openrc | 没有，PID 1 是 Android 的 |
| 完整 OS | 是 | 否，只是个 App |
| 包管理器 + 仓库 | 有 | 有（apt + Termux 源）→ 这点像 |
| libc | glibc | Bionic（安卓自研） |
| 目录 | 标准 FHS（/usr /etc） | 单一前缀 `$PREFIX` |
| 安装方式 | 刷机/装机 | 装 APK |

类比：Ubuntu/Arch 是"一整栋带地基的独立房子"；Termux 是"租在 Android 公寓里的标准化工位"——家具（apt、仓库、shell）齐全，但没有自己的地基和入户门。Windows 上的同类是 MSYS2/Cygwin。

**对学习的影响**：

- CLI 层面（shell、文件、权限、管道、编译、包管理）与发行版体验一致，可以放心学；
- 系统层面（开机流程、systemd、内核、真多用户、挂载）没有对应物——以后用 proot Ubuntu 或 WSL/虚拟机补。

## 三、Bionic vs glibc：预编译二进制的"大坑"

**动态链接**的程序不把 C 标准库打进自己身体，而是运行时"现找"两样东西：

1. 启动器（动态链接器），路径写死；
2. 一堆 `.so` 共享库。

Ubuntu 上编译的程序写的是：启动器 `/lib/ld-linux-aarch64.so.1`（glibc 的）；Android 上根本没有这个文件，Termux 程序用的是 `/system/bin/linker64`（Bionic 的）。

所以 GitHub 上下的 linux-arm64 预编译二进制会报：
`cannot execute: required file not found`（新版 bash）或 `No such file or directory`（旧 shell）——**文件明明就在，找不到的是它要求的"启动器"**。

> [!warning] 记住这条报错
> 看到 "required file not found" 但 `ls` 明明有文件 → 八成是 glibc 动态链接的程序进了 Bionic 的地盘。

**30 秒判断法**（装了 `file` + `binutils` 后）：

```bash
file ./下载的程序
```

- 含 `interpreter /lib/ld-linux-aarch64.so.1` → 跑不了（glibc 动态）
- 含 `statically linked` / `static-pie` → 能跑（静态，不依赖任何 libc 路径）
- 含 `interpreter /system/bin/linker64` → 能跑（Bionic）
- 顺便：`uname -m` 看本机架构（应为 aarch64）

四条出路：

1. `pkg install`（官方源里的都是 Bionic 版）——首选
2. 源码编译（Termux 里装 clang/make，编出来就是 Bionic）
3. 找静态产物（纯 Go 默认静态；btop 官方 musl 静态包也属此类）
4. 进 proot Ubuntu 容器里跑（容器里有整套 glibc）

## 四、仓库与签名：搞清"下载源"

三层概念：

| 层级 | 说明 | 签名 |
|---|---|---|
| 镜像站（tuna/sjtu/…） | 同一官方仓库的字节级副本，只换 URL | 与官方一致 |
| 官方仓库族 | main / x11 / root | 官方 GPG key（termux-keyring） |
| TUR（第三方） | 官方不收的包、旧版本、预编译 Python 包 | 独立 GPG key |

规模实测（aarch64，2026-10 数据）：

| 仓库 | 包数 | 定位 |
|---|---|---|
| main | 3004 | 主体，最全 |
| x11 | 1180 | GUI/X11 相关 |
| root | 99 | 需 root 的设备（无需装） |
| TUR | 活跃补充 | `pkg install tur-repo` 后可用 |

- 换镜像不影响任何东西（内容与签名完全一样，只是"快递点"不同）。
- TUR 是第三方源，装它 = 主动信任第三方。
- 另一个"签名"概念：App 安装渠道（F-Droid/GitHub）的 APK 签名不同，Termux 主 App 与插件（Styling/API）必须**同渠道**安装，混装会报 `INSTALL_FAILED_UPDATE_INCOMPATIBLE`。

## 五、pkg 与 apt

`pkg` 是官方包装脚本，`apt` 是底层真身；两者操作同一套数据库和同一份源配置。

| 你敲的 | 实际执行 |
|---|---|
| `pkg update` | 镜像体检/换源 + `apt update` |
| `pkg upgrade` | 镜像体检 + `apt update`（条件性）+ `apt full-upgrade` |
| `pkg install/search` | 镜像体检 + 缓存判断 + `apt install/search` |
| `pkg files` | `dpkg -L` |
| `apt list --upgradable` | 只能直接用 apt（pkg 没封装） |

pkg 的特调：

- **镜像体检**：缓存过期就全测一遍镜像并自动选（所以第一次 `pkg update` 会刷屏测试各镜像）
- **拒绝 root**：脚本开头就检查，root 下直接报错——Termux 永远不用 sudo
- 前缀简写：`pkg i`、`pkg up`、`pkg se` 都行

用法建议：日常装/升/搜走 `pkg`；查询类进阶操作（`apt list --upgradable`、`apt policy`、`apt purge`）直接用 `apt`。

## 术语小词典

| 术语 | 大白话 |
|---|---|
| Termux | 安卓上的终端模拟器 + Linux 环境 App |
| `$PREFIX` | Termux 的"系统目录"（/data/data/com.termux/files/usr） |
| `$HOME` | Termux 的家目录，`~` 就是它（.../files/home） |
| 用户态 | 内核之外的所有程序和环境 |
| libc | C 标准库，程序运行的地基；Linux 用 glibc，安卓用 Bionic |
| 动态链接 | 程序运行时去系统里找"启动器"和共享库 |
| 静态链接 | 需要的库全部打进程序体内，不依赖系统路径 |
| 镜像站 | 同一份软件内容的分布式副本（快递点） |
| GPG 签名 | 用密码学证明"包来自官方、没被篡改" |
| pkg / apt | Termux 的包管理入口 / 底层真实执行者 |

> [!question]- 自测
> 1. Termux 是发行版吗？为什么？
> 2. 为什么 GitHub 下载的 linux-arm64 程序会报 "required file not found"？
> 3. 换镜像站需要重新信任签名吗？
> 4. `pkg install` 和 `apt install` 有什么区别？
>
> 答案：1. 不是。无自带内核、无 init、非完整操作系统，是寄生在 Android 上的用户态 App。2. 它是 glibc 动态链接，写死的启动器路径 `/lib/ld-linux-...` 在 Android 不存在；找不到的是"启动器"而不是文件本身。3. 不需要，镜像站是官方仓库的字节级副本，签名一致。4. 最终都调用 apt，但 pkg 会先做镜像体检、缓存判断等 Termux 特调。
