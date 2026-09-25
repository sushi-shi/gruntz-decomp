#ifndef GRUNTZ_DOUBLEVECTOR_H
#define GRUNTZ_DOUBLEVECTOR_H

#include <rva.h>

struct DoubleVector2 {
    double m_x;
    double m_y;

    DoubleVector2() {}

    DoubleVector2(const double a, const double b) : m_x(a), m_y(b) {}

    void Init(const double a = 0.0, const double b = 0.0) {
        m_x = a;
        m_y = b;
    }

    const DoubleVector2 operator-(const DoubleVector2& other) const {
        return DoubleVector2(m_x - other.m_x, m_y - other.m_y);
    }
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

#define SET_VECTOR3_BOUNDS(lower, upper, minimum, maximum)                                         \
    (lower).m_x = (minimum);                                                                       \
    (upper).m_x = (maximum);                                                                       \
    (lower).m_y = (minimum);                                                                       \
    (upper).m_y = (maximum);                                                                       \
    (lower).m_z = (minimum);                                                                       \
    (upper).m_z = (maximum)
#define VECTOR2_MAG_COMPONENTS(x, y) sqrt((x) * (x) + (y) * (y))
#define VECTOR2_SCALE_TO_I32(outX, outY, x, y, scale)                                              \
    (outX) = static_cast<i32>((x) * (scale));                                                      \
    (outY) = static_cast<i32>((y) * (scale))
#define VECTOR_ADVANCE_COMPONENT(position, elapsed, velocity, scale)                               \
    (position) = (position) + static_cast<double>(elapsed) * (velocity) * (scale)
#define VECTOR_ADVANCE_VALUE(position, elapsed, firstScale, secondScale)                           \
    ((position) + ((elapsed) * (firstScale)) * (secondScale))
#define VECTOR_COMPONENT_ROUND_BIAS(out, value)                                                    \
    if ((value) > 0.0) {                                                                           \
        (out) = 0.5;                                                                               \
    } else if ((value) < 0.0) {                                                                    \
        (out) = -0.5;                                                                              \
    } else {                                                                                       \
        (out) = 0.0;                                                                               \
    }
#define VECTOR_SUBTRACT_SCALED_TO_I32(position, delta, scale)                                      \
    static_cast<i32>(static_cast<float>(position) - static_cast<float>(delta) * (scale))

#endif // GRUNTZ_DOUBLEVECTOR_H
