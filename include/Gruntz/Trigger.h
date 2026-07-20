#ifndef GRUNTZ_GRUNTZ_CTRIGGER_H
#define GRUNTZ_GRUNTZ_CTRIGGER_H

#include <rva.h>

struct CGameObject;

struct CTrigger {
    char m_pad0[0x10];
    CGameObject* m_10; // +0x10  bound HUD sprite (game object)
    char m_pad14[0x170 - 0x14];
    i32 m_170; // +0x170  required level id
    char m_pad174[0x198 - 0x174];
    i32 m_198; // +0x198  required layer id
};

#endif // GRUNTZ_GRUNTZ_CTRIGGER_H
