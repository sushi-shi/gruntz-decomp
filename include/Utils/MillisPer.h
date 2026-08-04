#ifndef UTILS_MILLISPER_H
#define UTILS_MILLISPER_H

#include <Enums.h>

// Millisecond conversion factors.
//
// Two functions reconstructed in different files spell the identical ladder,
// which is what makes these conversions rather than tuning values -
// FormatElapsedTime and the TimeSplit helper both do:
//
//   hours   = ms / MILLIS_PER_HOUR;    ms -= hours   * MILLIS_PER_HOUR;
//   minutes = ms / MILLIS_PER_MINUTE;  ms -= minutes * MILLIS_PER_MINUTE;
//   seconds = ms / MILLIS_PER_SECOND;
//
// and the first then prints them "%i:%02i:%02i".
//
// Only the CONVERSIONS live here. A duration that happens to be 60000 ms - the
// level-preview countdown, say - is a tuning value and keeps its own literal;
// it is a minute long by choice, not by arithmetic.
GZ_ENUM_CONST_BEGIN(MillisPer)
    MILLIS_PER_SECOND = 1000,
    MILLIS_PER_MINUTE = 60000,
    MILLIS_PER_HOUR = 3600000
GZ_ENUM_CONST_END(MillisPer)

#endif // UTILS_MILLISPER_H
