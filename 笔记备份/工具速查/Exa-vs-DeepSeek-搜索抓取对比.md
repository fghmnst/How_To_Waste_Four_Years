---
date: 2026-10-29
tags:
  - 决策
  - 工具
  - opencode
---

# Exa 与 DeepSeek 搜索/抓取对比

> [!tip] 结论先行
> 个人在 OpenCode 里用，**继续用 Exa**。检索质量领先约 17%，有 $10/月免费额度，集成成本为零。DeepSeek Search 更便宜、更快，但质量落后、有 128 字符硬限制，而且要自己包工具。抓取继续用内置 `webfetch`，遇到 JS 站点再用 DeepSeek Fetch 兜底。

## 一、先分清两件事：搜索 vs 抓取

这两个词经常被混着说，其实是两条完全不同的链路：

| | 搜索（search） | 抓取（fetch） |
|---|---|---|
| 你要什么 | “帮我找几个关于 X 的网页” | “把这个 URL 的内容读出来” |
| 前提 | 不知道网址 | 已经知道网址 |
| 需要索引 | 需要，搜索引擎先爬好全网 | 不需要，直接发 HTTP 请求 |
| 需要第三方 | 需要（Exa、DeepSeek 等） | 不需要（浏览器/curl 就能做） |
| 类比 | 问图书管理员“哪本书讲这个” | 拿到书名后自己去翻那一页 |

所以“抓取要不要配第三方”的答案是：**不需要**。OpenCode 内置的 `webfetch` 就是一个带格式转换的 curl。

## 二、选手是谁

### Exa

- 2021 年成立（原名 Metaphor），做的是**语义搜索**：用 embedding 按“意思”找网页，而不是关键词匹配。
- 客户是 AI 应用和 agent，不是普通用户。
- OpenCode 原生支持它作为 `websearch` provider，配置只要一行。
- 价格：Search auto 档 $7/1k 次；免费额度 $10/月，月初重置，不滚存。

### DeepSeek Search / Fetch

- 2026 年 10 月上线，用**自研搜索引擎**，不是接别人的索引。
- 两个独立端点：
  - `POST https://api.deepseek.com/v1/tools/search`
  - `POST https://api.deepseek.com/v1/tools/fetch`
- 价格：各 $0.75/1k 次成功请求，无免费额度。
- 返回字段很丰富：`rerank_score`（0–3 分）、`authority`（0–4 级）、`summary`、`content`。
- 支持 `site:`、`-site:`、`lang:`、`after:`、`before:` 和精确短语。

## 三、怎么比的

不是拍脑袋，跑了一套小基准：

| 项目 | 设计 |
|---|---|
| 检索 query | 30 条，分 6 类：中文时事、中文技术、英文技术、英文商业、长尾语义、长查询 |
| 重复 | 每条 query 每家跑 2 次，取第二次做质量、两次算延迟 |
| 公平性 | 同样的 query 文本、同样取 10 条、同一小时内跑完 |
| 裁判 | 两个**跨家族** LLM：Kimi K3 和 GLM-5.3，去掉来源标签盲评 |
| 评分维度 | 相关性、权威性、时效性、多样性、回答充分性，各 0–10 分 |
| 抓取 | 8 个 URL：静态文档、国内站点、JS 站点、GitHub、Markdown、PDF、Wikipedia、Reddit |
| 成本 | 按官方列表价计算 |

> 为什么要两个裁判？单个 LLM 会有偏好。两个不同家族的模型一致率 90%，结论才比较可信。

## 四、结果

### 4.1 检索质量

满分 50（5 个维度各 10 分）：

| 类目 | Exa（Kimi） | DeepSeek（Kimi） | Exa（GLM） | DeepSeek（GLM） |
|---|---|---|---|---|
| 中文时事 | **42.8** | 36.2 | **42.8** | 36.0 |
| 中文技术 | **38.4** | 32.2 | **37.2** | 32.6 |
| 英文技术 | **38.2** | 31.6 | **39.0** | 32.4 |
| 英文商业 | **42.6** | 34.8 | **42.6** | 34.8 |
| 长尾语义 | **36.2** | 31.0 | **37.4** | 31.4 |
| 长查询 | **34.6** | 32.6 | **33.2** | 31.2 |
| **总计** | **38.8** | **33.1** | **38.7** | **33.1** |

胜场：Kimi 27:3，GLM 26:4，两裁判一致率 **90%**。

分维度：

| 维度 | Exa | DeepSeek | 说明 |
|---|---|---|---|
| 相关性 | **8.8** | 7.0 | Exa 结果更直接切题 |
| 权威性 | **7.9** | 5.9 | Exa 多官方文档/主流媒体；DS 多 CSDN、聚合站 |
| 时效性 | 7.1 | 7.1 | 打平；DS 有日期的比例更高（80% vs 50%） |
| 多样性 | 6.8 | 6.6 | 接近 |
| 充分性 | **8.3** | 6.4 | Exa 的 highlights 是按查询锚定的摘录，更能直接回答问题 |

一个典型例子：搜“STM32 HAL UART DMA 环形缓冲”，Exa 前几条是 ST 官方 GitHub 示例、CSDN 教程、GitHub 代码；DeepSeek 前几条是 StackOverflow 修订页、CSDN 文库（付费墙）、一篇无关的 git diff。

### 4.2 速度与成本

| 指标 | Exa | DeepSeek | 赢家 |
|---|---|---|---|
| p50 延迟 | 911ms | **466ms** | DeepSeek（快约 49%） |
| p95 延迟 | 1250ms | **747ms** | DeepSeek |
| 单价 | $7/1k | **$0.75/1k** | DeepSeek（便宜 9.3 倍） |
| 免费额度 | **$10/月** | 无 | Exa |
| 平均去重域名 | 6.9 | **8.1** | DeepSeek |
| 有日期结果比例 | 50% | **80%** | DeepSeek |

> [!note] 关键点
> 单价看着差 9 倍，但个人一个月搜几十到几百次，Exa 的 $10/月额度根本用不完，**实付都是 0**。所以“便宜 9 倍”在你的场景里不成立。

### 4.3 抓取质量

| URL | 类型 | webfetch | DeepSeek Fetch | 结论 |
|---|---|---|---|---|
| opencode 文档 | 静态 | 10.2k 字符，426ms | 9.7k，5.2s | webfetch 快 12 倍 |
| deepseek.com/news | 国内站点 | 3.4k，289ms | 468 字符，118ms | webfetch 内容更全 |
| react.dev | JS 站点 | **404** | 3.8k，145ms | DeepSeek 胜 |
| GitHub 仓库 | GitHub | 21.3k，2.1s | 7.7k，10.3s | webfetch 更快更全 |
| exa 的 .md 文档 | Markdown | 51.3k，1.3s | 101.9k，3.9s | DeepSeek 更完整 |
| arXiv PDF | PDF | 不支持 | 404 | 都不行 |
| Wikipedia | 大页面 | 50.5k，0.9s | **404** | webfetch 胜 |
| Reddit | 反爬 | “成功”但 0 字符 | 404 | 都失败 |

## 五、三个意外发现

### 1. DeepSeek 的 128 字符是硬限制

不仅 `query` 限制 128 字符，`ori_question`（原问题）**也限制 128**。实测 176 字符直接报错：

```json
{"error":{"message":"'query' must be at most 128 characters"}}
```

这意味着复杂长问题没法靠“原问题字段”绕过，只能截断或拆成多次搜索。Exa 支持长自然语言查询，这是实质差距。

### 2. webfetch 在 react.dev 的 404 是“内容协商”问题

同一个 URL：

```bash
curl https://react.dev/                                   # 200
curl -H "Accept: text/markdown" https://react.dev/        # 404
```

`webfetch` 默认请求 markdown，站点不支持这个 Accept 就直接回 404，而不是回退到 HTML。**不是网络问题，也不是站点挂了。**

### 3. DeepSeek Fetch 不是通用抓取器

Wikipedia 主页、Reddit 在 curl 下都是 200，但 DeepSeek Fetch 一律 404。它更像是“只抓自己索引/白名单里的站点”，不是任意 URL 的 HTTP 客户端。另外它也不支持 PDF。

## 六、优劣势总结

### Exa（当前配置）

**优势**

- 检索相关性、权威性、回答充分性明显更好
- OpenCode 原生集成，零代码，配置热加载
- 支持长自然语言查询
- $10/月免费额度，个人用量实付 0
- 配套的 webfetch 免费、快、站点覆盖广

**劣势**

- 单价高，规模化时是 DeepSeek 的 9.3 倍
- 延迟更高
- 偶尔过度返回自家域名
- webfetch 对 `Accept: text/markdown` 不兼容的站点会误报 404

### DeepSeek Search / Fetch

**优势**

- 快
- 便宜
- 元数据丰富：`rerank_score`、`authority`、`summary`、`content`
- 日期覆盖率高
- Fetch 在 JS 站点表现好

**劣势**

- 检索质量整体落后
- 128 字符查询上限
- 无免费额度
- OpenCode 不内置，要自己包工具或 MCP
- Fetch 站点覆盖窄，不支持 PDF

## 七、给你的建议

1. **保留 Exa**。你的用量下免费额度足够，质量优势明显，集成成本为零。
2. **不要为了省钱换 DeepSeek Search**。省不到钱，还要维护工具，质量还下降。
3. **DeepSeek Search 的适用场景**：高并发自建产品、需要 `authority`/`rerank_score` 做二次排序、或已经深度绑定 DeepSeek 账单。
4. **抓取继续用 webfetch**；遇到 JS 站点再用 DeepSeek Fetch 兜底；PDF 走 `read` 或专门解析器。
5. **给 webfetch 加长度校验**：空内容不要当成功，必要时重试或换 Fetch。

## 八、术语小词典

| 术语 | 大白话解释 |
|---|---|
| 语义搜索 | 按“意思”找，而不是按关键词匹配。你说“找讲这个问题的公司”，它能找到没出现这些词的页面 |
| embedding | 把一段文字变成一串数字向量，意思相近的文字向量也相近，搜索引擎靠它判断“像不像” |
| highlights | Exa 返回的“按查询锚定的摘录”，只挑和你的问题最相关的几句话 |
| snippet / summary | DeepSeek 返回的短摘要（100–200 字）和长摘要（600–800 字） |
| rerank_score | DeepSeek 给结果的相关性分数，0–3 分，≥1.5 算优质 |
| authority | DeepSeek 给网页的权威性分级，0–4 级，≥3 算很权威 |
| p50 / p95 | 延迟分布：50% 的请求快于 p50，95% 快于 p95；p95 更能反映“最慢的那批” |
| MCP | Model Context Protocol，给 AI 挂外部工具的标准接口，可以理解为“AI 的 USB 接口” |
| Accept 头 | HTTP 请求里告诉服务器“我想要什么格式”的字段，比如 `text/html` 或 `text/markdown` |
| 内容协商 | 客户端说“我要 markdown”，服务器不支持就直接报错，而不是自动回退成 HTML |

## 九、自测

> [!question]- 1. 为什么搜索需要第三方服务，抓取不需要？
> 搜索需要先有一个“爬好全网并建好索引”的引擎，个人搭不起来；抓取只是对已知 URL 发 HTTP 请求，浏览器或 curl 就能做。

> [!question]- 2. DeepSeek 的 128 字符限制影响什么？
> 影响复杂长问题。`query` 和 `ori_question` 都限 128 字符，超过直接 400；只能截断或拆成多次搜索。Exa 没有这个限制。

> [!question]- 3. 为什么 webfetch 在 react.dev 报 404，而 curl 正常？
> webfetch 默认发 `Accept: text/markdown`，react.dev 对这个 Accept 返回 404；curl 默认发 `Accept: */*`，所以是 200。这是内容协商不兼容，不是网络问题。

> [!question]- 4. 你的场景应该选哪个？
> 继续用 Exa。个人用量下免费额度覆盖，质量更高，零集成成本；DeepSeek 只在自建高并发产品或需要结构化元数据时才有优势。

---

关联：[[somezhishi/somezhishi|somezhishi]] · [[somezhishi/工具速查/opencode-TUI-速查表|opencode TUI 速查表]]
