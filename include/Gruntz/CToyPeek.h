// CToyPeek.h - the toy-peek HUD eyecandy (C:\Proj\Gruntz), a CUserLogic tile-logic
// leaf (RTTI .?AVCToyPeek@@). The 1-arg ctor (0x98140) folds the shared
// CUserLogic(obj) prologue then a per-class tail; the leaf state begins at +0x58.
// Offsets + code bytes are load-bearing; field names are placeholders.
#ifndef GRUNTZ_CTOYPEEK_H
#define GRUNTZ_CTOYPEEK_H

#include <rva.h>
#include <Gruntz/UserLogic.h>

class CToyPeek : public CUserLogic {
public:
    CToyPeek(CGameObject* obj); // 0x98140

    char m_pad40[0x58 - 0x40];
    i32 m_58; // +0x58  running-clock snapshot (g_645588)
    i32 m_5c; // +0x5c
    i32 m_60; // +0x60  countdown (0x1388)
    i32 m_64; // +0x64
};

#endif // GRUNTZ_CTOYPEEK_H
