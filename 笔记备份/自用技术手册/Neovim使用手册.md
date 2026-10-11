# Neovim 使用手册（Termux）

> 本手册对应 `~/.config/nvim/` 下这套从零搭建的配置。
> 组成：文件树浏览（neo-tree）＋ 代码补全（nvim-cmp + clangd/pylsp/lua_ls）＋ 语法高亮（nvim-treesitter）。
> 已装插件 14 个，由 lazy.nvim 管理。

---

## 1. 快速开始

| 你想做的事 | 命令 |
|---|---|
| 打开编辑器（自动出现左侧文件树） | `nvim` |
| 打开某个文件 | `nvim 路径/文件.c` |
| 打开某个目录 | `nvim 路径/目录` |
| 首次或插件缺失时补装插件 | 进入 nvim 后执行 `:Lazy sync` |
| 健康检查 | `:checkhealth` |

提示：`leader` 键是**空格**。下文 `空格 w` 表示先按空格再按 w。

---

## 2. 键位速查

### 2.1 全局键位

| 键 | 作用 |
|---|---|
| `空格 w` | 保存 |
| `空格 q` | 关闭当前窗口 |
| `空格 Q` | 强制退出全部 |
| `空格 h` | 清除搜索高亮 |
| `空格 e` | 开关左侧文件树 |
| `Ctrl+h` / `Ctrl+j` / `Ctrl+k` / `Ctrl+l` | 切到 左/下/上/右 窗口 |

### 2.2 文件树（neo-tree 窗口内）

| 键 | 作用 |
|---|---|
| `Enter` 或 `l` | 打开文件 / 展开目录 |
| `h` | 收起目录 |
| `a` | 新建文件 |
| `A` | 新建目录 |
| `d` | 删除（会有确认） |
| `r` | 重命名 |
| `c` | 复制到指定路径 |
| `m` | 移动到指定路径 |
| `y` / `x` / `p` | 复制到剪贴板 / 剪切 / 粘贴 |
| `R` | 刷新 |
| `H` | 显示或隐藏隐藏文件 |
| `.` | 把当前目录设为树根 |
| `q` | 关闭文件树 |
| `?` | 弹出全部键位帮助（不确定就按它） |

> 说明：默认 `l` 是"预览"，本配置改成了"打开"；`h` 改成了"收起目录"；空格键在该窗口内被禁用，避免和 leader 冲突。

### 2.3 补全（插入模式）

| 键 | 作用 |
|---|---|
| `Ctrl+空格` | 手动触发补全 |
| `Ctrl+n` / `Ctrl+p` | 选中 下一个 / 上一个 |
| `Enter` | 确认选中项 |
| `Ctrl+e` | 取消补全 |
| `Tab` / `Shift+Tab` | 有补全菜单时切换；否则跳到代码片段的占位符 |

### 2.4 LSP（打开代码文件、服务器附加后）

| 键 | 作用 |
|---|---|
| `gd` | 跳到定义 |
| `gD` | 跳到声明 |
| `gr` | 列出所有引用 |
| `gi` | 跳到实现 |
| `K` | 悬停显示文档 |
| `空格 rn` | 重命名符号 |
| `空格 ca` | 代码操作（补全导入、快速修复等） |
| `空格 d` | 弹窗显示当前行诊断 |
| `]d` / `[d` | 下一个 / 上一个诊断 |

---

## 3. 常用命令

| 命令 | 作用 |
|---|---|
| `:Lazy` | 打开插件管理面板 |
| `:Lazy sync` | 安装/更新缺失插件 |
| `:Lazy update` | 更新全部插件 |
| `:Lazy clean` | 删除不再使用的插件 |
| `:Neotree toggle` | 开关文件树 |
| `:Neotree reveal` | 在树中定位当前编辑的文件 |
| `:checkhealth` | 总体健康检查 |
| `:checkhealth vim.lsp` | 查看 LSP 服务器状态 |
| `:lua =vim.lsp.get_clients()` | 列出当前附加的 LSP 客户端 |
| `:TSInstall python` | 安装某语言的 treesitter 解析器 |
| `:TSUpdate` | 更新已安装的解析器 |

> 注意：**不要用 `:Mason`**。Mason 会下载 glibc 二进制，在 Termux（Android bionic）上装不上。语言服务器一律用 `pkg` / `pip` / `npm` 安装。

---

## 4. 配置文件说明

```
~/.config/nvim/
├── init.lua                 入口：设置 leader，加载下列模块
├── lazy-lock.json           插件版本锁定文件（自动生成）
└── lua/
    ├── options.lua          编辑器选项（行号、真彩、鼠标、持久撤销等）
    ├── keymaps.lua          全局键位
    ├── lazy_bootstrap.lua   自动安装并启动 lazy.nvim
    └── plugins/
        ├── neotree.lua      文件树
        ├── cmp.lua          补全引擎 nvim-cmp
        ├── lsp.lua          LSP 服务器接线
        └── treesitter.lua   语法高亮
```

---

## 5. 如何给新语言加代码补全

三步。以新增 TypeScript 为例：

1. **装语言服务器**（用系统包管理器，不要用 Mason）：
   ```sh
   npm i -g typescript typescript-language-server
   ```
   其它常见例子：C/C++ 用 `pkg install clang`（自带 clangd）；Python 用 `pip install python-lsp-server`；Lua 用 `pkg install lua-language-server`。

2. **在 `lua/plugins/lsp.lua` 里启用**（服务器名要写 nvim-lspconfig 里的名字）：
   ```lua
   vim.lsp.config("ts_ls", {})
   vim.lsp.enable("ts_ls")
   ```

3. **（可选）装语法高亮**：进入 nvim 执行
   ```
   :TSInstall typescript
   ```

保存后重启 nvim 生效。

---

## 6. 如何安装新插件

1. 在 `lua/plugins/` 下新建一个 `.lua` 文件，返回一个 lazy.nvim 插件规格：
   ```lua
   return {
     "作者/仓库名",
     config = function()
       -- 可选：插件的 setup 配置
     end,
   }
   ```
2. 进入 nvim 执行 `:Lazy sync` 安装。

---

## 7. 图标（Nerd Font）

当前文件树的图标是**开启的**，使用 neo-tree 默认的 Nerd Font 图标（目录图标、文件类型图标、展开箭头）。因为终端字体已是 Nerd Font，无需额外配置；`~/.config/nvim/lua/plugins/neotree.lua` 里用的是 neo-tree 默认图标，没有做任何覆盖。

如果将来换到**没有 Nerd Font 的终端**，图标会显示成"豆腐块"方框。两种处理方式：

- **给终端装 Nerd Font**：下载一款（如 JetBrainsMono Nerd Font）的 `.ttf`，放到字体目录：
  ```sh
  cp 下载的字体.ttf ~/.termux/font.ttf
  termux-reload-settings
  ```
  下载页：https://www.nerdfonts.com/font-downloads ，或 https://github.com/ryanoasis/nerd-fonts/releases

- **关闭图标**（回到纯文本显示）：在 `~/.config/nvim/lua/plugins/neotree.lua` 的 `default_component_configs` 里加入
  ```lua
  icon = {
    provider = function(icon)
      icon.text = ""
      return icon
    end,
  },
  ```
  并把 `indent` 改成
  ```lua
  indent = { with_expanders = true, expander_collapsed = "▶", expander_expanded = "▼" },
  ```

改完重启 nvim 生效。

---

## 8. 故障排查

| 现象 | 排查 |
|---|---|
| 补全不弹出 | `:checkhealth vim.lsp`；确认对应服务器在 PATH（如 `which clangd pylsp lua-language-server`） |
| Python 第三方库（cv2/numpy）没提示 | 这些库没装在系统 Python 里，pylsp 解析不到；用 `pip install` 装上对应包 |
| Python 文件没有语法高亮 | 确认 `~/.local/share/nvim/site/queries/python/` 存在；该目录需被加入 runtimepath（配置里已处理） |
| 图标显示成方框 | 当前终端不是 Nerd Font；换字体或按第 7 节关闭图标 |
| 首次启动很慢 | 首次要联网 clone 插件；之后正常 |
| 想看插件是否报错 | `:Lazy` 面板里按 `H` 看 health，或 `:checkhealth` |

---

## 9. 本手册的渲染方法

本文件是标准 Markdown，几种查看方式：

**方法 A：终端内直接渲染（最省事）**
```sh
pkg install glow
glow ~/SomeThingFunny/Neovim使用手册.md
```
也可以在 nvim 里执行 `:!glow %` 就地预览。

**方法 B：转成 HTML 用浏览器看**
```sh
pkg install pandoc
pandoc ~/SomeThingFunny/Neovim使用手册.md -s --metadata title="Neovim 使用手册" -o ~/SomeThingFunny/Neovim使用手册.html
termux-open ~/SomeThingFunny/Neovim使用手册.html   # 需要安装 Termux:API
```

**方法 C：用 Obsidian 打开**
把本文件复制或软链到你已有的 Obsidian 仓库中：
```sh
ln -s ~/SomeThingFunny/Neovim使用手册.md ~/SomeThingFunny/projects/obsidian/Neovim使用手册.md
```
然后用 Obsidian 打开该 vault，即可获得带目录、可点击的最佳阅读体验。

**方法 D：直接在 nvim 里看**
```sh
nvim ~/SomeThingFunny/Neovim使用手册.md
```
treesitter 会高亮 Markdown 语法。想要"所见即所得"的内联渲染，可另装 `render-markdown.nvim` 插件。
