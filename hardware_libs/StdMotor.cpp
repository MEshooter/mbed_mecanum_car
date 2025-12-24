#include "StdMotor.h"

StdMotor::StdMotor(PinName _m, PinName _l1, PinName _l2, int _p): 
    pwmSignal(_m), logic1(_l1), logic2(_l2), period(_p), speed(0) {
    printf("P0: %d\n\r", period);
    pwmSignal.period_us(period);
    printf("P1: %d\n\r", pwmSignal.read_period_us());
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

    // printf("SPEED: %.4f, DUTY: %d\n\r", speed, duty);
    // printf("WIDTH: %d\n\r", pwmSignal.read_pulsewidth_us());
    // printf("PERCENT: %.2f\n\r", pwmSignal.read());
    // printf("PERIOD: %d\n\r", pwmSignal.read_period_us());

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
