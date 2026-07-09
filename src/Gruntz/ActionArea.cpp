// ActionArea.cpp - the action-area trigger tile-logic game-object (C:\Proj\Gruntz),
// a CUserLogic leaf. Only the 1-arg ctor is reconstructed here.
#include <Gruntz/ActionArea.h>
#include <Image/ImageSet.h> // CImageSet::SetAllTypes (0x152480) on the object's image set

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
    TILE_LOGIC_SEED(obj);
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

// CActionArea::ApplyColor (0x8580) - re-name the bound object's sprite for the owning
// team (owner 1 -> BLUE, owner 2 -> RED), reset its image set's pixel-format types,
// and clear the object's active bit. Returns 1 on a recognized owner, else 0.
RVA(0x00008580, 0x5e)
i32 CActionArea::ApplyColor(i32 owner) {
    switch (owner) {
        case 1: {
            m_38->ApplyName("GAME_ACTIONAREA_BLUE");
            char* rec = m_38->m_194;
            ((CImageSet*)rec)->SetAllTypes(8);
            break;
        }
        case 2: {
            m_38->ApplyName("GAME_ACTIONAREA_RED");
            char* rec = m_38->m_194;
            ((CImageSet*)rec)->SetAllTypes(8);
            break;
        }
        default:
            return 0;
    }
    m_38->m_stateFlags &= ~1;
    return 1;
}
