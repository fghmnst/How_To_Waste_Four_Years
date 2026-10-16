---
date: 2026-08-19
tags:
  - 排障
  - wsl
  - linux
---

# WSL 串口权限：dialout 组与 chmod 临时方案

> 2026-08-19。CH340（`/dev/ttyUSB0`）在 WSL 里的权限坑。

## 坑

1. 普通用户打开串口 → `PermissionError: [Errno 13] Permission denied`
2. **临时方案有保质期**：`sudo chmod 666 /dev/ttyUSB0` 只在设备枚举期间有效——CH340 重新插拔（或 usbipd 重新 attach）后**权限丢失**，需再执行一次。第一次测试时连着踩了两次。

## 正确解法（一次性）

```bash
sudo usermod -aG dialout $USER
# 重新登录 WSL（exit 后重开）生效
```

组权限是持久的，不依赖设备枚举。验证：`id` 输出含 `dialout`。

## 记忆点

- 设备重枚举 = chmod 方案失效的信号
- 调试时"刚才还能开串口，现在权限拒绝" → 先想是不是设备重新插拔过
- AGENTS.md 已记录本约定（`fire_control/` 段落）
