#ifndef GRUNTZ_DDRAWMGR_COLORHSVMACROS_H
#define GRUNTZ_DDRAWMGR_COLORHSVMACROS_H

#define HSV_MAX(a, b) ((a) > (b) ? (a) : (b))

#define HSV_MIN(a, b) ((a) < (b) ? (a) : (b))

#define INTERPOLATE(start, end, amount) ((start) * (g_one - (amount)) + (end) * (amount))

#endif // GRUNTZ_DDRAWMGR_COLORHSVMACROS_H
