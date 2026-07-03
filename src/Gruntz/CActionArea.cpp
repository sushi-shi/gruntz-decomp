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
    m_prevAnimSetNode = m_objAux->m_1c;
    m_objAux->m_1c = g_buteTree.Find("A");
    if (m_object->m_latchedAnimId != 6) {
        m_object->m_latchedAnimId = 6;
        m_object->m_flags |= 0x20000;
    }
    m_54 = 1;
    m_60 = 0;
    m_64 = 0;
    m_38->m_stateFlags |= 1;
}

// CActionArea::GetTypeTag (0x7f80) - vtable slot 2: the class's logic-type id
// (0x423), the 6-byte `mov eax,<id>; ret` accessor archetype. Regular method (the
// fat CUserLogic base slot 2 carries a placeholder signature; the leaf vtable is
// not a diffed symbol, so a plain method reproduces the slot bytes exactly).
RVA(0x00007f80, 0x6)
LogicTypeId CActionArea::GetTypeTag() {
    return LOGIC_ACTIONAREA; // 0x423
}

// CActionArea::~CActionArea (0x7fd0) - the leaf adds no destructible members beyond
// CUserLogic (its own +0x54.. state is plain ints), so its dtor folds the bare
// CUserLogic teardown (store CUserLogic vptr, inline-destruct the +0x18 link via
// ~EngStr, store CUserBase vptr; the /GX leaf-dtor archetype). Declaring the virtual
// dtor gives CActionArea its own most-derived vftable so the ctor stamps it (3rd
// vptr) like retail. The out-of-line copy is COMDAT-folded onto the byte-identical
// ~CProjActOwner @0x7fd0 (already RVA-pinned in projactregistry.cpp), so it is left
// UN-annotated here to avoid a duplicate-RVA (the folded-leaf-dtor convention, cf.
// CSecretLevelTrigger::~CSecretLevelTrigger in UserLogic.cpp).
CActionArea::~CActionArea() {}

#include <rva.h>
