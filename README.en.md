[![中文](https://img.shields.io/badge/语言-中文-red)](./README.md)
[![English](https://img.shields.io/badge/Language-English-blue)](./README.en.md)

# 4WD Visual Car Based on Mbed and Raspberry Pi

## Project Overview

This is a 4WD omnidirectional car control project based on mbed OS 6, with basic vision capabilities. Motion control is implemented on the NXP LPC1768, which is mainly responsible for:

- Receiving Bluetooth serial commands
- Receiving and forwarding Raspberry Pi serial commands
- Controlling four motors for omnidirectional translation and in-place rotation
- Using an ultrasonic module for forward obstacle-avoidance protection

For more information about mbed OS and the LPC1768, see the [official mbed website](https://os.mbed.com/). The vision module, including the camera and gimbal, is implemented on the Raspberry Pi in Python, and can be operated through a WeChat mini-program. These two related modules are available here: [Bluetooth controller (WeChat mini-program)](https://github.com/MEshooter/bluetooth_controller), [vision module](https://github.com/MEshooter/berryeye).

The included `mbed-os` directory is a runtime dependency. The main application logic is located in:

- `main.cpp`
- `hardware_libs/`
- `common_libs/`

For the structural design, simulation tests, and circuit design, refer to `/docs/Group 15_Report_Engineering_Design_1.pdf`.

## Features

- Button mode for forward, backward, left, right, and rotation control
- Joystick mode using a 2D velocity vector to control motion direction
- Adjustable base speed
- Ultrasonic forward collision prevention
- Forwarding selected high-level commands to the Raspberry Pi
- Reserved extension interfaces for gimbal servos, vision recognition, and target tracking, with actual implementation on the Raspberry Pi side

## Physical Showcase

You can watch the demo video at [this link](https://www.bilibili.com/video/BV1r3XsBiE4U).

<p align="center">
    <img src="https://github.com/user-attachments/assets/b6aaeaaa-1f19-4b45-aaf5-7ac2d83b1c72" alt="Car-front-left" width="360"/>
    <img src="https://github.com/user-attachments/assets/b936af82-56b9-4dd4-a999-b21042754ff6" alt="Car-back-left" width="360"/>
</p>

<p align="center">
    <img src="https://github.com/user-attachments/assets/5f4c9f4c-d136-490d-8f35-65b1cb6441a4" alt="Car-front" width="270"/>
</p>

## Directory Structure

```text
.
├─ main.cpp                 Main entry point, serial I/O, command parsing, obstacle avoidance
├─ mbed_app.json            mbed configuration
├─ common_libs
│  └─ Vector.hpp            2D vector utility
├─ hardware_libs
│  ├─ MotionControl.*       Motion control and four-wheel speed distribution
│  ├─ StdMotor.*            Standard PWM motor driver
│  ├─ SimpleMotor.*         Simplified motor driver (unused in the current main program)
│  ├─ Servo.*               Servo control library (unused in the current main program)
│  └─ Ultrasonic.hpp        Ultrasonic ranging
└─ mbed-os                  mbed OS dependency
```

## Hardware Interfaces

### Motors

The main program defines four motor objects:

- `BR(p21, p19, p18, 50)`
- `BL(p22, p17, p16, 50)`
- `FR(p23, p15, p14, 50)`
- `FL(p24, p13, p12, 50)`

Each `StdMotor` includes:

- 1 PWM output
- 2 direction logic outputs

Here, `50` means the PWM period is `50 us`.

### Serial Ports

- Bluetooth serial: `bt(p9, p10)`, baud rate `9600`
- Raspberry Pi serial: `rpi(p28, p27)`, baud rate `115200`
- PC debug serial: `pc(USBTX, USBRX)`

### Ultrasonic

- `trig`: `p29`
- `echo`: `p30`

## Software Architecture

### 1. Main Thread

The main thread continuously performs two tasks:

- Reads the ultrasonic distance and decides whether forward motion should be locked
- Retrieves serial commands from the mailbox and parses them

### 2. Input Thread

The project starts an additional `inputThread` for serial reception:

- The Bluetooth and Raspberry Pi serial ports trigger interrupts through `sigio`
- The interrupt handlers only set event flags instead of processing data directly
- The input thread reads serial characters after receiving an event
- After a full command line is received, it is wrapped into `CmdInfo` and pushed into `Mail<CmdInfo, 16>`

This design avoids complex logic inside interrupts and reduces coupling between serial reception and the main control flow.

### 3. Motion Control Layer

`MotionControl` is responsible for decomposing the desired motion into the speeds of four wheels:

- In button mode, a unit direction vector is synthesized from the four directional states `f/b/l/r`
- In joystick mode, the input `(v_x, v_y)` directly generates a unit direction vector
- The rotation term is represented by `w`, whose value is `-1 / 0 / 1`

The four-wheel speed distribution formula is:

```text
speedFL = V.y + V.x + W
speedFR = V.y - V.x - W
speedBL = V.y - V.x + W
speedBR = V.y + V.x - W
```

Where:

- `V = baseSpeed * unit(v_x, v_y)`
- `W = baseSpeed * w`

## Local Debugging

1. Clone the complete project files into the same directory
2. Ensure the `mbed-os` dependency is in place
3. Import the project into Mbed Studio
4. Compile and flash it to the target development board
5. Send commands through a serial tool or Bluetooth client for debugging

## Control Modes

The project supports two control modes:

- `BUTTON`: button mode, suitable for controlling direction through discrete commands
- `JOYSTICK`: joystick mode, suitable for controlling the velocity direction with a 2D vector

These modes can be switched dynamically via commands.

## Command Protocol

The project supports two types of commands:

- Single commands: start with `#`
- Multi-parameter commands: start with `:`

### Single Commands

Format example: `# U`

The meanings are:

- `U / u`: start moving forward / stop moving forward
- `D / d`: start moving backward / stop moving backward
- `L / l`: start moving left / stop moving left
- `R / r`: start moving right / stop moving right
- `B / b`: start counterclockwise rotation / stop rotation
- `E / e`: start clockwise rotation / stop rotation
- `S / s`: enable / disable ultrasonic obstacle-avoidance mode

Notes:

- Uppercase letters enable an action
- Lowercase letters cancel an action
- `B` and `E` do not stack rotation; repeated triggers switch or clear the state according to the implemented logic

### Multi-Parameter Commands

Format example: `: V 0.71 0.71`

#### 1. Velocity Vector Command

```text
: V v_x v_y
```

Meaning:

- Sends the velocity vector `v = (v_x, v_y)`
- Parameters are floating-point numbers
- Mainly used in joystick mode

Notes:

- The program normalizes the vector internally
- So the input mainly represents direction, while the actual speed magnitude is determined by the base speed `baseSpeed`

#### 2. Mode Switching Command

```text
: SM JS
: SM BT
```

Meaning:

- `SM JS`: switch to joystick mode
- `SM BT`: switch to button mode

#### 3. Base Speed Adjustment Command

```text
: SPD speed
```

Meaning:

- Adjusts the base speed level
- The parameter is an integer

Implementation details:

- The main program first converts the input into `5 + speed * 0.5`
- `MotionControl::setBaseSpeed()` then clamps it to `[5, 10]`
- The final internal `baseSpeed` range is `50 ~ 100`

This means:

- Very small `speed` values are clamped to the minimum speed
- Very large `speed` values are clamped to the maximum speed

#### 4. Servo Control Command

```text
: SVO UD deg
: SVO LR deg
```

Meaning:

- Adjusts the servo angle of the gimbal in the up/down or left/right direction
- `deg` is the angle value, with a recommended range of `-90` to `90`

Notes:

- The current mbed main program does not parse the command parameters directly
- The command is forwarded to the Raspberry Pi as-is

#### 5. Camera Control Command

```text
: CAM SHOT
: CAM REC
: CAM END
```

Meaning:

- `SHOT`: take a photo
- `REC`: start recording
- `END`: stop recording

Notes:

- This command is forwarded by mbed to the Raspberry Pi for processing

#### 6. AI Recognition Switch Command

```text
: AI 1
: AI 0
```

Meaning:

- Enable / disable person or object recognition

Notes:

- This command is forwarded by mbed to the Raspberry Pi for processing

#### 7. Target Tracking Command

```text
: TRK 1
: TRK 0
```

Meaning:

- Start / stop target tracking

Notes:

- This command is forwarded by mbed to the Raspberry Pi for processing

#### 8. Target Box Information Command

```text
: OBJ x_c y_c a b
```

Meaning:

- Describes the center position and bounding box size of the target object
- `x_c` and `y_c` indicate the target center coordinates
- `a` and `b` indicate the semi-axis lengths of the target box along the `x` and `y` directions

Notes:

- This branch was originally intended to implement target tracking by passing the target position and movement direction in the field of view into a PID-based automatic tracking flow, but it is not actually used

## Raspberry Pi Forwarding Mechanism

The following commands are directly forwarded by mbed to the Raspberry Pi:

- `SVO`, `CAM`, `TRK`, `AI`

mbed behaves as follows:

1. Returns a prompt message to the Bluetooth side, for example `Sent ": CAM SHOT"`
2. Sends the full command with a trailing newline to the Raspberry Pi serial port

Therefore, in the overall system, mbed acts both as the chassis controller and as the forwarding node for upper-layer vision commands.

## Ultrasonic Obstacle Avoidance Logic

The project provides a simple forward obstacle-avoidance function.

### Switching

Toggle it with the following command: `# S`

The Bluetooth side will receive similar feedback: `Ultrasonic Mode: 1`

### How It Works

The main loop continuously measures the distance and calculates: `distLimit = 15 * speedGrade - 70`

In the current code: `speedGrade = 7`, so `distLimit = 35`

When all of the following conditions are met, forward locking is enabled:

- Ultrasonic mode is enabled
- `distance < 35 cm`
- `distance > 2 cm`

After locking:

- Positive motor speed is forced to `0`
- Backward motion and other non-positive outputs can still execute

## Key Classes

### `MotionControl`

Responsibilities:

- Stores the current control mode
- Stores button states
- Stores the base speed
- Stores the rotation direction
- Updates the four-wheel speeds according to the target motion

Key interfaces:

- `changeMode(ControlMode)`
- `addDir(const Direction&)`
- `delDir(const Direction&)`
- `setRotation(int)`
- `setBaseSpeed(double)`
- `updateMotion(const Vector<double>&)`
- `stop()`

### `StdMotor`

Responsibilities:

- Controls a DC motor according to the speed value
- Uses PWM to control speed
- Uses two logic pins to control direction

Speed convention:

- `speed > 0`: forward
- `speed < 0`: reverse
- The valid range is limited to `[-100, 100]`

### `Ultrasonic`

Responsibilities:

- Obtains distance by measuring the trigger pulse and echo timing

Distance formula: `distance = echo_high_time_us * 0.017`

The returned value is generally interpreted in centimeters.

### `Servo`

Responsibilities:

- Maps angles to PWM pulse widths for servo control

Notes:

- Since the LPC1768 supports only one PWM hardware frequency and it is already occupied by the motors, this library is not used in the actual project; servo control is implemented on the Raspberry Pi side

## Debug Output

The program prints the following information over serial output for debugging:

- Startup success message: `Successfully started.`
- Received command content
- Current velocity vector: `VELOCITY: ...`
- Target speeds of the four wheels: `SPEED: ...`

These outputs are useful for troubleshooting command parsing, speed distribution, and obstacle-avoidance logic.
