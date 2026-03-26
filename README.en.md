[![中文](https://img.shields.io/badge/语言-中文-red)](./README.md)
[![English](https://img.shields.io/badge/Language-English-blue)](./README.en.md)

# 4WD Visual Car Based on Mbed and Raspberry Pi - Motion Control Module

## Project Overview

This is a 4WD omnidirectional car control project based on mbed OS. The mbed controller is responsible for:

- Receiving Bluetooth serial commands
- Receiving and forwarding Raspberry Pi serial commands
- Controlling 4 motors to achieve forward, backward, left, and right translation as well as in-place rotation
- Using an ultrasonic module for forward obstacle-avoidance protection

The included `mbed-os` directory is the runtime dependency. The main application logic of this project is located in:

- `main.cpp`
- `hardware_libs/`
- `common_libs/`

## Features

- Button-mode control for forward, backward, left, right, and rotation
- Joystick-mode control through a 2D velocity vector
- Adjustable base speed
- Forward obstacle avoidance using ultrasonic sensing
- Forwarding of high-level commands to the Raspberry Pi
- Reserved interfaces for gimbal servo control, vision recognition, and target tracking, which are intended to be implemented on the Raspberry Pi side

## Physical Showcase

<p align="center">
    <img src="https://github.com/user-attachments/assets/b6aaeaaa-1f19-4b45-aaf5-7ac2d83b1c72" alt="Car front-left view" width="360"/>
    <img src="https://github.com/user-attachments/assets/b936af82-56b9-4dd4-a999-b21042754ff6" alt="Car back-left view" width="360"/>
</p>

<p align="center">
    <img src="https://github.com/user-attachments/assets/5f4c9f4c-d136-490d-8f35-65b1cb6441a4" alt="Car front view" width="270"/>
</p>

## Directory Structure

```text
.
├─ main.cpp                 Program entry, serial I/O, command parsing, obstacle avoidance
├─ mbed_app.json            mbed configuration
├─ common_libs
│  └─ Vector.hpp            2D vector utility
├─ hardware_libs
│  ├─ MotionControl.*       Motion control and wheel speed distribution
│  ├─ StdMotor.*            Standard PWM motor driver
│  ├─ SimpleMotor.*         Simplified motor driver (not used in the current main program)
│  ├─ Servo.*               Servo control library (not used in the current main program)
│  └─ Ultrasonic.hpp        Ultrasonic distance measurement
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
- 2 digital outputs for direction control

Here `50` means the PWM period is `50 us`.

### Serial Ports

- Bluetooth serial: `bt(p9, p10)`, baud rate `9600`
- Raspberry Pi serial: `rpi(p28, p27)`, baud rate `115200`
- PC debug serial: `pc(USBTX, USBRX)`

### Ultrasonic Module

- `trig`: `p29`
- `echo`: `p30`

## Software Architecture

### 1. Main Thread

The main thread continuously performs two tasks:

- Reads the ultrasonic distance and decides whether forward motion should be locked
- Retrieves commands from the mailbox and parses them

### 2. Input Thread

The project starts an additional `inputThread` for serial input:

- The Bluetooth and Raspberry Pi serial ports trigger interrupts through `sigio`
- The interrupt handler only sets event flags and does not process data directly
- The input thread reads incoming serial characters after receiving the event
- When a full line is received, it is packaged into `CmdInfo` and pushed into `Mail<CmdInfo, 16>`

This design avoids doing heavy work in the interrupt handler and reduces coupling between serial reception and the main control flow.

### 3. Motion Control Layer

`MotionControl` is responsible for decomposing the desired motion into wheel speeds:

- In button mode, a unit direction vector is generated from the `f/b/l/r` state flags
- In joystick mode, the input `(v_x, v_y)` is directly converted into a unit direction vector
- The rotation component is represented by `w`, whose value is `-1 / 0 / 1`

The wheel speed distribution is:

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

1. Clone all project files into the same directory
2. Make sure the `mbed-os` dependency is in place
3. Import the project into Mbed Studio
4. Compile and flash it to the target board
5. Use a serial assistant or Bluetooth client to send commands for debugging

## Control Modes

The project supports two control modes:

- `BUTTON`: button mode, suitable for discrete directional commands
- `JOYSTICK`: joystick mode, suitable for controlling direction with a 2D vector

These modes can be switched dynamically through commands.

## Command Protocol

The project supports two categories of commands:

- Single commands beginning with `#`
- Multi-parameter commands beginning with `:`

### Single Commands

Example:

```text
# U
```

Meaning:

- `U / u`: start forward / stop forward
- `D / d`: start backward / stop backward
- `L / l`: start left shift / stop left shift
- `R / r`: start right shift / stop right shift
- `B / b`: start counterclockwise rotation / stop rotation
- `E / e`: start clockwise rotation / stop rotation
- `S / s`: enable / disable ultrasonic obstacle-avoidance mode

Notes:

- Uppercase means enabling an action
- Lowercase means canceling an action
- `B` and `E` do not accumulate rotation; repeated triggers follow the code logic to switch or clear the rotation state

### Multi-Parameter Commands

Example:

```text
: V 0.71 0.71
```

#### 1. Velocity Vector Command

```text
: V v_x v_y
```

Meaning:

- Sends a velocity vector `v = (v_x, v_y)`
- Parameters are floating-point values
- Mainly used in joystick mode

Notes:

- The vector is normalized internally
- So the input mainly represents direction, while the actual speed magnitude is determined by `baseSpeed`

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

- The main program first converts the input to `5 + speed * 0.5`
- `MotionControl::setBaseSpeed()` then clamps it to `[5, 10]`
- The final internal `baseSpeed` range is `50 ~ 100`

That means:

- Very small `speed` values are limited to the minimum speed
- Very large `speed` values are limited to the maximum speed

#### 4. Servo Control Command

```text
: SVO UD deg
: SVO LR deg
```

Meaning:

- Adjusts the gimbal servo angle in the up/down or left/right direction
- `deg` is the target angle, and the recommended range is `-90` to `90`

Notes:

- The current mbed main program does not parse these parameters directly
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

- Enable or disable human/object recognition

Notes:

- This command is forwarded by mbed to the Raspberry Pi for processing

#### 7. Target Tracking Command

```text
: TRK 1
: TRK 0
```

Meaning:

- Start or stop target tracking

Notes:

- This command is forwarded by mbed to the Raspberry Pi for processing

#### 8. Target Box Information Command

```text
: OBJ x_c y_c a b
```

Meaning:

- Describes the target center position and bounding box size
- `x_c` and `y_c` represent the center coordinates of the target
- `a` and `b` represent the semi-axis lengths of the target box in the `x` and `y` directions

Notes:

- This branch appears to have been intended for implementing target tracking by feeding target position and motion information into a PID controller
- In the current code, the actual handling logic is commented out and not used

## Raspberry Pi Forwarding Mechanism

The following commands are directly forwarded by mbed to the Raspberry Pi:

- `SVO`
- `CAM`
- `TRK`
- `AI`

The mbed side performs:

1. Sends a prompt back to Bluetooth, for example `Sent ": CAM SHOT"`
2. Sends the full command with a trailing newline to the Raspberry Pi serial port

So in the full system, mbed acts not only as the chassis controller, but also as the forwarding node for upper-layer vision commands.

## Ultrasonic Obstacle Avoidance Logic

The project provides a simple forward obstacle-avoidance function.

### Switching

Use the following command:

```text
# S
```

The Bluetooth side will receive feedback similar to:

```text
Ultrasonic Mode: 1
```

### How It Works

The main loop continuously measures distance and calculates:

```text
distLimit = 15 * speedGrade - 70
```

In the current code:

- `speedGrade = 7`
- Therefore `distLimit = 35`

When the following conditions are met, forward locking is enabled:

- Ultrasonic mode is enabled
- `distance < 35 cm`
- `distance > 2 cm`

After locking:

- Positive motor speed is forced to `0`
- Backward motion and other non-positive outputs can still run

## Key Class Descriptions

### `MotionControl`

Responsibilities:

- Stores the current control mode
- Stores button states
- Stores the base speed
- Stores the rotation direction
- Updates the wheel speeds according to the target motion

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

- Controls a DC motor according to a speed value
- Uses PWM to control speed
- Uses two logic pins to control direction

Speed convention:

- `speed > 0`: forward
- `speed < 0`: reverse
- Range limited to `[-100, 100]`

### `Ultrasonic`

Responsibilities:

- Obtains distance by measuring the echo pulse width

Distance formula:

```text
distance = echo_high_time_us * 0.017
```

The return value can generally be interpreted as centimeters.

### `Servo`

Responsibilities:

- Maps angles to PWM pulse widths for servo control

Notes:

- The source comment explicitly mentions the PWM resource limitation on LPC1768
- If the motors already occupy some PWM capability, the servo library should be used with caution

## Debug Output

The program prints the following information through serial output for debugging:

- Startup success message: `Successfully started.`
- Received command content
- Current velocity vector: `VELOCITY: ...`
- Target speed of the four wheels: `SPEED: ...`

These outputs are helpful for checking command parsing, speed distribution, and obstacle-avoidance behavior.
