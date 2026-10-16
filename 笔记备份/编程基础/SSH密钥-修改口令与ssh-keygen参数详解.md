---
date: 2026-09-09
tags:
  - 入门
  - ssh
  - linux
---

# SSH 密钥：修改/移除口令（ssh-keygen -p 全参数详解）

> [!note] 缘起
> 一次真实事故：密钥创建时设了口令（passphrase），导致 WSL 里 SSH 认证一直被服务器拒绝（`Permission denied (publickey)`）——因为客户端拿不到口令就无法用私钥签名。本文完整讲解"移除口令"的每一个命令与参数。

相关：[[somezhishi/编程基础/Linux权限-chmod详解|Linux 权限：chmod]]

---

## 一、背景：什么是密钥口令？

SSH 密钥对 = 私钥 + 公钥。创建密钥时可附加一层**口令加密**：私钥文件用对称加密写盘，使用时必须先提供口令解密。

| 状态 | 表现 |
| --- | --- |
| 私钥带口令 | 用私钥签名前要解密；无口令来源时认证直接失败 |
| 私钥无口令 | 拿到文件即可用（方便但安全性下降） |

> [!warning] 事故现象备忘
> 带口令私钥在**没有交互终端或 ssh-agent** 的环境（如自动化脚本、WSL 非交互会话）下会报：
> `Load key "...": incorrect passphrase supplied to decrypt private key`
> 最终表现为服务器端 `Permission denied (publickey)`——看起来像公钥不匹配，实则是私钥没解开。

---

## 二、核心命令：ssh-keygen 修改口令

```bash
ssh-keygen -p -f ~/.ssh/id_ed25519
```

### 2.1 逐参数拆解

| 部分 | 含义 |
| --- | --- |
| `ssh-keygen` | SSH 密钥管理工具：生成、修改、查看密钥（SSH 全家桶自带） |
| `-p` | 修改密钥口令模式。p = **p**assphrase。不带 -p 时默认是"生成新密钥" |
| `-f` | 指定要操作的密钥**文件路径**。f = **f**ile。指向私钥文件即可 |
| `~/.ssh/id_ed25519` | 目标私钥（`~` = 家目录展开为 `/home/fgh`，id_ed25519 = ED25519 算法的默认密钥名） |

### 2.2 交互过程（共 3 次输入）

1. `Enter old passphrase:` → 输入**旧口令**（若本无口令直接回车）
2. `Enter new passphrase:` → 输入新口令，或**直接回车 = 移除口令**
3. `Enter same passphrase again:` → 再输入一次确认（或再回车）

看到 `Your identification has been saved` 即成功。

### 2.3 Windows 版写法

原密钥在 Windows 时，PowerShell 中路径用环境变量写法：

```powershell
ssh-keygen -p -f "$env:USERPROFILE\.ssh\id_ed25519"
```

| 部分 | 含义 |
| --- | --- |
| `$env:USERPROFILE` | PowerShell 取当前用户目录（= `C:\Users\wyb21`） |
| `\` | Windows 路径分隔符 |
| 外层 `"..."` | 防止路径含空格时被拆成多个参数 |

---

## 三、非交互写法（参数直接带口令）

```bash
ssh-keygen -p -f ~/.ssh/id_ed25519 -P "旧口令" -N ""
```

| 参数 | 含义 |
| --- | --- |
| `-P` | 直接提供**旧**口令（P = old **p**assphrase），免去第一次提示 |
| `-N` | 直接指定**新**口令（N = **n**ew passphrase），`""` 空串 = 移除口令 |

> [!warning] 安全提示
> `-P`/`-N` 会把口令明文留在命令行历史（`.bash_history`）里。只有脚本化场景才值得用；手工操作推荐纯交互式（第二节），口令不落盘。

---

## 四、配套验证命令

```bash
ssh-keygen -y -f ~/.ssh/id_ed25519        # 用私钥反推公钥（验证能否无口令解密）
ssh-keygen -lf ~/.ssh/id_ed25519.pub      # 查看公钥指纹（l = list，f = file）
ssh -T git@gitee.com                       # 实测认证是否通过
```

| 参数 | 含义 |
| --- | --- |
| `-y` | 从私钥推导公钥（y = derive）。若私钥带口令会当场要求输入 |
| `-l` | 列出密钥指纹（fingerprint），用于比对服务器登记是否一致 |
| `ssh -T` | T = 不分配终端。Gitee 仅用于认证测试，成功后返回欢迎语 |

---

## 五、延伸：其他常用参数（排查时用过的）

```bash
ssh-keygen -t ed25519 -f ~/.ssh/新密钥名 -N "" -C "注释文字" -q
```

| 参数 | 含义 |
| --- | --- |
| `-t ed25519` | 指定算法（t = type）。ed25519 是当前推荐的安全快速算法 |
| `-f` | 输出文件路径（不写则交互式问路径） |
| `-N ""` | 新密钥口令 = 空（免交互生成无口令密钥） |
| `-C "注释"` | 给公钥加注释（C = comment），便于识别用途 |
| `-q` | 安静模式（q = quiet），少打印过程信息 |

```bash
ssh-add ~/.ssh/id_ed25519   # 把（带口令的）密钥装入 ssh-agent 内存
```

| 参数 | 含义 |
| --- | --- |
| `ssh-add` | 把密钥加入 ssh-agent：首次使用时输入一次口令，之后该会话内免密 |
| 文件路径 | 指定要加载的私钥；不带参数则加载默认密钥 |

---

## 六、经验总结

1. **口令 ≠ 密码登录**：口令只保护私钥文件本身，与服务器账号密码无关
2. **排查顺序**：先 `ssh-keygen -y -f 私钥` 确认能否解密，再看公钥是否在服务器登记，最后 `ssh -vT` 看详细过程
3. **对照实验法**：怀疑密钥有问题时，临时生成一把新密钥测试（`ssh-keygen -t ed25519 -f /tmp/test -N ""`），一次就能定位是"密钥问题"还是"账号/网络问题"
4. **移除口令 = 降低安全等级**：文件泄露即被冒用；介意的话用 ssh-agent 替代（保留口令但只在会话内免输）
5. **跨系统复用密钥**：Windows 生成的密钥可直接拷贝到 WSL 使用（同为 OpenSSH 格式），记得 `chmod 600` 私钥
