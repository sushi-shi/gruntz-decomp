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

#define VEC2_SET(v, a, b)                                                                          \
    {                                                                                              \
        (v).m_x = (a);                                                                             \
        (v).m_y = (b);                                                                             \
    }

#define VEC3_SET(v, a, b, c)                                                                       \
    {                                                                                              \
        (v).m_x = (a);                                                                             \
        (v).m_y = (b);                                                                             \
        (v).m_z = (c);                                                                             \
    }

#endif // GRUNTZ_DOUBLEVECTOR_H
