// ToyPeek.h - the toy-peek HUD eyecandy (C:\Proj\Gruntz), a CUserLogic tile-logic
// leaf (RTTI .?AVCToyPeek@@). The 1-arg ctor (0x98140) folds the shared
// CUserLogic(obj) prologue then a per-class tail; the leaf state begins at +0x58.
// Offsets + code bytes are load-bearing; field names are placeholders.
#ifndef GRUNTZ_CTOYPEEK_H
#define GRUNTZ_CTOYPEEK_H

#include <rva.h>
#include <Gruntz/UserLogic.h>

class CToyPeek : public CUserLogic, public CWapX {
public:
    // NO user-declared dtor: retail's is COMPILER-GENERATED (implicit
    // elides the leaf-vptr restamp; @rva-symbol pin in the home TU).
    virtual i32 SerializeMove(CGruntArchive*, i32, i32, i32) OVERRIDE; // slot 1
    // slot 2: per-class logic-type id, inline (emitted with the ctor's vtable in ToyPeek.cpp)
    RVA(0x00011bf0, 0x6)
    virtual LogicTypeId GetTypeTag() OVERRIDE {
        return LOGIC_TOYPEEK;
    }
    // slot 4 @0x097de0 - the activation dispatcher over this class's act registry
    // (g_iconStateTable). PROVEN CToyPeek's, not CInGameIcon's (where it sat as
    // "RunState"): CToyPeek's RTTI vtable (0x1e7204) slot 4 holds ILT thunk 0x001e83,
    // whose bytes are `e9 58 5f 09 00` = `jmp 0x097de0`. CInGameIcon's own slot 4 is a
    // DIFFERENT thunk (0x002658 -> 0x097880, RunAction), so 0x097de0 was never its
    // virtual - and MSVC5 has no /OPT:ICF, so the body has exactly one owner.
    // Corroborated by the act-cluster archetype (InitActReg 0x15 / FireActivation
    // 0x102 / RegisterActs 0x18d / ctor), which here runs InitIconStateTable ->
    // 0x097de0 -> RegisterIconState 0x097f40 -> CToyPeek::CToyPeek 0x098140: the
    // cluster ends at THIS class's ctor. Nothing but the label tied it to CInGameIcon -
    // RegisterIconState is a FREE fn storing an UNTYPED code ptr. The body still lives
    // in InGameIcon.cpp (that .text run); re-homing the TU is a separate question.
    virtual void FireActivation(i32 id) OVERRIDE; // 0x097de0
public:
    CToyPeek(CGameObject* obj); // 0x98140

    char m_pad54[0x58 - 0x54];
    // The peek timer: a 64-bit start-clock snapshot (m_startClock) and countdown
    // window (m_countdown), each a lo/hi i32 pair (retail zero-inits both halves).
    i32 m_startClockLo; // +0x58  running-clock snapshot (g_frameTime)
    i32 m_startClockHi; // +0x5c
    i32 m_countdownLo;  // +0x60  countdown (0x1388)
    i32 m_countdownHi;  // +0x64
};
VTBL(CToyPeek, 0x1e7204);

// The handler PMF stored in each g_iconStateTable slot (a 4-byte code pointer on
// this complete single-inheritance class; FireActivation dispatches it on `this`).
typedef i32 (CUserLogic::*ToyPeekActHandler)();

#endif // GRUNTZ_CTOYPEEK_H
