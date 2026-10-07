# DianDengMaster · 串口双舵机云台（STM32 练习工程）

> 视觉追踪火控云台的主线练习工程：PC 端 WASD 键盘测试 → 串口发送增量 → STM32F103C8T6 解析 → PWM 驱动 2×SG90 舵机。

## 功能

- **72MHz 主频**（HSE 8MHz ×9）
- **双舵机 PWM**：TIM2 CH1(PA0) / CH2(PA1)，50Hz（Prescaler 71 / Period 19999 / Pulse 1500 归位）
- **串口控制**：USART1(PA9/PA10) 115200，协议 `X增量,Y增量\n`（sscanf 解析，限位 500–2500）
- **PC 端测试**：`scripts/servo_test.py`——WASD 单键控制双舵机（termios 无回车）

## 硬件接线

| 引脚 | 外设 | 说明 |
|---|---|---|
| PA0 | TIM2_CH1 | X 舵机信号线（橙色） |
| PA1 | TIM2_CH2 | Y 舵机信号线（橙色） |
| PA9 / PA10 | USART1_TX / RX | ↔ CH340（交叉接线：PA9→CH340 RX，PA10→CH340 TX） |
| 3.3V / GND | 电源轨 | 与 CH340 共地 |

**注意**：2×SG90 舵机需 **5V/2A 独立供电**（建议升压模块或 2×18650+降压），3.7V 直连会力矩不足；舵机供电负极必须与板子共地。

## 烧录

VS Code 打开本目录 → ST-Link 接入（SWD）→ F5 运行（launch.json 已配 ST-Link GDB）。

命令行备选（STM32CubeCLT）：

```bash
cmake --build build/Debug
arm-none-eabi-objcopy -O binary build/Debug/DDM_test.elf build/Debug/DDM_test.bin
STM32_Programmer_CLI -c port=SWD mode=UR -w build/Debug/DDM_test.bin 0x08000000 -v -rst
```

## 运行 WASD 测试

```bash
pip install pyserial
python3 scripts/servo_test.py --port /dev/ttyUSB0 --step 50
```

- `WASD`：X 左/右、Y 上/下；`Q/E`：退出；`FLIP_X` / `FLIP_Y` 在脚本顶部可翻转方向（Y 舵机实测需 `FLIP_Y=True`）

## 目录结构

```
DianDengMaster/
├── Core/               # 用户代码（main.c 串口解析 + 舵机 PWM 核心逻辑）
├── Drivers/            # HAL 库 + CMSIS（ST 官方，编译必需）
├── cmake/              # CubeMX 生成的构建配置 + arm-none-eabi toolchain
├── .vscode/            # launch.json（F5 烧录调试）
├── scripts/            # PC 端工具（servo_test.py WASD 测试）
├── DDM_test.ioc        # CubeMX 工程（重新生成可还原外设配置）
├── CMakeLists.txt      # 工程入口（project: DDM_test）
└── CMakePresets.json   # Debug/Release 构建预设
```

## 串口协议

每行一帧：`X增量,Y增量\n`（整数，单位 = PWM 脉宽计数）。例：`50,-100` → X 舵机 +50、Y 舵机 -100，超出 [500, 2500] 自动限位。

## 相关

- 复刻对象：视觉追踪火控云台（OpenCV 识别 + PID，PC 端 `ball_track.py` / `pid.py` 后续接入）
- 芯片：STM32F103C8T6（Blue Pill，72MHz，64KB Flash）
