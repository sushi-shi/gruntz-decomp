// Effect6b.h - CEffect6b, the grunt exit-animation holder facet at CGrunt+0x150
// (Apply @0x6b2e0 advances the entrance player's anim cursor). `this` is
// &CGrunt::m_150, so the members ARE the CGrunt +0x150..+0x15c band: m_player ==
// CGrunt::m_154 (the CEntranceAnimPlayer) and m_prevDesc == CGrunt::
// m_prevEntranceDesc (+0x15c, "= m_154->m_1b4 cache" - which is exactly what
// Apply stores). The +0x08 hole is CGrunt::m_158 (AnimWorkerObj*). Folding this
// facet into an embedded CGrunt struct member is deferred with the
// CEntranceAnimPlayer == CDecayMgr merge (see the identity note below).
//
// IDENTITY (2026-07-16): the pointee of m_player is ONE object with three views -
// CEntranceAnimPlayer (Grunt.h), CDecayMgr (GruntBehaviorLeaf.h) and the deleted
// CAnimOwner6b (+0x1b4 slice). Their "+0x1a0 layout conflict" is already resolved
// by Grunt.h's own Cursor() note: the CDecayMgr embedded CAniAdvanceCursor m_1a0
// (0x3c) is the TRUE shape, and the player's m_1a4/m_1b4/m_1c0/m_1c8 are duplicate
// names for the cursor's interior (+0x14 active descriptor / +0x20 / +0x28).
// The single-class merge (embed the cursor, drop the duplicates, reconcile the
// player==CGameObject base) is the remaining deferred fold.
#ifndef GRUNTZ_EFFECT6B_H
#define GRUNTZ_EFFECT6B_H

#include <Ints.h>
#include <rva.h>

class CEntranceAnimPlayer; // the +0x154 entrance player (<Gruntz/Grunt.h>)
class CAniElement;         // the active-anim descriptor

SIZE_UNKNOWN(CEffect6b);
struct CEffect6b {
    char _00[4];                    // +0x00  == CGrunt::m_150
    CEntranceAnimPlayer* m_player;  // +0x04  == CGrunt::m_154 (entrance player)
    char _08[0xc - 8];              // +0x08  == CGrunt::m_158 (AnimWorkerObj*)
    CAniElement* m_prevDesc;        // +0x0c  == CGrunt::m_prevEntranceDesc
    void Apply(i32 a, i32 b);       // 0x6b2e0
};

#endif // GRUNTZ_EFFECT6B_H
