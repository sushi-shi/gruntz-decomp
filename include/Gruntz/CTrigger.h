// CTrigger.h - the trigger object the point-probe (0x75af0, g_gameReg->m_68->Probe)
// returns. One class, two views recovered from different call sites:
//   +0x10  the bound HUD sprite (read by the teleporter spawn in UserLogic.cpp)
//   +0x170 the required level id (the secret-level trigger match in CSecretLevelTrigger.cpp)
//   +0x198 the required layer id
// Only offsets + code bytes are load-bearing; field names are placeholders.
#ifndef GRUNTZ_GRUNTZ_CTRIGGER_H
#define GRUNTZ_GRUNTZ_CTRIGGER_H

#include <rva.h>

// The bound HUD sprite; defined by the sprite-owning TU (UserLogic.cpp). Only the
// trigger's +0x10 field is typed against it, so a forward declaration suffices.
struct CTeleHudSprite;

struct CTrigger {
    char m_pad0[0x10];
    CTeleHudSprite* m_10; // +0x10  bound HUD sprite
    char m_pad14[0x170 - 0x14];
    i32 m_170; // +0x170  required level id
    char m_pad174[0x198 - 0x174];
    i32 m_198; // +0x198  required layer id
};

#endif // GRUNTZ_GRUNTZ_CTRIGGER_H
