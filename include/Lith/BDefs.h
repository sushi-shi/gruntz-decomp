#ifndef LITH_BDEFS_H
#define LITH_BDEFS_H

#define SQR(value) ((value) * (value))
#define ROUND(value) static_cast<int>(value + 0.5)
#define LTCLAMP(a, min, max) ((a) < (min) ? (min) : ((a) > (max) ? (max) : (a)))

#endif // LITH_BDEFS_H
