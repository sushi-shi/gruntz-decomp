#include <Gruntz/ToyPeek.h>
#include <Rez/FrameClock.h> // frame-clock band (g_frameDelta/g_frameTime/g_killCueClock/g_engineFrameDelta)
#include <Gruntz/SerialArchive.h> // CFileMemBase (the inherited CWapX::Chain arg; ex SerialObjRef.h)
#include <Io/FileMem.h>           // the serialize stream (CFileMemBase == the real CFileMemBase)
#include <Bute/ButeTree.h>        // g_buteTree

// CToyPeek::~CToyPeek @0x11c40 - empty vtable-anchor dtor; folds the CUserLogic
// teardown (the /GX leaf-dtor archetype).
// IMPLICIT dtor (retail is COMPILER-GENERATED - eh-dtor-vptr-restamp CAUSE B):
// a user-declared `~CToyPeek() {}` emits the leaf-vptr restamp, and the CWapX
// base EH state blocks the dead-store elision that used to hide it. The ??_G
// in the vtable-emitting TU forces the implicit ??1 COMDAT; pinned by name.
#include <rva.h>
RVA_COMPGEN(0x00011c40, 0x44, ??1CToyPeek@@UAE@XZ)

// CToyPeek::CToyPeek (0x98140) - fold the shared CUserLogic(obj) init, then nudge
// the owner up 0x18 px, lock its draw order to 0xdbba0, apply the small-icon
// status-bar sprite, seed the +0x58..+0x64 countdown state and bind the "A" node.
// @early-stop
// eh-ctor-vptr-restamp-position wall (docs/patterns/eh-ctor-vptr-restamp-position.md):
// body byte-identical; residual is the /GX leaf-vptr re-stamp position + EH-state ids
// + the leaf-field zero-init schedule (CToyPeek's wider +0x58..+0x64 init block).
RVA(0x00098140, 0x18e)
CToyPeek::CToyPeek(CGameObject* obj) : CUserLogic(obj), CWapX(obj) {
    m_startClockLo = 0;
    m_countdownLo = 0;
    m_startClockHi = 0;
    m_countdownHi = 0;
    m_object->m_screenY -= 0x18;
    if (m_object->m_sortKey != 0xdbba0) {
        m_object->m_sortKey = 0xdbba0;
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

RVA(0x000983e0, 0x98)
i32 CToyPeek::SerializeMove(CFileMemBase* ar, i32 mode, i32 a3, i32 a4) {
    if (CUserLogic::SerializeMove(ar, mode, a3, a4) == 0) {
        return 0;
    }
    if (Chain(ar, mode, a3, reinterpret_cast<CGameObject*>(a4)) == 0) {
        return 0;
    }
    // The two i64 timer fields sit contiguous (m_startClock @+0x58, m_countdown
    // @+0x60); retail walks one pointer over the blob.
    i32* p = &m_startClockLo;
    switch (mode) {
        case 4:
            ar->Write(p, 8);
            p += 2;
            ar->Write(p, 8);
            break;
        case 7:
            ar->Read(p, 8);
            p += 2;
            ar->Read(p, 8);
            break;
    }
    return 1;
}

#include <rva.h>
VTBL(CToyPeek, 0x001e7204);
