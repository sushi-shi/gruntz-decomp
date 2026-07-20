#ifndef GRUNTZ_EFFECT6B_H
#define GRUNTZ_EFFECT6B_H

#include <Ints.h>
#include <rva.h>

struct CGameObject; // the +0x154 entrance-anim sprite (<Gruntz/UserLogic.h>)
class CAniElement;  // the active-anim descriptor

SIZE_UNKNOWN(CEffect6b);
struct CEffect6b {
    char _00[4];              // +0x00  == CGrunt::m_150
    CWwdGameObjectA* m_player;    // +0x04  == CGrunt::m_154 (the entrance-anim object)
    char _08[0xc - 8];        // +0x08  == CGrunt::m_158 (AnimWorkerObj*)
    CAniElement* m_prevDesc;  // +0x0c  == CGrunt::m_prevEntranceDesc
    void Apply(i32 a, i32 b); // 0x6b2e0
};

#endif // GRUNTZ_EFFECT6B_H
