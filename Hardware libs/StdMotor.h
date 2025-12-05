#ifndef STDMOTOR_H
#define STDMOTOR_H

#ifdef __ARM_FP
#undef  __ARM_FP
#endif

#include "mbed.h"

class StdMotor{
    PwmOut pwmSignal;
    DigitalOut logic1, logic2;
    int period;
    double speed;
    // speed range: -100 ~ 100
public:
    StdMotor(PinName, PinName, PinName, int);
    void start();
    void setSpeed(double);
    int getPeriod();
    double getSpeed();
};

#endif