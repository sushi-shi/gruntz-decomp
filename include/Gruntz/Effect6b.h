// Effect6b.h - CEffect6b, the grunt exit-animation holder facet at CGrunt+0x150
// (Apply @0x6b2e0 advances the entrance player's anim cursor). `this` is
// &CGrunt::m_150, so the members ARE the CGrunt +0x150..+0x15c band: m_player ==
// CGrunt::m_154 (the created entrance-anim CGameObject) and m_prevDesc == CGrunt::
// m_prevEntranceDesc (+0x15c, the m_154->m_1a0.m_14 cache - which is exactly what
// Apply stores). The +0x08 hole is CGrunt::m_158 (AnimWorkerObj*).
//
// IDENTITY (2026-07-16, RESOLVED): the pointee of m_player is the plain
// CGameObject - the ex CEntranceAnimPlayer / CDecayMgr / CAnimOwner6b views are
// all dissolved (<Gruntz/UserLogic.h>; the +0x1a0 tail is the embedded
// CAniAdvanceCursor, whose +0x14 is the active descriptor Apply caches).
#ifndef GRUNTZ_EFFECT6B_H
#define GRUNTZ_EFFECT6B_H

#include <Ints.h>
#include <rva.h>

struct CGameObject; // the +0x154 entrance-anim sprite (<Gruntz/UserLogic.h>)
class CAniElement;  // the active-anim descriptor

SIZE_UNKNOWN(CEffect6b);
struct CEffect6b {
    char _00[4];              // +0x00  == CGrunt::m_150
    CGameObject* m_player;    // +0x04  == CGrunt::m_154 (the entrance-anim object)
    char _08[0xc - 8];        // +0x08  == CGrunt::m_158 (AnimWorkerObj*)
    CAniElement* m_prevDesc;  // +0x0c  == CGrunt::m_prevEntranceDesc
    void Apply(i32 a, i32 b); // 0x6b2e0
};

#endif // GRUNTZ_EFFECT6B_H
