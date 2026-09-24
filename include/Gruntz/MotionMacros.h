#ifndef GRUNTZ_GRUNTZ_MOTIONMACROS_H
#define GRUNTZ_GRUNTZ_MOTIONMACROS_H

#include <Lith/BDefs.h>

#define ARRIVAL_V(v, a, s, target)                                                                 \
    do {                                                                                           \
        double targetPosition = (target);                                                          \
        double nv;                                                                                 \
        if (a == g_motionZero) {                                                                   \
            nv = v;                                                                                \
        } else {                                                                                   \
            double delta = (targetPosition - (s)) * a;                                             \
            double disc = SQR(v) - delta * g_motionNegTwo;                                         \
            if (g_motionZero > disc) {                                                             \
                disc = g_motionZero;                                                               \
            }                                                                                      \
            double r = sqrt(disc);                                                                 \
            nv = (v > g_motionZero) ? r : -r;                                                      \
        }                                                                                          \
        v = nv;                                                                                    \
    } while (0)

#define STEP_AXIS(v, a, s, vmax, loBand, hiBand, posClamp, scr)                                    \
    do {                                                                                           \
        double step0 = dt * a;                                                                     \
        double t = (v - step0 * g_motionNegHalf) * dt;                                             \
        scr = t;                                                                                   \
        do {                                                                                       \
            double c;                                                                              \
            if (t > vmax) {                                                                        \
                c = vmax;                                                                          \
            } else if (t < -vmax) {                                                                \
                c = -vmax;                                                                         \
            } else {                                                                               \
                break;                                                                             \
            }                                                                                      \
            scr = c;                                                                               \
            ARRIVAL_V(v, a, s, c + s);                                                             \
        } while (0);                                                                               \
        double oldS = s;                                                                           \
        double newS = scr + s;                                                                     \
        s = newS;                                                                                  \
        if (newS > hiBand) {                                                                       \
            ARRIVAL_V(v, a, s, hiBand);                                                            \
            scr = hiBand - oldS;                                                                   \
            s = hiBand;                                                                            \
        } else if (newS < loBand) {                                                                \
            ARRIVAL_V(v, a, s, loBand);                                                            \
            scr = loBand - oldS;                                                                   \
            s = loBand;                                                                            \
        } else {                                                                                   \
            v += step0;                                                                            \
        }                                                                                          \
        if (v > posClamp)                                                                          \
            v = posClamp;                                                                          \
    } while (0)

#endif // GRUNTZ_GRUNTZ_MOTIONMACROS_H
