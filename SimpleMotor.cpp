#include "SimpleMotor.h"

SimpleMotor::SimpleMotor(PinName &_p1, PinName &_p2): p1(_p1), p2(_p2) {
    speed = 0;
}

void SimpleMotor::setSpeed(int level) {
    speed = level;
}

void SimpleMotor::refresh(int cnt) {
    int duty = abs(speed) * 100 / 10;
    if(cnt >= duty) p1 = p2 = 0;
    else {
        if(speed > 0) {
            p1 = cnt < duty ? 1 : 0;
            p2 = 0;
        }
        else {
            p1 = 0;
            p2 = cnt < duty ? 1 : 0;
        }
    }
}
