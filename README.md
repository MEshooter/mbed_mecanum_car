[![中文](https://img.shields.io/badge/语言-中文-red)](./README.md)
[![English](https://img.shields.io/badge/Language-English-blue)](./README.en.md)

# 基于 Mbed 和树莓派的四驱视觉小车 - 运动控制部分

## 项目简介

这是一个基于 mbed OS 的四驱全向小车控制项目。mbed 主控负责：

- 接收蓝牙串口命令
- 接收和转发树莓派串口命令
- 控制 4 个电机实现前后左右平移与原地旋转
- 使用超声波模块进行前向避障保护

项目中包含的 `mbed-os` 为运行环境依赖，本项目的业务逻辑主要位于：

- `main.cpp`
- `hardware_libs/`
- `common_libs/`

## 功能概览

- 按键模式控制前进、后退、左移、右移、旋转
- 摇杆模式通过二维速度向量控制运动方向
- 支持基础速度调节
- 支持超声波前向防撞
- 支持将部分高层命令透传给树莓派
- 预留云台舵机、视觉识别、目标跟踪等扩展接口，实际由树莓派实现

## 实物展示

<p align="center">
    <img src="https://github.com/user-attachments/assets/b6aaeaaa-1f19-4b45-aaf5-7ac2d83b1c72" alt="Car-front-left" width="360"/>
    <img src="https://github.com/user-attachments/assets/b936af82-56b9-4dd4-a999-b21042754ff6" alt="Car-back-left" width="360"/>
</p>

<p align="center">
    <img src="https://github.com/user-attachments/assets/5f4c9f4c-d136-490d-8f35-65b1cb6441a4" alt="Car-front" width="270"/>
</p>

## 目录结构

```text
.
├─ main.cpp                 主程序入口，串口收发、命令解析、避障控制
├─ mbed_app.json            mbed 配置
├─ common_libs
│  └─ Vector.hpp            二维向量工具
├─ hardware_libs
│  ├─ MotionControl.*       运动控制与四轮速度分配
│  ├─ StdMotor.*            标准 PWM 电机驱动
│  ├─ SimpleMotor.*         简化电机驱动（当前主程序未使用）
│  ├─ Servo.*               舵机控制库（当前主程序未使用）
│  └─ Ultrasonic.hpp        超声波测距
└─ mbed-os                  mbed OS 依赖
```

## 硬件接口

### 电机

主程序中定义了四个电机对象：

- `BR(p21, p19, p18, 50)`
- `BL(p22, p17, p16, 50)`
- `FR(p23, p15, p14, 50)`
- `FL(p24, p13, p12, 50)`

每个 `StdMotor` 包含：

- 1 路 PWM 输出
- 2 路方向逻辑输出

其中 `50` 表示 PWM 周期为 `50 us`。

### 串口

- 蓝牙串口：`bt(p9, p10)`，波特率 `9600`
- 树莓派串口：`rpi(p28, p27)`，波特率 `115200`
- PC 调试串口：`pc(USBTX, USBRX)`

### 超声波

- `trig`: `p29`
- `echo`: `p30`

## 软件架构

### 1. 主线程

主线程持续执行两件事：

- 读取超声波距离并根据距离决定是否锁定前进
- 从邮箱中取出串口命令并执行解析

### 2. 输入线程

项目额外启动了一个 `inputThread` 负责串口接收：

- 蓝牙串口和树莓派串口通过 `sigio` 触发中断
- 中断中只设置事件标志，不直接处理数据
- 输入线程收到事件后读取串口字符
- 一行命令结束后封装为 `CmdInfo` 放入 `Mail<CmdInfo, 16>` 邮箱

这种设计避免了在中断中执行复杂逻辑，也降低了串口接收和主控制流程之间的耦合。

### 3. 运动控制层

`MotionControl` 负责将期望运动分解为四个轮子的速度：

- 按键模式下，根据 `f/b/l/r` 四个方向状态合成单位方向向量
- 摇杆模式下，根据输入的 `(v_x, v_y)` 直接生成单位方向向量
- 旋转量由 `w` 表示，取值为 `-1 / 0 / 1`

四轮速度分配公式如下：

```text
speedFL = V.y + V.x + W
speedFR = V.y - V.x - W
speedBL = V.y - V.x + W
speedBR = V.y + V.x - W
```

其中：

- `V = baseSpeed * unit(v_x, v_y)`
- `W = baseSpeed * w`

## 本地调试方法

1. 将项目文件完整克隆到同一目录中
2. 确保 `mbed-os` 依赖已经就位
3. 使用 Mbed Studio 导入项目
4. 编译并下载到目标开发板
5. 通过串口助手或蓝牙端发送命令进行调试

## 控制模式

项目支持两种控制模式：

- `BUTTON`：按键模式，适合通过离散指令控制方向
- `JOYSTICK`：摇杆模式，适合通过二维向量控制速度方向

可通过命令动态切换。

## 命令协议

项目支持两类命令：

- 单命令：以 `#` 开头
- 多参数命令：以 `:` 开头

### 单命令

格式示例：

```text
# U
```

说明如下：

- `U / u`：开始前进 / 停止前进
- `D / d`：开始后退 / 停止后退
- `L / l`：开始左移 / 停止左移
- `R / r`：开始右移 / 停止右移
- `B / b`：开始逆时针旋转 / 停止旋转
- `E / e`：开始顺时针旋转 / 停止旋转
- `S / s`：打开 / 关闭超声波避障模式

说明：

- 大写表示使能某个动作
- 小写表示取消某个动作
- `B` 与 `E` 不会叠加旋转，重复触发会按代码逻辑切换或清零

### 多参数命令

格式示例：

```text
: V 0.71 0.71
```

#### 1. 速度向量命令

```text
: V v_x v_y
```

含义：

- 发送速度向量 `v = (v_x, v_y)`
- 参数为浮点数
- 主要用于摇杆模式

说明：

- 程序内部会对向量进行单位化
- 因此输入主要表示方向，实际速度大小由基础速度 `baseSpeed` 决定

#### 2. 模式切换命令

```text
: SM JS
: SM BT
```

含义：

- `SM JS`：切换到摇杆模式
- `SM BT`：切换到按键模式

#### 3. 基础速度调节命令

```text
: SPD speed
```

含义：

- 调整基础速度等级
- 参数为整数

实现说明：

- 主程序会先将输入转换为 `5 + speed * 0.5`
- `MotionControl::setBaseSpeed()` 再将其限制到 `[5, 10]`
- 最终内部 `baseSpeed` 范围为 `50 ~ 100`

这意味着：

- `speed` 太小会被限制到最低速度
- `speed` 太大也会被限制到最大速度

#### 4. 舵机控制命令

```text
: SVO UD deg
: SVO LR deg
```

含义：

- 调整云台上下或左右方向的舵机角度
- `deg` 为角度值，建议范围 `-90` 到 `90`

说明：

- 当前 mbed 主程序并不直接解析该命令参数
- 该命令会被原样转发到树莓派

#### 5. 摄像头控制命令

```text
: CAM SHOT
: CAM REC
: CAM END
```

含义：

- `SHOT`：拍照
- `REC`：开始录像
- `END`：结束录像

说明：

- 该命令由 mbed 转发到树莓派处理

#### 6. AI 识别开关命令

```text
: AI 1
: AI 0
```

含义：

- 打开 / 关闭人像或物体识别

说明：

- 该命令由 mbed 转发到树莓派处理

#### 7. 目标跟踪命令

```text
: TRK 1
: TRK 0
```

含义：

- 开始 / 结束目标跟踪

说明：

- 该命令由 mbed 转发到树莓派处理

#### 8. 目标框信息命令

```text
: OBJ x_c y_c a b
```

含义：

- 描述目标物体的中心位置和外接框大小
- `x_c`、`y_c` 表示目标中心坐标
- `a`、`b` 表示目标框在 `x`、`y` 方向上的半轴长度

说明：

- 该分支原本希望实现目标跟踪功能，通过传入目标在视野内的位置与移动方向，利用 PID 控制自动追踪，但实际未采用

## 树莓派透传机制

以下命令会被 mbed 直接透传给树莓派：

- `SVO`
- `CAM`
- `TRK`
- `AI`

mbed 的行为是：

1. 向蓝牙端返回一条提示消息，例如 `Sent ": CAM SHOT"`
2. 向树莓派串口发送完整命令并补上换行

因此，mbed 在整个系统中既是底盘控制器，也是上层视觉命令的转发节点。

## 超声波避障逻辑

项目提供简单的前向避障功能。

### 开关方式

通过以下命令切换：

```text
# S
```

蓝牙端会收到类似反馈：

```text
Ultrasonic Mode: 1
```

### 生效方式

主循环中会不断测量距离并计算：

```text
distLimit = 15 * speedGrade - 70
```

当前代码中：

- `speedGrade = 7`
- 因此 `distLimit = 35`

当满足以下条件时，会开启前进锁定：

- 超声波模式已开启
- `distance < 35 cm`
- `distance > 2 cm`

锁定后：

- 电机的正向速度会被强制置为 `0`
- 但后退和其他非正向输出仍可执行

## 关键类说明

### `MotionControl`

作用：

- 保存当前控制模式
- 保存按键状态
- 保存基础速度
- 保存旋转方向
- 根据目标运动更新四轮速度

关键接口：

- `changeMode(ControlMode)`
- `addDir(const Direction&)`
- `delDir(const Direction&)`
- `setRotation(int)`
- `setBaseSpeed(double)`
- `updateMotion(const Vector<double>&)`
- `stop()`

### `StdMotor`

作用：

- 根据速度值控制一个直流电机
- 使用 PWM 控制速度
- 使用两个逻辑引脚控制方向

速度约定：

- `speed > 0`：正向
- `speed < 0`：反向
- 范围限制为 `[-100, 100]`

### `Ultrasonic`

作用：

- 通过触发脉冲和回波测时获取距离

距离计算：

```text
distance = echo_high_time_us * 0.017
```

返回值单位通常可理解为厘米。

### `Servo`

作用：

- 将角度映射为 PWM 脉宽控制舵机

注意：

- 注释中明确提到 LPC1768 的 PWM 资源限制
- 若电机已经占用了某些 PWM 能力，舵机库可能需要谨慎使用

## 调试输出

程序会通过串口打印以下信息，便于调试：

- 启动成功信息：`Successfully started.`
- 接收到的命令内容
- 当前速度向量：`VELOCITY: ...`
- 四个轮子的目标速度：`SPEED: ...`

这些输出对排查命令解析、速度分配和避障逻辑都很有帮助。
