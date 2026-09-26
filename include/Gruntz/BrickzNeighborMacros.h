#ifndef GRUNTZ_GRUNTZ_BRICKZNEIGHBORMACROS_H
#define GRUNTZ_GRUNTZ_BRICKZNEIGHBORMACROS_H

#define DECLARE_BRICKZ_NEIGHBORS(cell, x, y)                                                       \
    BrickzCell* up = NULL;                                                                         \
    BrickzCell* down = NULL;                                                                       \
    BrickzCell* right = NULL;                                                                      \
    BrickzCell* left = NULL;                                                                       \
    BrickzCell* ur = NULL;                                                                         \
    BrickzCell* ul = NULL;                                                                         \
    BrickzCell* dr = NULL;                                                                         \
    BrickzCell* dl = NULL;                                                                         \
    if ((y) > 0) {                                                                                 \
        up = (cell) - m_width;                                                                     \
    }                                                                                              \
    if (static_cast<u32>(y) < m_height - 1) {                                                      \
        down = (cell) + m_width;                                                                   \
    }                                                                                              \
    if (static_cast<u32>(x) < m_width - 1) {                                                       \
        right = (cell) + 1;                                                                        \
    }                                                                                              \
    if ((x) > 0) {                                                                                 \
        left = (cell) - 1;                                                                         \
    }                                                                                              \
    if (up && right) {                                                                             \
        ur = up + 1;                                                                               \
    }                                                                                              \
    if (up && left) {                                                                              \
        ul = up - 1;                                                                               \
    }                                                                                              \
    if (down && right) {                                                                           \
        dr = down + 1;                                                                             \
    }                                                                                              \
    if (down && left) {                                                                            \
        dl = down - 1;                                                                             \
    }

#define BRICKZ_OPPOSITE_NEIGHBORS_OPEN                                                             \
    ((up && down && !(up->m_flags & BRICKZ_BLOCKED_MASK)                                           \
      && !(down->m_flags & BRICKZ_BLOCKED_MASK))                                                   \
     || (right && left && !(right->m_flags & BRICKZ_BLOCKED_MASK)                                  \
         && !(left->m_flags & BRICKZ_BLOCKED_MASK))                                                \
     || (ur && dl && !(ur->m_flags & BRICKZ_BLOCKED_MASK) && !(dl->m_flags & BRICKZ_BLOCKED_MASK)) \
     || (ul && dr && !(ul->m_flags & BRICKZ_BLOCKED_MASK)                                          \
         && !(dr->m_flags & BRICKZ_BLOCKED_MASK)))

#endif // GRUNTZ_GRUNTZ_BRICKZNEIGHBORMACROS_H
