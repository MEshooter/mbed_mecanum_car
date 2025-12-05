#ifndef __SIMPLEMOTOR_H
#define __SIMPLEMOTOR_H

#ifdef __ARM_FP
#undef  __ARM_FP
#endif

#include "mbed.h"

class SimpleMotor{
private:
    int speed;
    DigitalOut p1, p2;
public:
    SimpleMotor(PinName&, PinName&);
    void setSpeed(int);
    void refresh(int);
};

#endif