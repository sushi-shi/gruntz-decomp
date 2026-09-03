#ifndef GRUNTZ_DOUBLEVECTOR_H
#define GRUNTZ_DOUBLEVECTOR_H

#include <rva.h>

#include <Gruntz/CoordNode.h>

#include <math.h>

struct FloatVector2 {
    FloatVector2() {}

    FloatVector2(float a, float b) : x(a), y(b) {}

    FloatVector2(const Coord& value)
        : x(static_cast<float>(value.m_x)), y(static_cast<float>(value.m_y)) {}

    void Init(float a = 0.0f, float b = 0.0f) {
        x = a;
        y = b;
    }

    void Init(const Coord& value) {
        x = static_cast<float>(value.m_x);
        y = static_cast<float>(value.m_y);
    }

    const float& operator[](i32 index) const {
        return *(&x + index);
    }

    float& operator[](i32 index) {
        return *(&x + index);
    }

    Coord ToCoord() const {
        return Coord(static_cast<i32>(x), static_cast<i32>(y));
    }

    i32 operator==(const FloatVector2& other) const {
        return x == other.x && y == other.y;
    }

    i32 operator!=(const FloatVector2& other) const {
        return !(*this == other);
    }

    i32 NearlyEquals(const FloatVector2& other, float radius) const {
        FloatVector2 delta = *this - other;
        return delta.Dot(delta) < radius * radius;
    }

    FloatVector2 operator-() const {
        return FloatVector2(-x, -y);
    }

    const FloatVector2& operator+=(const FloatVector2& other) {
        x += other.x;
        y += other.y;
        return *this;
    }

    const FloatVector2& operator-=(const FloatVector2& other) {
        x -= other.x;
        y -= other.y;
        return *this;
    }

    const FloatVector2& operator*=(float scale) {
        x *= scale;
        y *= scale;
        return *this;
    }

    const FloatVector2& operator/=(float scale) {
        x /= scale;
        y /= scale;
        return *this;
    }

    FloatVector2 operator+(const FloatVector2& other) const {
        return FloatVector2(x + other.x, y + other.y);
    }

    FloatVector2 operator-(const FloatVector2& other) const {
        return FloatVector2(x - other.x, y - other.y);
    }

    FloatVector2 operator*(float scale) const {
        return FloatVector2(x * scale, y * scale);
    }

    friend FloatVector2 operator*(float scale, const FloatVector2& value) {
        return value * scale;
    }

    FloatVector2 operator/(float scale) const {
        return FloatVector2(x / scale, y / scale);
    }

    float Dot(const FloatVector2& other) const {
        return x * other.x + y * other.y;
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
            static_cast<float>(fabs(static_cast<double>(x))),
            static_cast<float>(fabs(static_cast<double>(y)))
        );
    }

    void Min(const FloatVector2& other) {
        x = x < other.x ? x : other.x;
        y = y < other.y ? y : other.y;
    }

    void Max(const FloatVector2& other) {
        x = x > other.x ? x : other.x;
        y = y > other.y ? y : other.y;
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

    float x;
    float y;
};

struct DoubleVector2 {
    DoubleVector2() {}

    DoubleVector2(double a, double b) : x(a), y(b) {}

    DoubleVector2(const Coord& value)
        : x(static_cast<double>(value.m_x)), y(static_cast<double>(value.m_y)) {}

    void Init(double a = 0.0, double b = 0.0) {
        x = a;
        y = b;
    }

    void Init(const Coord& value) {
        x = static_cast<double>(value.m_x);
        y = static_cast<double>(value.m_y);
    }

    const double& operator[](i32 index) const {
        return *(&x + index);
    }

    double& operator[](i32 index) {
        return *(&x + index);
    }

    Coord ToCoord() const {
        return Coord(static_cast<i32>(x), static_cast<i32>(y));
    }

    i32 operator==(const DoubleVector2& other) const {
        return x == other.x && y == other.y;
    }

    i32 operator!=(const DoubleVector2& other) const {
        return !(*this == other);
    }

    i32 NearlyEquals(const DoubleVector2& other, double radius) const {
        DoubleVector2 delta = *this - other;
        return delta.Dot(delta) < radius * radius;
    }

    DoubleVector2 operator-() const {
        return DoubleVector2(-x, -y);
    }

    const DoubleVector2& operator+=(const DoubleVector2& other) {
        x += other.x;
        y += other.y;
        return *this;
    }

    const DoubleVector2& operator-=(const DoubleVector2& other) {
        x -= other.x;
        y -= other.y;
        return *this;
    }

    const DoubleVector2& operator*=(double scale) {
        x *= scale;
        y *= scale;
        return *this;
    }

    const DoubleVector2& operator/=(double scale) {
        x /= scale;
        y /= scale;
        return *this;
    }

    DoubleVector2 operator+(const DoubleVector2& other) const {
        return DoubleVector2(x + other.x, y + other.y);
    }

    DoubleVector2 operator-(const DoubleVector2& other) const {
        return DoubleVector2(x - other.x, y - other.y);
    }

    DoubleVector2 operator*(double scale) const {
        return DoubleVector2(x * scale, y * scale);
    }

    friend DoubleVector2 operator*(double scale, const DoubleVector2& value) {
        return value * scale;
    }

    DoubleVector2 operator/(double scale) const {
        return DoubleVector2(x / scale, y / scale);
    }

    double Dot(const DoubleVector2& other) const {
        return x * other.x + y * other.y;
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
        return DoubleVector2(fabs(x), fabs(y));
    }

    void Min(const DoubleVector2& other) {
        x = x < other.x ? x : other.x;
        y = y < other.y ? y : other.y;
    }

    void Max(const DoubleVector2& other) {
        x = x > other.x ? x : other.x;
        y = y > other.y ? y : other.y;
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

    double x;
    double y;
};

struct DoubleVector3 {
    DoubleVector3() {}

    DoubleVector3(double a, double b, double c) : x(a), y(b), z(c) {}

    void SetXY(const Coord& value) {
        x = static_cast<double>(value.m_x);
        y = static_cast<double>(value.m_y);
    }

    Coord ToCoord() const {
        return Coord(static_cast<i32>(x), static_cast<i32>(y));
    }

    void Init(double a = 0.0, double b = 0.0, double c = 0.0) {
        x = a;
        y = b;
        z = c;
    }

    const double& operator[](i32 index) const {
        return *(&x + index);
    }

    double& operator[](i32 index) {
        return *(&x + index);
    }

    double Dot(const DoubleVector3& other) const {
        return x * other.x + y * other.y + z * other.z;
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

        max = fabs(x);
        med = fabs(y);
        min = fabs(z);

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
        x = x * inverse;
        y = y * inverse;
        z = z * inverse;
    }

    void NormApprox(double value = 1.0) {
        double mag = MagApprox();
        if (mag == 0.0) {
            return;
        }

        double inverse = value / mag;
        x = x * inverse;
        y = y * inverse;
        z = z * inverse;
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
        return DoubleVector3(
            other.y * z - other.z * y,
            other.z * x - other.x * z,
            other.x * y - other.y * x
        );
    }

    i32 operator==(const DoubleVector3& other) const {
        return x == other.x && y == other.y && z == other.z;
    }

    i32 operator!=(const DoubleVector3& other) const {
        return !(*this == other);
    }

    i32 NearlyEquals(const DoubleVector3& other, double radius = 0.0) const {
        DoubleVector3 delta = *this - other;
        return delta.Dot(delta) <= radius * radius;
    }

    i32 operator>(const DoubleVector3& other) const {
        return x > other.x && y > other.y && z > other.z;
    }

    i32 operator<(const DoubleVector3& other) const {
        return x < other.x && y < other.y && z < other.z;
    }

    i32 operator>=(const DoubleVector3& other) const {
        return x >= other.x && y >= other.y && z >= other.z;
    }

    i32 operator<=(const DoubleVector3& other) const {
        return x <= other.x && y <= other.y && z <= other.z;
    }

    DoubleVector3 operator-() const {
        return DoubleVector3(-x, -y, -z);
    }

    const DoubleVector3& operator=(const DoubleVector3& other) {
        x = other.x;
        y = other.y;
        z = other.z;
        return *this;
    }

    const DoubleVector3& operator+=(const DoubleVector3& other) {
        x += other.x;
        y += other.y;
        z += other.z;
        return *this;
    }

    const DoubleVector3& operator-=(const DoubleVector3& other) {
        x -= other.x;
        y -= other.y;
        z -= other.z;
        return *this;
    }

    void operator+=(double scalar) {
        x += scalar;
        y += scalar;
        z += scalar;
    }

    void operator-=(double scalar) {
        x -= scalar;
        y -= scalar;
        z -= scalar;
    }

    const DoubleVector3& operator*=(double scale) {
        x *= scale;
        y *= scale;
        z *= scale;
        return *this;
    }

    const DoubleVector3& operator/=(double scale) {
        double inverse = 1.0 / scale;
        x *= inverse;
        y *= inverse;
        z *= inverse;
        return *this;
    }

    DoubleVector3 operator+(const DoubleVector3& other) const {
        return DoubleVector3(x + other.x, y + other.y, z + other.z);
    }

    DoubleVector3 operator-(const DoubleVector3& other) const {
        return DoubleVector3(x - other.x, y - other.y, z - other.z);
    }

    DoubleVector3 operator*(double scale) const {
        return DoubleVector3(x * scale, y * scale, z * scale);
    }

    friend DoubleVector3 operator*(double scale, const DoubleVector3& value) {
        return value * scale;
    }

    DoubleVector3 operator*(const DoubleVector3& other) const {
        return DoubleVector3(x * other.x, y * other.y, z * other.z);
    }

    DoubleVector3 operator/(double scale) const {
        double inverse = 1.0 / scale;
        return DoubleVector3(x * inverse, y * inverse, z * inverse);
    }

    DoubleVector3 operator/(const DoubleVector3& other) const {
        return DoubleVector3(x / other.x, y / other.y, z / other.z);
    }

    double Dist(const DoubleVector3& other) const {
        return (*this - other).Mag();
    }

    double DistSqr(const DoubleVector3& other) const {
        return (*this - other).MagSqr();
    }

    double x;
    double y;
    double z;
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
    return DoubleVector2(PixelRoundBias(direction.x), PixelRoundBias(direction.y));
}

#endif // GRUNTZ_DOUBLEVECTOR_H
