/**
 *  This library is designed for controlling the motor
 *  with two logic inputs and a PWM input.
 *  The rotating speed is proportional to the pulsewidth of the PWM signal.
 */

#ifndef STDMOTOR_H
#define STDMOTOR_H

#ifdef __ARM_FP
#undef  __ARM_FP
#endif

#include "mbed.h"

class StdMotor{
    PwmOut pwmSignal;
    DigitalOut logic1, logic2;
    // Defalut period: 50 us
    int period;
    // Speed range: -100 ~ 100
    // speed > 0: forward
    // speed < 0: backward
    double speed;
    bool forwardLock;
public:
    StdMotor(PinName _m, PinName _l1, PinName _l2, int _p): 
        pwmSignal(_m), logic1(_l1), logic2(_l2), period(_p), speed(0), forwardLock(false) {
        pwmSignal.period_us(period);
        forwardLock = 0;
    }
    void start();
    void setSpeed(double);
    void setPeriod(int);
    void setForwardLock(bool);
    int getPeriod();
    double getSpeed();
    int getWidth();
};

#endif