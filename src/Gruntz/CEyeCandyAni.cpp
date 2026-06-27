// CEyeCandyAni.cpp - the animated eyecandy game-object (C:\Proj\Gruntz), a
// CUserLogic leaf. Only the 1-arg ctor is reconstructed here.
#include <Gruntz/CEyeCandyAni.h>

// The global bute store (g_buteTree @0x6bf620; Find 0x16d190 __thiscall ret 4);
// pinned in src/Gruntz/UserLogic.cpp, re-declared so the "A" node lookup masks.
DATA(0x002bf620)
extern CButeTree g_buteTree;

// CEyeCandyAni::CEyeCandyAni (0xac870) - fold the shared CUserLogic(obj) init, bind
// the "A" bute node, apply the cycle geometry, then run the shared eyecandy z-clamp
// + BigActHeight de-prioritize tail (the SAME archetype as CEyeCandy/CBehindCandyAni).
// @early-stop
// eh-ctor-vptr-restamp-position wall (docs/patterns/eh-ctor-vptr-restamp-position.md):
// body byte-identical; residual is the /GX leaf-vptr re-stamp position + EH-state ids.
RVA(0x000ac870, 0x20e)
CEyeCandyAni::CEyeCandyAni(CGameObject* obj) : CUserLogic(obj) {
    m_30 = m_14->m_1c;
    m_14->m_1c = g_buteTree.Find("A");
    if (m_38->m_1b4 == 0) {
        m_40 = m_38->m_1b4;
        m_38->ApplyLookupGeometry("GAME_CYCLE100", 0);
    }
    CGameObject* o = m_10;
    if (o->m_74 == 0 && o->m_198 != 0) {
        i32 v = o->m_198->m_1c + o->m_60 + 0x186a0;
        if (o->m_74 != v) {
            o->m_74 = v;
            o->m_08 |= 0x20000;
        }
    }
    CGameObjLayer* aux = m_10->m_198;
    if (aux != 0) {
        if (aux->m_10 >= g_buteMgr.GetInt("World", "BigActHeight")
            || m_10->m_198->m_14 >= g_buteMgr.GetInt("World", "BigActHeight")) {
            if (m_10->m_7c != 0) {
                m_10->m_7c->m_08 &= ~6;
                m_10->m_7c->m_08 |= 1;
                m_38->m_08 &= ~0x1000002;
                m_38->m_08 |= 0x800000;
            }
        }
    }
}
