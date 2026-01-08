#include "StdMotor.h"

void StdMotor::setSpeed(double _speed) {
    // Normalize the speed value
    speed = std::max(std::min(_speed, 100.0), -100.0);

    if (forwardLock && speed > 0) {
        speed = 0;
    }

    // Convert the speed to the pulsewidth
    int duty = std::abs(speed) / 100.0 * period;
    
    if(speed < 0) {
        logic1 = 0;
        logic2 = 1;
    }
    else {
        logic1 = 1;
        logic2 = 0;
    }
    
    pwmSignal.pulsewidth_us(duty);
}

void StdMotor::setForwardLock(bool lock) {
    forwardLock = lock;
    if (forwardLock && speed > 0) {
        setSpeed(speed);
    }
}

int StdMotor::getPeriod(){
    return period;
}

double StdMotor::getSpeed(){
    return speed;
}

int StdMotor::getWidth(){
    return pwmSignal.read_pulsewidth_us();
}

void StdMotor::setPeriod(int p){
    period = p;
    pwmSignal.period_us(p);
    printf("!P: %d\n\r", pwmSignal.read_period_us());
    pwmSignal.pulsewidth_us(0);
}
