---
date: 2026-10-09
tags: [指南, 工具, linux, git, ssh]
---

# Termux-平板环境搭建清单

> 用途：华为平板 Termux 从零到可用的完整命令清单，按顺序执行即可。
> 约定：命令默认在**平板 Termux** 执行；标「WSL」的在电脑上执行；server 相关值一律用占位符（写位置不写值）。
> 配套概念笔记：[[somezhishi/工具速查/Termux-入门与核心概念|Termux 入门与核心概念]]

> [!info] 当前进度（2026-10-10）
> - **AI 助手**：opencode 2.0.22 ✅ / CCB 2.8.4 ✅（均已配置 provider、可正常对话；安装与注意见下文专章）
> - **中文输入**：✅ 已解决（2026-10-10，三件套配方见「中文输入」小节）
> - **btop**：压缩包已下载，**待解压安装**；在压缩包所在目录依次执行：
>   1. `tar -xzf btop-aarch64-unknown-linux-musl.tar.gz`
>   2. `mv btop/bin/btop "$PREFIX/bin/"`
>   3. `mkdir -p "$PREFIX/share/btop" && cp -r btop/themes "$PREFIX/share/btop/"`
>   4. `checkbin "$PREFIX/bin/btop"` 验证后再跑 `btop`
> - **阶段 0/A 其余项**（SSH 密钥、`~/.ssh/config`、克隆仓库、vimrc/zsh/护栏/checkbin 验收）：**未开始**，后续按清单执行

## 阶段 0 · 手动准备

**0.1 更新与基础安装**（镜像已自动选好；想换：`termux-change-repo`）

```bash
pkg upgrade -y
termux-setup-storage
pkg install -y git openssh vim curl wget file binutils
```

- `pkg upgrade -y`：升级全部已装包；`-y` 自动确认所有询问（中途若问是否替换配置文件，直接回车）。
- `termux-setup-storage`：申请安卓存储权限，生成 `~/storage/` 软链接（指向 Download 等共享目录）。
- 基础包：git / ssh 客户端 / vim / 两个下载工具 / `file` 类型识别 / `binutils`（提供 readelf）。

验证：`git --version && ssh -V && vim --version | sed -n '1p'`

**0.2 SSH 密钥（平板专属）**

```bash
ssh-keygen -t ed25519 -C tablet
cat ~/.ssh/id_ed25519.pub
```

- 两次回车（不设口令：密钥在 App 沙盒内受保护；设了口令每次使用都要输）。
- 把输出的整行公钥分别加入：
  - **GitHub 网页**：Settings → SSH and GPG keys → New SSH key → 粘贴
  - **server2（WSL 执行，`<整行>` 换成公钥内容）**：

```bash
ssh server2 'echo "<整行>" >> ~/.ssh/authorized_keys'
```

**0.3 写 ~/.ssh/config**（HostName/User 从 WSL 的 `~/.ssh/config` 抄；不写进任何仓库文件）

```bash
mkdir -p ~/.ssh && chmod 700 ~/.ssh
vim ~/.ssh/config
chmod 600 ~/.ssh/config
```

文件内容：

```text
Host server2
    HostName <从 WSL config 抄>
    User <同上>
    IdentityFile ~/.ssh/id_ed25519
    ServerAliveInterval 30
```

**0.4 连通性测试**

```bash
ssh -T git@github.com     # 期望：Hi ... You've successfully authenticated
ssh server2 'echo ok'     # 期望：ok
```

首次连接问 fingerprint 输 `yes`。GitHub 超时多为网络问题，重试或换网络；Plan B：HTTPS + PAT。

**0.5 git 配置与克隆**

```bash
git config --global user.name "fghmnst"
git config --global user.email "<与 WSL 一致>"
git config --global core.editor vim
git clone git@github.com:fghmnst/projects.git ~/projects
ls ~/projects
```

- `user.name`/`user.email` 是提交记录里的署名；`core.editor` 指定 git 调用哪个编辑器。
- 注意：不要碰 clone 里的 `obsidian/.obsidian/plugins/remotely-save/`。

## 阶段 A · 护栏与工具

**A.1 工具清单**

```bash
pkg install -y htop ncdu duf dust fastfetch screenfetch mandoc less which tree jq ripgrep zsh zsh-completions
```

**A.2 vim 基础配置**

```bash
cat > ~/.vimrc <<'EOF'
syntax on
set number
set tabstop=4 shiftwidth=4 expandtab
set incsearch hlsearch
set encoding=utf-8
set mouse=a
EOF
```

**A.3 zsh + 默认 shell**

```bash
cat > ~/.zshrc <<'EOF'
autoload -Uz compinit && compinit
setopt hist_ignore_all_dups
PROMPT='%F{green}%n%f:%F{blue}%~%f$ '

# 安装 starship 后取消下一行注释：
# eval "$(starship init zsh)"
EOF
chsh -s zsh
```

验证：重开会话后 `echo $SHELL` 应为 `.../zsh`。

**A.4 防误触护栏**

```bash
cat > ~/.termux-guard.sh <<'EOF'
alias rm='rm -I'
alias cp='cp -i'
alias mv='mv -i'

mkdir -p "$HOME/.trash"

trash() {
    t="$HOME/.trash/$(date +%s)-$$"
    mkdir -p "$t" && mv "$@" "$t/" && echo "已入垃圾桶: $t"
}

danger() {
    printf '即将执行:'; printf ' %s' "$@"; printf '\n确认请输入 YES: '
    read -r ans
    if [ "$ans" = "YES" ]; then
        printf '%s %s\n' "$(date -Iseconds)" "$*" >> "$HOME/.danger.log"
        command "$@"
    else
        echo 已取消; return 1
    fi
}
EOF
grep -q termux-guard ~/.bashrc || echo '[ -f ~/.termux-guard.sh ] && . ~/.termux-guard.sh' >> ~/.bashrc
grep -q termux-guard ~/.zshrc  || echo '[ -f ~/.termux-guard.sh ] && . ~/.termux-guard.sh' >> ~/.zshrc
```

设计要点：

- `rm -I`：删除超过 3 个文件或递归时才询问；日常小删不打扰。
- `trash`：删除变"移入 `~/.trash/时间戳-PID`"，后悔可捞。
- `danger`：伪 sudo，高危命令统一走它，输 YES 才执行并记 `~/.danger.log`。
- 两个 rc 文件都挂载：切 zsh 后护栏不丢。

**A.5 checkbin（二进制体检器）**

```bash
cat > "$PREFIX/bin/checkbin" <<'EOF'
#!/data/data/com.termux/files/usr/bin/bash
host="$(uname -m)"
command -v file >/dev/null || { echo "缺 file：pkg install file binutils"; exit 1; }
for f in "$@"; do
  [ -e "$f" ] || { echo "$f: 不存在"; continue; }
  info="$(file -b "$f" 2>/dev/null)"
  case "$info" in
    *script*|*text*executable*)
      echo "$f: [脚本] shebang: $(sed -n '1p' "$f")"; continue;;
  esac
  [[ "$info" == *ELF* ]] || { echo "$f: [跳过] 非 ELF/脚本"; continue; }
  case "$info" in
    *aarch64*) arch=aarch64;; *"ARM"*) arch=arm32;; *x86-64*) arch=x86_64;; *80386*) arch=i686;; *) arch=未知;;
  esac
  [ "$arch" = "$host" ] || { echo "$f: [跑不了] 架构 $arch != $host"; continue; }
  if [[ "$info" == *statically\ linked* || "$info" == *static-pie* ]]; then
    echo "$f: [能跑] 静态链接（$arch）"; continue
  fi
  interp="$(readelf -l "$f" 2>/dev/null | awk -F': ' '/interpreter/{print $NF}' | tr -d ']')"
  case "$interp" in
    *linker64*) echo "$f: [能跑] Android Bionic";;
    *ld-linux*) echo "$f: [跑不了] glibc → 找静态版或进 proot";;
    *ld-musl*)  echo "$f: [跑不了] musl → 找静态版";;
    *)          echo "$f: [存疑] interpreter=${interp:-未读到}";;
  esac
done
EOF
chmod +x "$PREFIX/bin/checkbin"
```

用法：`checkbin 文件1 文件2`；原理见 [[somezhishi/工具速查/Termux-入门与核心概念|概念笔记]]第三节。

**A.6 总验收**

```bash
fastfetch
htop                      # q 退出
type rm                   # 应显示 alias rm='rm -I'
danger echo hi            # 输 YES 后输出 hi
checkbin "$PREFIX/bin/ls" # [能跑] Android Bionic
ssh server2 'echo ok'
ssh -T git@github.com
echo $SHELL               # 重开会话后应为 zsh
```

## 静态二进制安装范式（btop 案例）

btop 在 Termux 官方源和 TUR 都**没有**；官方 release 提供的是 **musl 静态包**（对 glibc 大坑免疫）——这是"GitHub 预编译产物"的正确打开方式：

```bash
mkdir -p ~/downloads && cd ~/downloads
curl -LO --retry 3 --retry-delay 2 https://github.com/aristocratos/btop/releases/download/v1.4.7/btop-aarch64-unknown-linux-musl.tar.gz
tar -xzf btop-aarch64-unknown-linux-musl.tar.gz
mv btop/bin/btop "$PREFIX/bin/"
mkdir -p "$PREFIX/share/btop"
cp -r btop/themes "$PREFIX/share/btop/"
checkbin "$PREFIX/bin/btop"   # [能跑] 静态链接
btop
```

要点：

- 资产名要对：`aarch64` + `musl`；实测该二进制为 `statically linked`。
- 主题放 `$PREFIX/share/btop/themes`（btop 按二进制相邻的 `../share/btop/themes` 找）；配置自动写到 `~/.config/btop/`。
- 升级 = 换 URL 里的版本号重下覆盖；不归 pkg 管理。
- 包里的 `install.sh` 是给标准 Linux（/usr/local）用的，别跑。

### 解压命令对照表

| 后缀 | 是什么 | 解压命令 |
|---|---|---|
| `.tar` | 只打包，不压缩 | `tar -xf` |
| `.gz` | 只压缩（单文件） | `gunzip`（删原文件）/ `gzip -dk`（保留原文件）/ `zcat`（只看） |
| `.tar.gz` / `.tgz` | 打包 + 压缩 | `tar -xzf` |
| `.zip` | 打包 + 压缩一体 | `unzip`（需 `pkg install unzip`） |
| `.xz` / `.bz2` | 另一种压缩 | `xz -d` / `bzip2 -d`；tar 系用 `tar -xJf` / `tar -xjf` |

安全习惯：先列表（`tar -tzf` / `unzip -l`）→ 建专用目录 → 解压进去（tar 若自带顶层目录则无散落风险）。

## AI 编程助手：opencode 与 CCB（2026-10-09 实装）

> 两者均已装在这块平板上、provider 已配置可对话；命令名不冲突（`opencode` / `ccb`），可共存。

### opencode（Hope2333 v2 原生版，当前 2.0.22）

**为什么手动装 deb**（官方三条路在 Termux 全不可用，实测）：

- 官方 curl 脚本：下载的是 glibc 动态二进制 → Bionic 报 `required file not found`；
- 官方 npm 包 `opencode-ai`：平台白名单没有 android，且装到的还是同一个 glibc 二进制；
- 官方安装脚本的 apt 源：索引钉在旧 tag 且引用的资产已 404；pacman 源可用但需先切实验性 pacman，不值得。

```bash
# 0) 清理官方脚本残留（防 PATH 里旧命令抢先）
rm -rf ~/.opencode
grep -n 'opencode' ~/.bashrc ~/.zshrc    # 有 ~/.opencode/bin 的 PATH 行则手动删

# 1) 前置
pkg install -y ripgrep
getprop ro.build.version.sdk             # 需 >= 28

# 2) 下载 + 校验
mkdir -p ~/downloads && cd ~/downloads
curl -LO --retry 3 https://github.com/Hope2333/opencode-termux/releases/download/Push261005/opencode_2.0.22_aarch64.deb
# 注意：官方 SHA256SUMS.txt 只覆盖 *-upx.xz 资产、不含 deb（实测），所以用结构自检 + 记录本地哈希：
sha256sum opencode_2.0.22_aarch64.deb
dpkg-deb --info opencode_2.0.22_aarch64.deb | sed -n '1,12p'

# 3) 安装 + 验证
dpkg -i opencode_2.0.22_aarch64.deb      # 报缺依赖才执行：apt install -f -y
command -v opencode && opencode --version
checkbin "$(command -v opencode)"        # [能跑] Android Bionic
```

| 参数 | 值 |
|---|---|
| 下载 / 安装体积 | 68.6 MB / ≈276 MB |
| 依赖 | 无（`Depends:` 为空）；仅要求 Android API ≥ 28 |
| 互斥 | `Conflicts: opencode-wrapper`（同代 native/wrapper/compressed 三选一） |
| 升级 | 手动：新 tag 出新 deb 后重下覆盖 `dpkg -i`（apt 下 `pkg upgrade` 不适用） |

### CCB（claude-code-best 2.8.4）

纯 JS bundle，跑在 Termux 原生 Node 上，无需重编译；npm 安装最顺。

```bash
# 1) 前置
pkg install -y nodejs npm ripgrep

# 2) 安装 + 验证
npm i -g claude-code-best
ccb --version
ccb        # 首次进 /login → 选第三方兼容 API（Anthropic Compatible / OpenAI 兼容），填 Base URL + Key
```

Termux 注意（来自其手册与 issue #1272 / #1273）：

- 临时目录保险（旧版 `/tmp` 无写权限，新版已修；加上无副作用）：
  ```bash
  mkdir -p ~/.cache/ccb-tmp
  echo 'export CLAUDE_CODE_TMPDIR="$HOME/.cache/ccb-tmp"' >> ~/.zshrc
  ```
- postinstall 失败兜底：`npm i -g claude-code-best --ignore-scripts`（bin 仍会链接，补 `pkg install ripgrep` 即可）；
- Node 26 异常兜底：`pkg uninstall nodejs && pkg install nodejs-lts`；
- **不要用 Anthropic 官方账号登录**（该重建版关闭了官方封控点，用官方账号有封号/合规风险），配第三方兼容 API；
- WebSearch / WebFetch 取决于所配 API 是否自带搜索能力（已知 DeepSeek 后端不支持）。

风险声明：该仓库**无 license**、非官方重建，来源信任自担；与 opencode（开源项目 + 社区打包）不是同一风险等级。

### 中文输入（2026-10-10 已解决）

**最终配方（三件套，Termux 内直接输中文，零新增软件）**：

1. **`enforce-char-based-input = true`**——把输入类型从 TYPE_NULL 切到"字符基础"模式，输入法才会走 commitText 上屏路径：

   ```bash
   grep -n 'enforce-char-based-input' ~/.termux/termux.properties
   # 没有该行则追加：
   echo 'enforce-char-based-input = true' >> ~/.termux/termux.properties
   termux-reload-settings
   ```

2. **Termux 设置 → Keyboard → Soft Keyboard Enabled 确认为开**（若接外接键盘，注意同分类下 "Soft Keyboard Only If No Hardware" 的行为）；

3. **在底部扩展键条上向左滑动至空白**（社区经典 trick）。

验证：`echo 中文测试`——打拼音 → 输入法候选条选字 → 中文正常上屏。实测环境：华为平板自带输入法。

**原理一句话**：默认 TYPE_NULL 时输入法只用按键通道（中文不组词）；char-based 模式下 IME 调用 `commitText()`，而 Termux 的 `BaseInputConnection` 恰好实现了 `commitText → sendTextToTerminal`（源码实锤）。副作用：拼音组合过程在终端里不可见（候选在输入法自己的候选条上），选字后正常上屏（上游 issue #5242 在做组合预览）。

**保底方案**（终端 App / 网页输入失败时）：

```bash
pkg install termux-api jq
# 另需 F-Droid 安装 Termux:API 应用（必须与主 App 同渠道签名）
opencode run "$(termux-dialog text | jq -r .text)"        # 弹原生输入框 → 一次性问答
opencode --prompt "$(termux-dialog text | jq -r .text)"   # 带提示词进 TUI
ccb -p "$(termux-dialog text | jq -r .text)"              # CCB（以其 --help 为准）

# 万能函数（加进 ~/.zshrc）：弹框 → 写剪贴板 → 去 TUI 或 App 里粘贴
cn() { termux-dialog text | jq -r .text | termux-clipboard-set; echo "已复制到剪贴板"; }
ask() { opencode run "$(termux-clipboard-get)"; }
```

**弯路档案（已排除，别重试）**：

| 路线 | 结论 |
|---|---|
| ttyd 网页终端（浏览器输入） | 华为平板浏览器内无法输入（浏览器输入层问题） |
| opencode web UI | 实测存在问题，不采用 |
| TermAll（本机 SSH 客户端） | 密钥导入有 bug；坚持密钥登录则不可用 |
| YukiSSH | 仅支持密码登录，不符密钥要求（其"透明 EditText"输入方案可参考） |
| 日后仍要"SSH 客户端 + 密钥" | 候选：ConnectBot（开源、支持密钥导入）/ Termius |

### 选型对照

| 维度 | opencode | CCB |
|---|---|---|
| 许可/来源 | 开源（MIT）+ 社区 bionic 打包 | Anthropic 专有软件的非官方重建，无 license |
| 安装 | 手动 deb | npm 即装 |
| 体积 | 约 276 MB | 包 23.5 MB + Node 运行时 |
| 运行形态 | 原生二进制，启动快 | Node 解释执行，启动偏慢、内存偏高 |
| 更新 | 上游/社区跟版快 | 跟随上游、版本滞后 |
| 风险 | 社区包，代码可审计 | ToS/法律灰色地带；勿用官方账号 |

**结论**：已双装共存，试用后留一个主力；长期主力更推荐 opencode（开源、活跃、来源可审计）。

## 常见坑速查

- **/sdcard 不能执行程序**：`~/storage/downloads` 是安卓共享目录，禁止执行权限（chmod 也无效）——二进制必须放在家目录 `~` 里。
- **报 `required file not found` 但文件存在** → glibc 动态二进制，用 `checkbin` 验，见概念笔记。
- **首次 ssh/clone 的 fingerprint 提示** → 输 `yes`。
- **`Permissions ... too open`** → `chmod 600 ~/.ssh/id_ed25519`、`chmod 700 ~/.ssh`。
- **zip 中文名乱码**（Windows 打的包）→ 可试 `unzip -O GBK`（不一定支持）；zip 不保留执行权限，解出可执行文件要 `chmod +x`。
- **华为杀后台断 ssh** → `termux-wake-lock` + 系统设置里给 Termux 关闭电池优化（手动）。
- **镜像慢/失败** → `termux-change-repo` 换清华，再 `pkg update` 重试。

## 可选 · 美化（未随清单自动安装）

```bash
pkg install -y starship eza bat fzf zoxide git-delta lazygit
echo 'eval "$(starship init zsh)"' >> ~/.zshrc
```

- 字体/配色：F-Droid 装 **Termux:Styling**（须与主 App 同渠道）→ 长按终端 → More → Style；手动方式：字体放 `~/.termux/font.ttf` 后 `termux-reload-settings`。
- 图标需 Nerd Font；可加 `alias ll='eza -lah --icons'`。

## 收尾 · 备份

```bash
termux-wake-lock
termux-backup ~/storage/downloads/prefix-$(date +%F).tar.gz
tar -czf ~/storage/downloads/home-$(date +%F).tar.gz -C ~ --exclude=storage --exclude=projects .
```

- `termux-wake-lock`：长 ssh/编译前阻止 CPU 休眠；结束时 `termux-wake-unlock`。
- `termux-backup` **只备份 `$PREFIX`（软件本体），不含 `$HOME`**——所以第二条 tar 不可省（dotfiles、`.ssh` 都在 $HOME）。
- 恢复：`termux-restore` 还原 `$PREFIX`；home 备份包手动解到 `~`。
