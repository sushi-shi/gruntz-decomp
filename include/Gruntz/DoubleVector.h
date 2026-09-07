#ifndef GRUNTZ_DOUBLEVECTOR_H
#define GRUNTZ_DOUBLEVECTOR_H

#include <rva.h>

struct DoubleVector2 {
    void Init(double a = 0.0, double b = 0.0) {
        m_x = a;
        m_y = b;
    }

    double m_x;
    double m_y;
};

struct DoubleVector3 {
    void Init(double a = 0.0, double b = 0.0, double c = 0.0) {
        m_x = a;
        m_y = b;
        m_z = c;
    }

    double m_x;
    double m_y;
    double m_z;
};

#endif // GRUNTZ_DOUBLEVECTOR_H
