#ifndef GRUNTZ_GRUNTZ_LEVELREZPATH_H
#define GRUNTZ_GRUNTZ_LEVELREZPATH_H

#include <Ints.h>

struct LevelRezData {
    char m_pad00[0x2ec];
    i32 m_2ec; // +0x2ec  returned field
    char m_pad2f0[0x5f4 - 0x2f0];
};

#endif // GRUNTZ_GRUNTZ_LEVELREZPATH_H
