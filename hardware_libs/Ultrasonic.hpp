#ifndef ULTRASONIC_H
#define ULTRASONIC_H

#ifdef __ARM_FP
#undef  __ARM_FP
#endif

#include "mbed.h"

class Ultrasonic {
    DigitalOut trig;
    InterruptIn echo;
    Timer timer;
    float distance;

public:
    Ultrasonic(PinName _trig, PinName _echo) : trig(_trig), echo(_echo) {
        distance = 999.0f;
        echo.rise(callback(this, &Ultrasonic::startTimer));
        echo.fall(callback(this, &Ultrasonic::stopTimer));
    }

    void startTimer() {
        timer.reset();
        timer.start();
    }

    void stopTimer() {
        timer.stop();
        auto us = timer.elapsed_time().count(); 
        distance = us * 0.017f; 
    }

    float getDistance() {
        trig = 0; wait_us(2);
        trig = 1; wait_us(10);
        trig = 0;
        wait_us(10000); 
        return distance;
    }
};

#endif