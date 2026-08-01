#ifndef GRUNTZ_RANDOM_H
#define GRUNTZ_RANDOM_H

#include <Ints.h>

extern u8 g_randSeeded;
extern i32 g_randSeed;
extern char g_coinRolled;
extern i32 g_coinValue;

namespace Rng {

    i32 Next2();
}

#endif // GRUNTZ_RANDOM_H
