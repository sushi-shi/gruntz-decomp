#ifndef GRUNTZ_DOUBLEVECTOR_H
#define GRUNTZ_DOUBLEVECTOR_H

#include <rva.h>

#include <Gruntz/CoordNode.h>

#include <math.h>

#define VECTOR2_MAG_COMPONENTS(x, y) sqrt((x) * (x) + (y) * (y))

struct FloatVector2 {
    FloatVector2() {}

    FloatVector2(float a, float b) : m_x(a), m_y(b) {}

    FloatVector2(const Coord& value)
        : m_x(static_cast<float>(value.m_x)), m_y(static_cast<float>(value.m_y)) {}

    void Init(float a = 0.0f, float b = 0.0f) {
        m_x = a;
        m_y = b;
    }

    void Init(const Coord& value) {
        m_x = static_cast<float>(value.m_x);
        m_y = static_cast<float>(value.m_y);
    }

    const float& operator[](i32 index) const {
        return *(&m_x + index);
    }

    float& operator[](i32 index) {
        return *(&m_x + index);
    }

    Coord ToCoord() const {
        Coord result = {static_cast<i32>(m_x), static_cast<i32>(m_y)};
        return result;
    }

    i32 operator==(const FloatVector2& other) const {
        return m_x == other.m_x && m_y == other.m_y;
    }

    i32 operator!=(const FloatVector2& other) const {
        return !(*this == other);
    }

    i32 NearlyEquals(const FloatVector2& other, float radius) const {
        FloatVector2 delta = *this - other;
        return delta.Dot(delta) < radius * radius;
    }

    FloatVector2 operator-() const {
        return FloatVector2(-m_x, -m_y);
    }

    const FloatVector2& operator+=(const FloatVector2& other) {
        m_x += other.m_x;
        m_y += other.m_y;
        return *this;
    }

    const FloatVector2& operator-=(const FloatVector2& other) {
        m_x -= other.m_x;
        m_y -= other.m_y;
        return *this;
    }

    const FloatVector2& operator*=(float scale) {
        m_x *= scale;
        m_y *= scale;
        return *this;
    }

    const FloatVector2& operator/=(float scale) {
        m_x /= scale;
        m_y /= scale;
        return *this;
    }

    FloatVector2 operator+(const FloatVector2& other) const {
        return FloatVector2(m_x + other.m_x, m_y + other.m_y);
    }

    FloatVector2 operator-(const FloatVector2& other) const {
        return FloatVector2(m_x - other.m_x, m_y - other.m_y);
    }

    FloatVector2 operator*(float scale) const {
        return FloatVector2(m_x * scale, m_y * scale);
    }

    friend FloatVector2 operator*(float scale, const FloatVector2& value) {
        return value * scale;
    }

    FloatVector2 operator/(float scale) const {
        return FloatVector2(m_x / scale, m_y / scale);
    }

    float Dot(const FloatVector2& other) const {
        return m_x * other.m_x + m_y * other.m_y;
    }

    float LengthSqr() const {
        return Dot(*this);
    }

    float Length() const {
        return static_cast<float>(sqrt(static_cast<double>(LengthSqr())));
    }

    float MagSqr() const {
        return LengthSqr();
    }

    float Mag() const {
        return Length();
    }

    float DistSqr(const FloatVector2& other) const {
        return (*this - other).LengthSqr();
    }

    float Dist(const FloatVector2& other) const {
        return (*this - other).Length();
    }

    FloatVector2 GetAbs() const {
        return FloatVector2(
            static_cast<float>(fabs(static_cast<double>(m_x))),
            static_cast<float>(fabs(static_cast<double>(m_y)))
        );
    }

    void Min(const FloatVector2& other) {
        m_x = m_x < other.m_x ? m_x : other.m_x;
        m_y = m_y < other.m_y ? m_y : other.m_y;
    }

    void Max(const FloatVector2& other) {
        m_x = m_x > other.m_x ? m_x : other.m_x;
        m_y = m_y > other.m_y ? m_y : other.m_y;
    }

    FloatVector2 GetMin(const FloatVector2& other) const {
        FloatVector2 result = *this;
        result.Min(other);
        return result;
    }

    FloatVector2 GetMax(const FloatVector2& other) const {
        FloatVector2 result = *this;
        result.Max(other);
        return result;
    }

    FloatVector2 Unit() const {
        return *this / Length();
    }

    const FloatVector2& Normalize() {
        return *this /= Length();
    }

    float m_x;
    float m_y;
};

struct DoubleVector2 {
    DoubleVector2() {}

    DoubleVector2(double a, double b) : m_x(a), m_y(b) {}

    DoubleVector2(const Coord& value)
        : m_x(static_cast<double>(value.m_x)), m_y(static_cast<double>(value.m_y)) {}

    void Init(double a = 0.0, double b = 0.0) {
        m_x = a;
        m_y = b;
    }

    void Init(const Coord& value) {
        m_x = static_cast<double>(value.m_x);
        m_y = static_cast<double>(value.m_y);
    }

    const double& operator[](i32 index) const {
        return *(&m_x + index);
    }

    double& operator[](i32 index) {
        return *(&m_x + index);
    }

    Coord ToCoord() const {
        Coord result = {static_cast<i32>(m_x), static_cast<i32>(m_y)};
        return result;
    }

    i32 operator==(const DoubleVector2& other) const {
        return m_x == other.m_x && m_y == other.m_y;
    }

    i32 operator!=(const DoubleVector2& other) const {
        return !(*this == other);
    }

    i32 NearlyEquals(const DoubleVector2& other, double radius) const {
        DoubleVector2 delta = *this - other;
        return delta.Dot(delta) < radius * radius;
    }

    DoubleVector2 operator-() const {
        return DoubleVector2(-m_x, -m_y);
    }

    const DoubleVector2& operator+=(const DoubleVector2& other) {
        m_x += other.m_x;
        m_y += other.m_y;
        return *this;
    }

    const DoubleVector2& operator-=(const DoubleVector2& other) {
        m_x -= other.m_x;
        m_y -= other.m_y;
        return *this;
    }

    const DoubleVector2& operator*=(double scale) {
        m_x *= scale;
        m_y *= scale;
        return *this;
    }

    const DoubleVector2& operator/=(double scale) {
        m_x /= scale;
        m_y /= scale;
        return *this;
    }

    DoubleVector2 operator+(const DoubleVector2& other) const {
        return DoubleVector2(m_x + other.m_x, m_y + other.m_y);
    }

    DoubleVector2 operator-(const DoubleVector2& other) const {
        return DoubleVector2(m_x - other.m_x, m_y - other.m_y);
    }

    DoubleVector2 operator*(double scale) const {
        return DoubleVector2(m_x * scale, m_y * scale);
    }

    friend DoubleVector2 operator*(double scale, const DoubleVector2& value) {
        return value * scale;
    }

    DoubleVector2 operator/(double scale) const {
        return DoubleVector2(m_x / scale, m_y / scale);
    }

    double Dot(const DoubleVector2& other) const {
        return m_x * other.m_x + m_y * other.m_y;
    }

    double LengthSqr() const {
        return Dot(*this);
    }

    double Length() const {
        return sqrt(LengthSqr());
    }

    double MagSqr() const {
        return LengthSqr();
    }

    double Mag() const {
        return Length();
    }

    double DistSqr(const DoubleVector2& other) const {
        return (*this - other).LengthSqr();
    }

    double Dist(const DoubleVector2& other) const {
        return (*this - other).Length();
    }

    DoubleVector2 GetAbs() const {
        return DoubleVector2(fabs(m_x), fabs(m_y));
    }

    void Min(const DoubleVector2& other) {
        m_x = m_x < other.m_x ? m_x : other.m_x;
        m_y = m_y < other.m_y ? m_y : other.m_y;
    }

    void Max(const DoubleVector2& other) {
        m_x = m_x > other.m_x ? m_x : other.m_x;
        m_y = m_y > other.m_y ? m_y : other.m_y;
    }

    DoubleVector2 GetMin(const DoubleVector2& other) const {
        DoubleVector2 result = *this;
        result.Min(other);
        return result;
    }

    DoubleVector2 GetMax(const DoubleVector2& other) const {
        DoubleVector2 result = *this;
        result.Max(other);
        return result;
    }

    DoubleVector2 Unit() const {
        return *this / Length();
    }

    const DoubleVector2& Normalize() {
        return *this /= Length();
    }

    double m_x;
    double m_y;
};

struct DoubleVector3 {
    void SetXY(const Coord& value) {
        m_x = static_cast<double>(value.m_x);
        m_y = static_cast<double>(value.m_y);
    }

    Coord ToCoord() const {
        Coord result = {static_cast<i32>(m_x), static_cast<i32>(m_y)};
        return result;
    }

    void Init(double a = 0.0, double b = 0.0, double c = 0.0) {
        m_x = a;
        m_y = b;
        m_z = c;
    }

    const double& operator[](i32 index) const {
        return *(&m_x + index);
    }

    double& operator[](i32 index) {
        return *(&m_x + index);
    }

    double Dot(const DoubleVector3& other) const {
        return m_x * other.m_x + m_y * other.m_y + m_z * other.m_z;
    }

    double MagSqr() const {
        return Dot(*this);
    }

    double Mag() const {
        return sqrt(MagSqr());
    }

    double MagApprox() const {
        double min;
        double med;
        double max;
        double temp;

        max = fabs(m_x);
        med = fabs(m_y);
        min = fabs(m_z);

        if (max < med) {
            temp = max;
            max = med;
            med = temp;
        }
        if (max < min) {
            temp = max;
            max = min;
            min = temp;
        }

        return max + (med + min) * 0.25f;
    }

    void Norm(double value = 1.0) {
        double mag = Mag();
        if (mag == 0.0) {
            return;
        }

        double inverse = value / mag;
        m_x = m_x * inverse;
        m_y = m_y * inverse;
        m_z = m_z * inverse;
    }

    void NormApprox(double value = 1.0) {
        double mag = MagApprox();
        if (mag == 0.0) {
            return;
        }

        double inverse = value / mag;
        m_x = m_x * inverse;
        m_y = m_y * inverse;
        m_z = m_z * inverse;
    }

    double LengthSqr() const {
        return MagSqr();
    }

    double LengthSquared() const {
        return LengthSqr();
    }

    double Length() const {
        return Mag();
    }

    DoubleVector3 Unit() const {
        return *this / Length();
    }

    DoubleVector3 GetNormalized() const {
        return *this / Mag();
    }

    const DoubleVector3& Normalize() {
        return *this /= Length();
    }

    DoubleVector3 Cross(const DoubleVector3& other) const {
        DoubleVector3 result = {
            other.m_y * m_z - other.m_z * m_y,
            other.m_z * m_x - other.m_x * m_z,
            other.m_x * m_y - other.m_y * m_x
        };
        return result;
    }

    i32 operator==(const DoubleVector3& other) const {
        return m_x == other.m_x && m_y == other.m_y && m_z == other.m_z;
    }

    i32 operator!=(const DoubleVector3& other) const {
        return !(*this == other);
    }

    i32 NearlyEquals(const DoubleVector3& other, double radius = 0.0) const {
        DoubleVector3 delta = *this - other;
        return delta.Dot(delta) <= radius * radius;
    }

    i32 operator>(const DoubleVector3& other) const {
        return m_x > other.m_x && m_y > other.m_y && m_z > other.m_z;
    }

    i32 operator<(const DoubleVector3& other) const {
        return m_x < other.m_x && m_y < other.m_y && m_z < other.m_z;
    }

    i32 operator>=(const DoubleVector3& other) const {
        return m_x >= other.m_x && m_y >= other.m_y && m_z >= other.m_z;
    }

    i32 operator<=(const DoubleVector3& other) const {
        return m_x <= other.m_x && m_y <= other.m_y && m_z <= other.m_z;
    }

    DoubleVector3 operator-() const {
        DoubleVector3 result = {-m_x, -m_y, -m_z};
        return result;
    }

    const DoubleVector3& operator=(const DoubleVector3& other) {
        m_x = other.m_x;
        m_y = other.m_y;
        m_z = other.m_z;
        return *this;
    }

    const DoubleVector3& operator+=(const DoubleVector3& other) {
        m_x += other.m_x;
        m_y += other.m_y;
        m_z += other.m_z;
        return *this;
    }

    const DoubleVector3& operator-=(const DoubleVector3& other) {
        m_x -= other.m_x;
        m_y -= other.m_y;
        m_z -= other.m_z;
        return *this;
    }

    void operator+=(double scalar) {
        m_x += scalar;
        m_y += scalar;
        m_z += scalar;
    }

    void operator-=(double scalar) {
        m_x -= scalar;
        m_y -= scalar;
        m_z -= scalar;
    }

    const DoubleVector3& operator*=(double scale) {
        m_x *= scale;
        m_y *= scale;
        m_z *= scale;
        return *this;
    }

    const DoubleVector3& operator/=(double scale) {
        double inverse = 1.0 / scale;
        m_x *= inverse;
        m_y *= inverse;
        m_z *= inverse;
        return *this;
    }

    DoubleVector3 operator+(const DoubleVector3& other) const {
        DoubleVector3 result = {m_x + other.m_x, m_y + other.m_y, m_z + other.m_z};
        return result;
    }

    DoubleVector3 operator-(const DoubleVector3& other) const {
        DoubleVector3 result = {m_x - other.m_x, m_y - other.m_y, m_z - other.m_z};
        return result;
    }

    DoubleVector3 operator*(double scale) const {
        DoubleVector3 result = {m_x * scale, m_y * scale, m_z * scale};
        return result;
    }

    friend DoubleVector3 operator*(double scale, const DoubleVector3& value) {
        return value * scale;
    }

    DoubleVector3 operator*(const DoubleVector3& other) const {
        DoubleVector3 result = {m_x * other.m_x, m_y * other.m_y, m_z * other.m_z};
        return result;
    }

    DoubleVector3 operator/(double scale) const {
        double inverse = 1.0 / scale;
        DoubleVector3 result = {m_x * inverse, m_y * inverse, m_z * inverse};
        return result;
    }

    DoubleVector3 operator/(const DoubleVector3& other) const {
        DoubleVector3 result = {m_x / other.m_x, m_y / other.m_y, m_z / other.m_z};
        return result;
    }

    double Dist(const DoubleVector3& other) const {
        return (*this - other).Mag();
    }

    double DistSqr(const DoubleVector3& other) const {
        return (*this - other).MagSqr();
    }

    double m_x;
    double m_y;
    double m_z;
};

inline double PixelRoundBias(double direction) {
    if (direction > 0.0) {
        return 0.5;
    }
    if (direction < 0.0) {
        return -0.5;
    }
    return 0.0;
}

inline DoubleVector2 PixelRoundBias(const DoubleVector2& direction) {
    return DoubleVector2(PixelRoundBias(direction.m_x), PixelRoundBias(direction.m_y));
}

#define VECTOR_COMPONENT_ROUND_BIAS(out, value)                                                    \
    if ((value) > 0.0) {                                                                           \
        (out) = 0.5;                                                                               \
    } else if ((value) < 0.0) {                                                                    \
        (out) = -0.5;                                                                              \
    } else {                                                                                       \
        (out) = 0.0;                                                                               \
    }

#define VEC2_SET(vector, x, y)                                                                     \
    (vector).m_x = (x);                                                                            \
    (vector).m_y = (y)

#define VECTOR2_SCALE_TO_I32(outX, outY, x, y, scale)                                              \
    (outX) = static_cast<i32>((x) * (scale));                                                      \
    (outY) = static_cast<i32>((y) * (scale))

#define VECTOR_ADVANCE_COMPONENT(position, elapsed, velocity, scale)                               \
    (position) = (position) + static_cast<double>(elapsed) * (velocity) * (scale)

#define VECTOR_ADVANCE_VALUE(position, elapsed, firstScale, secondScale)                           \
    ((position) + ((elapsed) * (firstScale)) * (secondScale))

#define VECTOR_SUBTRACT_SCALED_TO_I32(position, delta, scale)                                      \
    static_cast<i32>(static_cast<float>(position) - static_cast<float>(delta) * (scale))

#define SET_VECTOR3_BOUNDS(lower, upper, minimum, maximum)                                         \
    (lower).m_x = (minimum);                                                                       \
    (upper).m_x = (maximum);                                                                       \
    (lower).m_y = (minimum);                                                                       \
    (upper).m_y = (maximum);                                                                       \
    (lower).m_z = (minimum);                                                                       \
    (upper).m_z = (maximum)
#endif // GRUNTZ_DOUBLEVECTOR_H
