#ifndef GRUNTZ_GRUNTZ_COORDNODE_H
#define GRUNTZ_GRUNTZ_COORDNODE_H

#include <rva.h>
#include <Wap32/TileGeometry.h>

#include <math.h>

struct Coord {
    Coord() {}

    Coord(i32 x, i32 y) : m_x(x), m_y(y) {}

    i32 m_x;
    i32 m_y;

    i32 operator==(const Coord& other) const {
        if (m_x != other.m_x) {
            return 0;
        }
        return m_y == other.m_y;
    }

    i32 operator!=(const Coord& other) const {
        return !(*this == other);
    }

    Coord operator-() const {
        return Coord(-m_x, -m_y);
    }

    Coord operator-(const Coord& other) const {
        return Coord(m_x - other.m_x, m_y - other.m_y);
    }

    Coord operator+(const Coord& other) const {
        return Coord(m_x + other.m_x, m_y + other.m_y);
    }

    const Coord& operator+=(const Coord& other) {
        m_x += other.m_x;
        m_y += other.m_y;
        return *this;
    }

    const Coord& operator-=(const Coord& other) {
        m_x -= other.m_x;
        m_y -= other.m_y;
        return *this;
    }

    const Coord& operator*=(i32 scale) {
        m_x *= scale;
        m_y *= scale;
        return *this;
    }

    const Coord& operator/=(i32 scale) {
        m_x /= scale;
        m_y /= scale;
        return *this;
    }

    Coord operator*(i32 scale) const {
        return Coord(m_x * scale, m_y * scale);
    }

    Coord operator/(i32 scale) const {
        return Coord(m_x / scale, m_y / scale);
    }

    i32 Dot(const Coord& other) const {
        return m_x * other.m_x + m_y * other.m_y;
    }

    i32 LengthSqr() const {
        return Dot(*this);
    }

    i32 Length() const {
        return static_cast<i32>(sqrt(static_cast<double>(LengthSqr())));
    }

    i32 DistSqr(const Coord& other) const {
        return (*this - other).LengthSqr();
    }

    i32 Dist(const Coord& other) const {
        return (*this - other).Length();
    }

    Coord GetAbs() const {
        return Coord(abs(m_x), abs(m_y));
    }

    void Min(const Coord& other) {
        if (other.m_x < m_x) {
            m_x = other.m_x;
        }
        if (other.m_y < m_y) {
            m_y = other.m_y;
        }
    }

    void Max(const Coord& other) {
        if (other.m_x > m_x) {
            m_x = other.m_x;
        }
        if (other.m_y > m_y) {
            m_y = other.m_y;
        }
    }

    Coord GetMin(const Coord& other) const {
        Coord result = *this;
        result.Min(other);
        return result;
    }

    Coord GetMax(const Coord& other) const {
        Coord result = *this;
        result.Max(other);
        return result;
    }

    i32 MagSqr() const {
        return LengthSqr();
    }

    i32 Mag() const {
        return Length();
    }

    RVA(0x00075a10, 0x12)
    Coord* Set(i32 x, i32 y) {
        m_x = x;
        m_y = y;
        return &*this;
    }
};

struct CoordNode {
    CoordNode* m_next;
    CoordNode* m_prev;
    Coord* m_coord;
};

inline void ScreenTile(Coord* pos) {
    pos->m_x >>= TILE_SHIFT_PX;
    pos->m_y >>= TILE_SHIFT_PX;
}

inline void TileCenter(Coord* pos) {
    pos->m_x = (pos->m_x << TILE_SHIFT_PX) + TILE_HALF_PX;
    pos->m_y = (pos->m_y << TILE_SHIFT_PX) + TILE_HALF_PX;
}

inline void SnapTileCenter(Coord* pos) {
    pos->m_x = (pos->m_x & ~TILE_MASK_PX) + TILE_HALF_PX;
    pos->m_y = (pos->m_y & ~TILE_MASK_PX) + TILE_HALF_PX;
}

#endif // GRUNTZ_GRUNTZ_COORDNODE_H
