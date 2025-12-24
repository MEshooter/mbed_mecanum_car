/**
 * The library for controlling servos.
 * If some of the PWM pins on LPC1768 is be used for other components (motors),
 * this library should be banned because LPC1768 only supports ONE period for PWM outputs.
 */

#ifndef SERVO_H
#define SERVO_H

#ifdef __ARM_FP
#undef __ARM_FP
#endif

#include "mbed.h"

class Servo{
private:
    PwmOut signal;
    double angle;
public:
    Servo(PinName _signal): signal(_signal){
        signal.period_ms(20);
        angle = 0;
        signal.pulsewidth_us(angle2Width(angle));
    }
    int angle2Width(double);
    void setAngle(double);
    double getAngle();
};

#endif