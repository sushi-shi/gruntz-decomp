// GameIconFlashEffect.cpp - the "GAME_ICONFLASH" eye-candy effect handler @0xae360,
// a free __cdecl(CGameObject* obj) dispatched from the effect registry (no direct
// rel32 caller - reached via a PMF/registry thunk). A tiny 2-state pump keyed on
// the object's aux state (obj->m_7c->m_1c):
//   state 0 -> mark the object collision-active (m_flags |= 1), apply the
//              "GAME_ICONFLASH" lookup geometry, advance the state to 5.
//   state 5 -> step the embedded anim-advance cursor (obj+0x1a0); when it becomes
//              armed-but-not-running (m_28 != 0 && m_20 == 0), retire the object
//              (m_flags |= 0x10000).
// Always returns 1. @orphan: the owning/registering TU is unrecovered (it sits in
// the gap between CUserLogic @0xae2a0 and CWayPoint @0xae3f0); homed as a focused
// free-function TU over the canonical CGameObject/AnimWorkerObj/CAniAdvanceCursor.
#include <Gruntz/UserLogic.h>
#include <Gruntz/AniAdvanceCursor.h>
#include <rva.h>

// The per-frame draw-delta the advance cursor consumes (canonical _g_6bf3bc @0x6bf3bc,
// reloc-masked here). The former g_defaultGeo alias was C++-mangled and UNBOUND -
// 0x6bf3bc binds to the extern "C" _g_6bf3bc.
extern "C" u32 g_engineFrameDelta;

RVA(0x000ae360, 0x6f)
i32 GameIconFlashEffect(CGameObject* obj) {
    AnimWorkerObj* w = obj->m_7c;
    i32 state = reinterpret_cast<i32>(w->m_1c);
    if (state != 0) {
        if (state == 5) {
            CAniAdvanceCursor* a = &static_cast<CWwdGameObjectA*>(obj)->m_1a0; // the handed obj IS the A-kind sprite
            a->Advance(g_engineFrameDelta);
            if (a->m_28 != 0 && a->m_20 == 0) {
                obj->m_flags |= 0x10000;
                return 1;
            }
        }
        return 1;
    }
    obj->m_flags |= 1;
    static_cast<CWwdGameObjectA*>(obj)->ApplyLookupGeometry("GAME_ICONFLASH", 0);
    w->m_1c = reinterpret_cast<void*>(5);
    return 1;
}
