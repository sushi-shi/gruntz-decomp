#ifndef GRUNTZ_GRUNTRANDOMPOINTMACROS_H
#define GRUNTZ_GRUNTRANDOMPOINTMACROS_H

// Pick a random point inside an object's extent rect.  Six spellings of this
// snippet existed for seven sites; folding each into the one below and reading
// a full build (2026-08-22) deleted two of them and improved a third site, so
// only the differences that cost bytes are still spelled separately, each with
// what folding it measured.

// Absorbed SPLIT_ABS (byte-neutral: hoisting abs into its own statement is the
// same IL) and one of the two SIGNED_OUTPUT sites (StepArrivalDefense +0.15).
#define SELECT_RANDOM_EXTENT_POINT(object, outX, spanX, outY, spanY)                               \
    i32 outX = object->m_extent.left;                                                              \
    i32 spanX = abs(object->m_extent.right - outX);                                                \
    i32 outY = object->m_extent.top;                                                               \
    i32 spanY = abs(object->m_extent.bottom - outY);                                               \
    if (spanX != 0) {                                                                              \
        outX += rand() % spanX;                                                                    \
    }                                                                                              \
    if (spanY != 0) {                                                                              \
        outY += rand() % spanY;                                                                    \
    }

// Both spans declared before either output.  ResolveArrivalReposition is the
// only site; folding it into the canonical order costs 95.29 -> 91.38.
#define SELECT_RANDOM_EXTENT_POINT_SPANS_FIRST(object, spanX, spanY, outX, outY)                   \
    i32 spanX = abs(object->m_extent.right - object->m_extent.left);                               \
    i32 spanY = abs(object->m_extent.bottom - object->m_extent.top);                               \
    i32 outX = object->m_extent.left;                                                              \
    i32 outY = object->m_extent.top;                                                               \
    if (spanX != 0) {                                                                              \
        outX += rand() % spanX;                                                                    \
    }                                                                                              \
    if (spanY != 0) {                                                                              \
        outY += rand() % spanY;                                                                    \
    }

// Base and output are distinct locals.  StepArrivalDefenseLean is the only site
// left; folding it costs 88.97 -> 86.92, while the same fold at
// StepArrivalDefense gained 0.15, so the two sites disagree and the loser keeps
// its own spelling.
#define SELECT_RANDOM_EXTENT_POINT_SEPARATE_BASE(object, baseX, spanX, baseY, spanY, outX, outY)   \
    i32 baseX = object->m_extent.left;                                                             \
    i32 spanX = abs(object->m_extent.right - baseX);                                               \
    i32 baseY = object->m_extent.top;                                                              \
    i32 spanY = abs(object->m_extent.bottom - baseY);                                              \
    i32 outX = baseX;                                                                              \
    if (spanX != 0) {                                                                              \
        outX += rand() % spanX;                                                                    \
    }                                                                                              \
    i32 outY = baseY;                                                                              \
    if (spanY != 0) {                                                                              \
        outY += rand() % spanY;                                                                    \
    }

// u32 outputs, so the caller's `outX < grid->m_width` bounds test is an
// unsigned compare.  Absorbed UNSIGNED_ASSIGN (byte-neutral).  REMOVAL
// CONDITION: respell both sites as the canonical signed locals plus an explicit
// static_cast<u32> at the compare, the way ChargeStep and ScanNearestTarget
// already write it, and measure.
#define SELECT_RANDOM_EXTENT_POINT_UNSIGNED_CAST(object, outX, spanX, outY, spanY)                 \
    u32 outX = static_cast<u32>(object->m_extent.left);                                            \
    i32 spanX = abs(object->m_extent.right - static_cast<i32>(outX));                              \
    u32 outY = static_cast<u32>(object->m_extent.top);                                             \
    i32 spanY = abs(object->m_extent.bottom - static_cast<i32>(outY));                             \
    if (spanX != 0) {                                                                              \
        outX += rand() % spanX;                                                                    \
    }                                                                                              \
    if (spanY != 0) {                                                                              \
        outY += rand() % spanY;                                                                    \
    }

#endif // GRUNTZ_GRUNTRANDOMPOINTMACROS_H
