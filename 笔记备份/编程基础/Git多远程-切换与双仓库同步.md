---
date: 2026-09-09
tags:
  - 入门
  - git
---

# Git 多远程：切换与双仓库同步

> [!note] 缘起
> 笔记库已备份到 Gitee（私有仓库），如果未来想换 GitHub、或同时向两个平台备份，该怎么做？本文讲透 Git 的"远程书签"机制。

相关：[[somezhishi/编程基础/SSH密钥-修改口令与ssh-keygen参数详解|SSH 密钥]] · [[somezhishi/编程基础/Linux权限-chmod详解|Linux 权限：chmod]] · [[somezhishi/编程基础/Linux任务控制-JobControl与kill|Linux 任务控制]]

---

## 一、核心概念：远程只是"命名书签"

Git 的 remote（远程）**不是绑定关系，而是一个可改名的地址书签**。系统不限制数量，想加几个都行。

```bash
git remote -v                 # 查看所有远程（v = verbose，连地址一起显示）
git remote add 名字 地址       # 新增远程书签
git remote remove 名字         # 删除书签
git remote rename 旧名 新名    # 改名
git remote set-url 名字 新地址  # 改书签指向的地址
```

典型状态（本库当前）：

| 书签名 | 指向 | 说明 |
| --- | --- | --- |
| `origin` | `git@gitee.com:fghmnst/somezhishi.git` | 第一个远程的惯例名字，被本地 master 跟踪 |

- 本地分支对远程的"跟踪"叫 **upstream（上游）**：`git branch -vv` 查看 `master [origin/master]`
- 有了上游，`git push` / `git pull` 不用带参数也能知道推/拉谁

---

## 二、场景一：切换单一远程到 GitHub

```bash
git remote set-url origin git@github.com:fghmnst/somezhishi.git
git push -u origin master
```

### 参数逐解

| 部分 | 含义 |
| --- | --- |
| `set-url` | 只修改书签指向的地址。提交历史、本地文件**全部保留** |
| `git@github.com:用户/仓库.git` | SSH 格式地址（git@服务器:路径） |
| `-u` | = `--set-upstream`，把 `origin/master` 设为本地 master 的新上游 |
| `git push -u origin master` | 推送到新地址，并重建追踪关系 |

### 前提条件（容易漏）

1. GitHub 上建好同名**私有**仓库（New repository → Private，不勾选任何初始化文件）
2. 公钥登记 GitHub：把 `~/.ssh/id_ed25519.pub` 内容加到 GitHub → Settings → **SSH and GPG keys**（**同一把密钥可登记多个平台**，无需重新生成）
3. 验证连通：`ssh -T git@github.com`（首次询问信任主机时输入 `yes`）

---

## 三、场景二：同时上传两个远程（双备份）

### 做法 A：两个书签，各推各的（推荐）

```bash
git remote add github git@github.com:fghmnst/somezhishi.git
git push -u github master
```

日常同步：
```bash
git push             # 推 Gitee
git push github      # 推 GitHub
```

要点：一个本地分支可以**同时跟踪多个远程**；`-u github` 只是让 master 额外跟踪 `github/master`，不影响原 origin。

### 做法 B：一个书签推两处

```bash
git remote set-url --add --push origin git@github.com:fghmnst/somezhishi.git
```

| 参数 | 含义 |
| --- | --- |
| `--add` | 追加一个地址，而不是覆盖原地址 |
| `--push` | 只改"推送地址列表"；fetch 拉取仍用原地址 |

效果：此后 `git push` **一次同时推给 Gitee 和 GitHub**。撤销追加：

```bash
git remote set-url --delete --push origin git@github.com:fghmnst/somezhishi.git
```

| 参数 | 含义 |
| --- | --- |
| `--delete` | 从推送地址列表中移除指定地址 |

缺点：想只推一边时写法绕（需精确 URL），对新手推荐做法 A。

---

## 四、场景三：更多仓库 / 进阶

- 再加仓库 = 重复"加书签 + 推送"循环：`git remote add 名字 地址 && git push -u 名字 master`
- GitHub Actions **Mirror 镜像**：建工作流让 GitHub 每次自动同步到 Gitee——适合"主平台 + 自动镜像"，属进阶玩法

---

## 五、排查与速查

```bash
git remote -v                     # 看所有地址（先看这里）
git branch -vv                    # 看 master 跟踪的是谁
git push 书签名                    # 定向推送某个远程
git push                          # 推上游远程（等价 git push origin）
```

| 症状 | 常见原因 |
| --- | --- |
| `Permission denied (publickey)` | 公钥没登记到目标平台 / 私钥有口令且无输入途径 |
| push 后网页还是旧内容 | 推到的是另一个仓库 / 分支名不一致（默认分支 master vs main） |
| 想"只改地址不动历史" | `git remote set-url`，永远不要删了重建 |

---

## 六、一句话总结

- 远程 = 书签：`add` 加、`set-url` 改、`remove` 删
- 切平台：改书签地址 + `push -u` 重建跟踪
- 双备份：两个书签各推各的，或 `--add --push` 一推两处
- 密钥是"人"的资产：一把无口令密钥可登记到 Gitee、GitHub、任何服务器
