#ifndef GRUNTZ_DDRAWMGR_ANIADVANCE_H
#define GRUNTZ_DDRAWMGR_ANIADVANCE_H

#include <Ints.h>
#include <rva.h>
#include <Gruntz/AniAdvanceCursor.h>
#include <Gruntz/Sprite.h>

struct LeafCue;

class DSoundCloneInst;

class CAniDesc : public CObject {
public:
    unsigned char m_flags;
    char m_pad05[0x08 - 0x05];
    i32 m_stepMode;
    i32 m_loopMode;
    i32 m_posMode;
    i32 m_param;
    i32 m_frameTime;
    i32 m_drawValue;
    i32 m_posDX;
    i32 m_posDY;
    char m_pad28[0x2c - 0x28];
    i32 m_randMod;

    LeafCue** m_randTable;
};
SIZE_UNKNOWN();

#endif // GRUNTZ_DDRAWMGR_ANIADVANCE_H
