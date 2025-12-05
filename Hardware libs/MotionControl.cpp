#include "MotionControl.h"

Direction::Direction(bool _f, bool _b, bool _l, bool _r):
    f(_f), b(_b), l(_l), r(_r) {}

Direction operator | (const Direction& a, const Direction& b) {
    Direction res = a;
    res |= b;
    return res;
}

Direction operator & (const Direction& a, const Direction& b) {
    Direction res = a;
    res &= b;
    return res;
}

Vector<double> dir2Vector(const Direction &dir) {
    double v_x = 0, v_y = 0;
    if(dir.f) v_y += 1;
    if(dir.b) v_y -= 1;
    if(dir.l) v_x -= 1;
    if(dir.r) v_x += 1;
    Vector<double> res(v_x, v_y);
    return res.unit();
}

MotionControl::MotionControl(StdMotor& _FL, StdMotor& _FR, StdMotor& _BL, StdMotor& _BR):
    FL(_FL), FR(_FR), BL(_BL), BR(_BR) {
    baseSpeed = 50;
    dir = Direction();
    mode = BUTTON;
    v = {0, 0};
    w = 0;
}

ControlMode MotionControl::getMode() {
    return mode;
}

void MotionControl::changeMode(ControlMode _mode) {
    mode = _mode;
}

void MotionControl::addDir(const Direction &_d) {
    dir |= _d;
}

void MotionControl::delDir(const Direction &_d) {
    dir &= _d;
}

void MotionControl::setRotation(const int& _w) {
    if(w != 0 && _w != 0) w = 0;
    else w = _w;
    updateMotion(v);
}

void MotionControl::setBaseSpeed(const int& speed) {
    baseSpeed = std::min(std::max(3, speed), 10) * 10;
}

void MotionControl::updateMotion(const Vector<double>& _v) {
    if(mode == BUTTON) v = dir2Vector(dir);
    else v = _v.unit();

    Vector<double> V = v * (double)baseSpeed;
    double W = baseSpeed * w;

    char prt[64];
    sprintf(prt, "VELOCITY: %.2f %.2f %.2f\n", V.x, V.y, W);
    printf("%s", prt);
    fflush(stdout);
    
    double speedFL = V.y + V.x + W;
    double speedFR = V.y - V.x - W;
    double speedBL = V.y - V.x + W;
    double speedBR = V.y + V.x - W;

    sprintf(prt, "SPEED: %.2f %.2f %.2f %.2f\n", speedFL, speedFR, speedBL, speedBR);
    printf("%s", prt);
    fflush(stdout);

    FL.setSpeed(speedFL);
    FR.setSpeed(speedFR);
    BL.setSpeed(speedBL);
    BR.setSpeed(speedBR);
}

void MotionControl::stop() {
    w = 0;
    dir = {0, 0, 0, 0};
    updateMotion(Vector<double>());
}
