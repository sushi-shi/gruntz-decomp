#ifndef LITH_OBJECTUTILITIES_H
#define LITH_OBJECTUTILITIES_H

#include <stdlib.h>

char IsRandomChance(int percent);

inline char IsRandomChance(int percent) {
    return rand() % 100 < percent;
}

#endif // LITH_OBJECTUTILITIES_H
