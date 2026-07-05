// ToyPeek.cpp - the toy-peek HUD eyecandy (C:\Proj\Gruntz), a CUserLogic leaf.
// Only the 1-arg ctor is reconstructed here (the shared CUserLogic(obj) prologue
// + the per-class eyecandy tail).
#include <Gruntz/ToyPeek.h>

// The global bute store (g_buteTree @0x6bf620; Find 0x16d190 __thiscall ret 4);
// pinned in src/Gruntz/UserLogic.cpp, re-declared so the "A" node lookup masks.
DATA(0x002bf620)
extern CButeTree g_buteTree;

// The running game clock (g_645588 .data int) stashed into the leaf's +0x58.
DATA(0x00245588)
extern i32 g_645588;

// CToyPeek::CToyPeek (0x98140) - fold the shared CUserLogic(obj) init, then nudge
// the owner up 0x18 px, lock its draw order to 0xdbba0, apply the small-icon
// status-bar sprite, seed the +0x58..+0x64 countdown state and bind the "A" node.
// @early-stop
// eh-ctor-vptr-restamp-position wall (docs/patterns/eh-ctor-vptr-restamp-position.md):
// body byte-identical; residual is the /GX leaf-vptr re-stamp position + EH-state ids
// + the leaf-field zero-init schedule (CToyPeek's wider +0x58..+0x64 init block).
RVA(0x00098140, 0x18e)
CToyPeek::CToyPeek(CGameObject* obj) : CTileLogic(obj) {
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
    m_startClockLo = g_645588;
    m_startClockHi = 0;
    m_prevAnimSetNode = m_objAux->m_1c;
    m_objAux->m_1c = g_buteTree.Find("A");
}

// class-metadata SIZE sweep (misc-Gruntz A-C): matching-neutral, hosted at
// .cpp EOF (see docs/class-metadata-sweep-log.md). SIZE_UNKNOWN = size not yet pinned.
#include <rva.h>
SIZE_UNKNOWN(CToyPeek);
