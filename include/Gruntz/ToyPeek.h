// ToyPeek.h - the toy-peek HUD eyecandy (C:\Proj\Gruntz), a CUserLogic tile-logic
// leaf (RTTI .?AVCToyPeek@@). The 1-arg ctor (0x98140) folds the shared
// CUserLogic(obj) prologue then a per-class tail; the leaf state begins at +0x58.
// Offsets + code bytes are load-bearing; field names are placeholders.
#ifndef GRUNTZ_CTOYPEEK_H
#define GRUNTZ_CTOYPEEK_H

#include <rva.h>
#include <Gruntz/UserLogic.h>

class CToyPeek : public CUserLogic {
public:
    virtual ~CToyPeek() OVERRIDE;                                      // slot 0
    virtual i32 SerializeMove(CGruntArchive*, i32, i32, i32) OVERRIDE; // slot 1
    // slot 2: per-class logic-type id, inline (emitted with the ctor's vtable in ToyPeek.cpp)
    RVA(0x00011bf0, 0x6)
    virtual LogicTypeId GetTypeTag() OVERRIDE {
        return LOGIC_TOYPEEK;
    }
    virtual i32 UserLogicVfunc2() OVERRIDE; // slot 4
    TILE_LOGIC_TAIL
public:
    CToyPeek(CGameObject* obj); // 0x98140

    char m_pad40[0x58 - 0x40];
    // The peek timer: a 64-bit start-clock snapshot (m_startClock) and countdown
    // window (m_countdown), each a lo/hi i32 pair (retail zero-inits both halves).
    i32 m_startClockLo; // +0x58  running-clock snapshot (g_frameTime)
    i32 m_startClockHi; // +0x5c
    i32 m_countdownLo;  // +0x60  countdown (0x1388)
    i32 m_countdownHi;  // +0x64
};
VTBL(CToyPeek, 0x1e7204);

#endif // GRUNTZ_CTOYPEEK_H
