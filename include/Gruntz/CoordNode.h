#ifndef GRUNTZ_GRUNTZ_COORDNODE_H
#define GRUNTZ_GRUNTZ_COORDNODE_H

#include <rva.h>

#include <Wap32/TileGeometry.h>

#include <math.h>

struct Coord {
    i32 m_x;
    i32 m_y;

    RVA(0x00075a10, 0x12)
    Coord* Set(i32 x, i32 y) {
        m_x = x;
        m_y = y;
        return &*this;
    }

    void Clamp(const Coord& lower, const Coord& upper) {
        Max(lower);
        Min(upper);
    }

    i32 Dist(const Coord& other) const {
        return (*this - other).Length();
    }

    i32 DistSqr(const Coord& other) const {
        return (*this - other).LengthSqr();
    }

    i32 Dot(const Coord& other) const {
        return m_x * other.m_x + m_y * other.m_y;
    }

    Coord GetAbs() const {
        Coord result = {abs(m_x), abs(m_y)};
        return result;
    }

    Coord GetMax(const Coord& other) const {
        Coord result = *this;
        result.Max(other);
        return result;
    }

    Coord GetMin(const Coord& other) const {
        Coord result = *this;
        result.Min(other);
        return result;
    }

    i32 Length() const {
        return static_cast<i32>(sqrt(static_cast<double>(LengthSqr())));
    }

    i32 LengthSqr() const {
        return Dot(*this);
    }

    i32 Mag() const {
        return Length();
    }

    i32 MagSqr() const {
        return LengthSqr();
    }

    void Max(const Coord& other) {
        if (other.m_x > m_x) {
            m_x = other.m_x;
        }
        if (other.m_y > m_y) {
            m_y = other.m_y;
        }
    }

    void Min(const Coord& other) {
        if (other.m_x < m_x) {
            m_x = other.m_x;
        }
        if (other.m_y < m_y) {
            m_y = other.m_y;
        }
    }

    i32 NearlyEquals(const Coord& other, i32 radius) const {
        Coord delta = *this - other;
        return delta.Dot(delta) < radius * radius;
    }

    i32 operator!=(const Coord& other) const {
        return !(*this == other);
    }

    Coord operator*(i32 scale) const {
        Coord result = {m_x * scale, m_y * scale};
        return result;
    }

    friend Coord operator*(i32 scale, const Coord& value) {
        return value * scale;
    }

    const Coord& operator*=(i32 scale) {
        m_x *= scale;
        m_y *= scale;
        return *this;
    }

    Coord operator+(const Coord& other) const {
        Coord result = {m_x + other.m_x, m_y + other.m_y};
        return result;
    }

    const Coord& operator+=(const Coord& other) {
        m_x += other.m_x;
        m_y += other.m_y;
        return *this;
    }

    Coord operator-() const {
        Coord result = {-m_x, -m_y};
        return result;
    }

    Coord operator-(const Coord& other) const {
        Coord result = {m_x - other.m_x, m_y - other.m_y};
        return result;
    }

    const Coord& operator-=(const Coord& other) {
        m_x -= other.m_x;
        m_y -= other.m_y;
        return *this;
    }

    Coord operator/(i32 scale) const {
        Coord result = {m_x / scale, m_y / scale};
        return result;
    }

    const Coord& operator/=(i32 scale) {
        m_x /= scale;
        m_y /= scale;
        return *this;
    }

    i32 operator==(const Coord& other) const {
        if (m_x != other.m_x) {
            return 0;
        }
        return m_y == other.m_y;
    }

    const i32& operator[](i32 index) const {
        return *(&m_x + index);
    }
};

#define SCREEN_TILE_INPLACE(pos)                                                                   \
    (pos)->m_x >>= TILE_SHIFT_PX;                                                                  \
    (pos)->m_y >>= TILE_SHIFT_PX

#define COORD_EQUALS_COMPONENTS(coord, x, y) ((coord).m_x == (x) && (coord).m_y == (y))
#define SET_VECTOR2_COMPONENTS(coord, x, y)                                                        \
    (coord).m_x = (x);                                                                             \
    (coord).m_y = (y)

inline void SnapTileCenter(Coord* pos) {
    pos->m_x = (pos->m_x & ~TILE_MASK_PX) + TILE_HALF_PX;
    pos->m_y = (pos->m_y & ~TILE_MASK_PX) + TILE_HALF_PX;
}

inline void TileCenter(Coord* pos) {
    pos->m_x = (pos->m_x << TILE_SHIFT_PX) + TILE_HALF_PX;
    pos->m_y = (pos->m_y << TILE_SHIFT_PX) + TILE_HALF_PX;
}

#endif // GRUNTZ_GRUNTZ_COORDNODE_H
