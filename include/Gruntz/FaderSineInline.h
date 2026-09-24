#ifndef GRUNTZ_GRUNTZ_FADERSINEINLINE_H
#define GRUNTZ_GRUNTZ_FADERSINEINLINE_H

#include <Gruntz/FaderSubtypes.h>

inline i32 CFaderSine::AccumulateSampleCount(i32 row, i32 delta, float step) {
    i32 count = 0;
    double wanted = delta * step;
    i32 whole = static_cast<i32>(wanted);
    if (whole < wanted) {
        m_fractionalCounts[row] += wanted - whole;
    }
    if (m_fractionalCounts[row] >= g_sineOne) {
        count = static_cast<i32>(m_fractionalCounts[row]);
        m_fractionalCounts[row] -= count;
    }
    count += whole;
    return count;
}

inline i32 CFaderSine::AdvanceSampleCursor(i32 row) {
    ++m_sampleCursors[row];
    if (m_sampleCursors[row] > m_width) {
        m_sampleCursors[row] = 0;
    }
    return m_sampleOrder[m_sampleCursors[row]];
}

#endif // GRUNTZ_GRUNTZ_FADERSINEINLINE_H
