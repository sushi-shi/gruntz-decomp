#ifndef GRUNTZ_DDRAWMGR_DDSCREEN_H
#define GRUNTZ_DDRAWMGR_DDSCREEN_H

#include <Ints.h>
#include <rva.h>

struct CTileInfo {
    i32 m_0;      // +0x0
    u32 m_width;  // +0x4  tile width
    u32 m_height; // +0x8  tile height
};
SIZE_UNKNOWN();

class CMoviePlayer;
typedef CMoviePlayer CDDScreen;

#endif // GRUNTZ_DDRAWMGR_DDSCREEN_H
