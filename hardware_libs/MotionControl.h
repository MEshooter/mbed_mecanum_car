/**  
 * The library is designed for motion controlling of the vehicle.
 * BUTTON mode allows the car to move in four directions.
 * JOYSTICK mode allows the car to move in all directions.
 * The moving direction of the car is based on a velocity vector, 
 * and the magnitude of the velocity is based on the variable baseSpeed.
 */

#ifndef MOTIONCONTROL_H
#define MOTIONCONTROL_H

#ifdef __ARM_FP
#undef  __ARM_FP
#endif

#include "mbed.h"
#include "StdMotor.h"
#include "Vector.hpp"

enum ControlMode{
    BUTTON, JOYSTICK
};

// Struct Direction is for the direction control in BUTTON mode
struct Direction{
    bool f, b, l, r;
    Direction(bool _f = 0, bool _b = 0, bool _l = 0, bool _r = 0);
    Direction& operator |= (const Direction &t){
        f |= t.f; b |= t.b; l |= t.l; r |= t.r;
        return *this;
    }
    Direction& operator &= (const Direction &t){
        f &= t.f; b &= t.b; l &= t.l; r &= t.r;
        return *this;
    }
    friend Direction operator | (const Direction&, const Direction&);
    friend Direction operator & (const Direction&, const Direction&);
};

Vector<double> dir2Vector(const Direction&);

class MotionControl{
private:
    StdMotor &FL, &FR, &BL, &BR;
    int baseSpeed, w; // +1: cw -1: ccw
    ControlMode mode;
    Direction dir;
    Vector<double> v;
public:
    MotionControl(StdMotor&, StdMotor&, StdMotor&, StdMotor&);
    ControlMode getMode();
    void changeMode(ControlMode);
    void addDir(const Direction&);
    void delDir(const Direction&);
    void setRotation(const int&);
    void setBaseSpeed(const int&);
    void updateMotion(const Vector<double>& _v = Vector<double>());
    void stop();
};

#endif