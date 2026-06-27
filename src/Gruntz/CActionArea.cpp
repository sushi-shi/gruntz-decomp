// CActionArea.cpp - the action-area trigger tile-logic game-object (C:\Proj\Gruntz),
// a CUserLogic leaf. Only the 1-arg ctor is reconstructed here.
#include <Gruntz/CActionArea.h>

// The global bute store (g_buteTree @0x6bf620; Find 0x16d190 __thiscall ret 4);
// pinned in src/Gruntz/UserLogic.cpp, re-declared so the "A" node lookup masks.
DATA(0x002bf620)
extern CButeTree g_buteTree;

// CActionArea::CActionArea (0x7da0) - fold the shared CUserLogic(obj) init, then
// name the bound object "GAME_ACTIONAREA_RED", bind the "A" bute node, lock the
// draw order to 6, seed the leaf state (+0x54=1) and flag the sub-object.
// @early-stop
// eh-ctor-vptr-restamp-position wall (docs/patterns/eh-ctor-vptr-restamp-position.md):
// body byte-identical; residual is the /GX leaf-vptr re-stamp position + EH-state ids.
RVA(0x00007da0, 0x17e)
CActionArea::CActionArea(CGameObject* obj) : CUserLogic(obj) {
    m_58 = 0;
    m_60 = 0;
    m_5c = 0;
    m_64 = 0;
    m_38->ApplyName("GAME_ACTIONAREA_RED");
    m_30 = m_14->m_1c;
    m_14->m_1c = g_buteTree.Find("A");
    if (m_10->m_74 != 6) {
        m_10->m_74 = 6;
        m_10->m_08 |= 0x20000;
    }
    m_54 = 1;
    m_60 = 0;
    m_64 = 0;
    m_38->m_40 |= 1;
}
