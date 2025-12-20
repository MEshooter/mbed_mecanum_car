#include "Servo.h"

int Servo::angle2Width(double val){
    return 500 + (val + 90.0) * 2000 / 180;
}

void Servo::setAngle(double val){
    angle = std::max(-90.0, std::min(90.0, val));
    signal.pulsewidth_us(angle2Width(angle));
}

double Servo::getAngle(){
    return angle;
}