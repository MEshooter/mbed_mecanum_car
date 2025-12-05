#ifndef VECTOR_H_
#define VECTOR_H_

#include <iostream>
#include <cmath>

template<typename T>
class Vector {
public:
    T x, y;

    Vector() : x(0), y(0) {}
    Vector(const T& _x, const T& _y) : x(_x), y(_y) {}
    Vector(const Vector<T>& t) : x(t.x), y(t.y) {}

    Vector& operator += (const Vector<T>& t) {
        x += t.x;
        y += t.y;
        return *this;
    }

    Vector& operator -= (const Vector<T>& t) {
        x -= t.x;
        y -= t.y;
        return *this;
    }

    friend Vector operator + (const Vector& a, const Vector& b) {
        return Vector(a.x + b.x, a.y + b.y);
    }

    friend Vector operator - (const Vector& a, const Vector& b) {
        return Vector(a.x - b.x, a.y - b.y);
    }

    friend T operator * (const Vector& a, const Vector& b) {
        return a.x * b.x + a.y * b.y;
    }

    friend Vector operator * (const T& k, const Vector& v) {
        return Vector(v.x * k, v.y * k);
    }

    friend Vector operator * (const Vector& v, const T& k) {
        return Vector(v.x * k, v.y * k);
    }

    friend std::ostream& operator << (std::ostream& os, const Vector& v) {
        os << "(" << v.x << ", " << v.y << ")";
        return os;
    }

    T mag() const {
        return std::sqrt(x * x + y * y);
    }

    Vector unit() const {
        T m = mag();
        if (m < 1e-9 && m > -1e-9) {
            return Vector(0, 0); 
        }
        return (*this) * (1.0 / m);
    }
};

template<typename T>
T cross(const Vector<T>& a, const Vector<T>& b) {
    return a.x * b.y - a.y * b.x;
}

#endif