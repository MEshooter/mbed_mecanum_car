#include "StdMotor.h"

StdMotor::StdMotor(PinName _m, PinName _l1, PinName _l2, int _p): 
    pwmSignal(_m), logic1(_l1), logic2(_l2), period(_p), speed(0) {
    pwmSignal.period_us(period);
}

void StdMotor::setSpeed(double _speed) {
    speed = std::max(std::min(_speed, 100.0), -100.0);
    int duty = std::abs(speed) / 100.0 * period;
    pwmSignal.pulsewidth_us(duty);
    
    if(speed < 0) {
        logic1 = 0;
        logic2 = 1;
    }
    else {
        logic1 = 1;
        logic2 = 0;
    }
}

int StdMotor::getPeriod(){
    return period;
}

double StdMotor::getSpeed(){
    return speed;
}
