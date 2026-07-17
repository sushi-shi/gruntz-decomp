// ToyPeek.cpp - the toy-peek HUD eyecandy (C:\Proj\Gruntz), a CUserLogic leaf.
// Only the 1-arg ctor is reconstructed here (the shared CUserLogic(obj) prologue
// + the per-class eyecandy tail).
#include <Gruntz/ToyPeek.h>
#include <Gruntz/SerialObjRef.h>  // CSerialObjRef::Chain (0x8c00) - the +0x34 facet in SerializeMove
#include <Io/FileMem.h>           // the serialize stream (CSerialArchive == the real CFileMemBase)
#include <Bute/ButeTree.h> // g_buteTree

// The global bute store (g_buteTree @0x6bf620; Find 0x16d190 __thiscall ret 4);
// pinned in src/Gruntz/UserLogic.cpp, re-declared so the "A" node lookup masks.

// The running game clock (g_frameTime .data int) stashed into the leaf's +0x58.
extern "C" i32 g_frameTime; // DEFINED in Projectile.cpp (extern "C" = canonical linkage)

// CToyPeek::~CToyPeek @0x11c40 - empty vtable-anchor dtor; folds the CUserLogic
// teardown (the /GX leaf-dtor archetype).
RVA(0x00011c40, 0x44)
CToyPeek::~CToyPeek() {}

// CToyPeek::CToyPeek (0x98140) - fold the shared CUserLogic(obj) init, then nudge
// the owner up 0x18 px, lock its draw order to 0xdbba0, apply the small-icon
// status-bar sprite, seed the +0x58..+0x64 countdown state and bind the "A" node.
// @early-stop
// eh-ctor-vptr-restamp-position wall (docs/patterns/eh-ctor-vptr-restamp-position.md):
// body byte-identical; residual is the /GX leaf-vptr re-stamp position + EH-state ids
// + the leaf-field zero-init schedule (CToyPeek's wider +0x58..+0x64 init block).
RVA(0x00098140, 0x18e)
CToyPeek::CToyPeek(CGameObject* obj) : CUserLogic(obj) {
    TILE_LOGIC_SEED(obj);
    m_startClockLo = 0;
    m_countdownLo = 0;
    m_startClockHi = 0;
    m_countdownHi = 0;
    m_object->m_screenY -= 0x18;
    if (m_object->m_latchedAnimId != 0xdbba0) {
        m_object->m_latchedAnimId = 0xdbba0;
        m_object->m_flags |= 0x20000;
    }
    m_38->ApplyLookupSprite("GAME_STATUSBAR_TABZ_STATZTAB_SMALLICONZ", m_object->m_124);
    m_countdownLo = 0x1388;
    m_countdownHi = 0;
    m_startClockLo = g_frameTime;
    m_startClockHi = 0;
    m_prevAnimSetNode = m_objAux->m_1c;
    m_objAux->m_1c = g_buteTree.Find("A");
}

// CToyPeek::SerializeMove (0x0983e0) - the class's vtable slot-1 override: chain
// the base CUserLogic::SerializeMove (bail 0), then the +0x34 serialized-object-
// reference (CSerialObjRef::Chain, bail 0), then round-trip the peek timer's two
// i64 fields (m_startClock @+0x58, m_countdown @+0x60; 8 B each) per mode
// (4 = write @+0x30, 7 = read @+0x2c). Retail walks ONE pointer over the pair
// (edi += 0x58 hoisted, then edi += 8), so the char* walk is the faithful spelling.
//
// RE-HOMED 2026-07-17 (SM1) from InGameIcon.cpp, where it was defined as
// `CInGameIcon::SerializeMove` - a misattribution that made CToyPeek's slot 1
// dispatch a sibling's method. PROOF (gruntz.match.vtable_slot_binding MISBOUND):
//   * vtable_scan --holds 0x0983e0 -> held by exactly ONE vtable: CToyPeek's
//     (0x1e7204) slot 1, via its ILT thunk 0x0043fe. CInGameIcon's own slot 1 is
//     a DIFFERENT body (0x098c90, the ~880 B CArchive marshaler).
//   * CToyPeek and CInGameIcon are SIBLINGS (both `: CUserLogic` by RTTI), so
//     CToyPeek cannot inherit a CInGameIcon method - the binding was impossible.
//   * Corroboration: the body streams two 8-byte fields at +0x58 and +0x60, which
//     are exactly CToyPeek's m_startClock/m_countdown - a layout derived
//     independently from the ctor above (which zero-inits both halves of each).
RVA(0x000983e0, 0x98)
i32 CToyPeek::SerializeMove(CGruntArchive* ar, i32 mode, i32 a3, i32 a4) {
    if (CUserLogic::SerializeMove(ar, mode, a3, a4) == 0) {
        return 0;
    }
    if (SerialRef34()->Chain(ar, mode, a3, (CGameObject*)a4) == 0) {
        return 0;
    }
    // The two i64 timer fields sit contiguous (m_startClock @+0x58, m_countdown
    // @+0x60); retail walks one pointer over the blob.
    char* p = (char*)&m_startClockLo;
    switch (mode) {
        case 4:
            ar->Write(p, 8);
            p += 8;
            ar->Write(p, 8);
            break;
        case 7:
            ar->Read(p, 8);
            p += 8;
            ar->Read(p, 8);
            break;
    }
    return 1;
}

// class-metadata SIZE sweep (misc-Gruntz A-C): matching-neutral, hosted at
// .cpp EOF (see docs/class-metadata-sweep-log.md). SIZE_UNKNOWN = size not yet pinned.
#include <rva.h>
SIZE_UNKNOWN(CToyPeek);
