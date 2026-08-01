#ifndef GRUNTZ_GRUNTZ_POSSOUND_H
#define GRUNTZ_GRUNTZ_POSSOUND_H

#include <Ints.h>
#include <rva.h>
#include <Mfc.h>
#include <DDrawMgr/AnimWorkerObj.h>

class CAmbientPosSound;
struct PosSoundAux;

struct PosSoundAux {
    char m_pad00[0x10];
    GameObjNotifyFn m_handler;

    char m_pad14[0x1c - 0x14];
    i32 m_requestState;
    char m_pad20[0x2c - 0x20];
    i32 m_srcL;
    i32 m_srcR;
    i32 m_srcT;
    i32 m_srcB;
    char m_pad3c[0x168 - 0x3c];
    CAmbientPosSound* m_voice;
};
SIZE_UNKNOWN();

struct PosSoundObj {
    char m_pad00[0x08];
    i32 m_flags08;
    char m_pad0c[0x40 - 0xc];
    i32 m_flags40;
    char m_pad44[0x5c - 0x44];
    i32 m_x;
    i32 m_y;
    char m_pad64[0x7c - 0x64];
    PosSoundAux* m_aux;
    char m_pad80[0x120 - 0x80];
    i32 m_120;
    char m_pad124[0x134 - 0x124];
    RECT m_extent;
    RECT m_area;
    RECT m_placed;
    char m_pad164[0x19c - 0x164];
    struct LeafCue* m_layer;
};
SIZE_UNKNOWN();

#endif // GRUNTZ_GRUNTZ_POSSOUND_H
