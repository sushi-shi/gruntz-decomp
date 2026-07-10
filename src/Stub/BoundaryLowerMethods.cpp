// BoundaryLowerMethods.cpp - small leaf methods recovered from the engine_boundary
// backlog (lower half, RVA < 0x133370). RTTI cannot attribute these COMDAT-folded
// methods, so the owning class names here are placeholders; only the OFFSETS +
// code bytes are load-bearing. Unmodeled engine callees/globals are declared
// NO-body so their rel32/DIR32 operands reloc-mask. Defined in retail-RVA order.
// The per-use owner/referent views now live in <Gruntz/BoundaryLowerMethodsViews.h>
// (pure code motion); the archive object folds to the canonical CSerialArchive.
#include <Gruntz/GruntzMgr.h> // canonical CGruntzMgr (MFC side; umbrella-first) - the
#include <Bute/ButeTree.h>
#include <DDrawMgr/DDrawSubMgrPages.h>
#include <Gruntz/Brickz.h>
#include <Gruntz/BattlezMapConfig.h>
// 0x8e880/0x915d0/0x91620 owners + CGruntzSoundZ
#include <Ints.h>
#include <rva.h>
#include <Gruntz/GameRegistry.h>              // canonical game-manager singleton (0x24556c) view
#include <Gruntz/BoundaryLowerMethodsViews.h> // owner/referent views for this TU
#include <Globals.h>

// The game-manager singleton (0x24556c) as the remaining C104dd0/C112bf0 leaves see it.
extern "C" CGameRegistry* g_mgrSettings;

// ===========================================================================
// 0x0213a0 - virtual-base field getter: read the field at +0x04 of the virtual
// base whose displacement lives in the vbtable's second slot. __thiscall.
// ===========================================================================
RVA(0x000213a0, 0xa)
i32 C213a0::Get() {
    i32* vb = *(i32**)this;
    i32 disp = vb[1];
    return *(i32*)((char*)this + disp + 4);
}

// (0x0464e0 CTypeColl464::Resolve re-homed to src/Gruntz/FortressFlag.cpp - the
// projectile/act type-id resolver; its CVariantSlot grow-path inserter is the
// canonical <Bute/ButeTree.h> CVariantSlot::Set. The CTypeColl464 class declaration
// stays shared in <Gruntz/BoundaryLowerMethodsViews.h>.)

// (0x050ca0 C50ca0::M re-homed to src/Gruntz/Grunt.cpp as
// CGrunt::LoadTypeTableClearMove - this==CGrunt (RunEntranceMove: mov ecx,esi),
// m_1a0==CGrunt::m_moveMode, Method@0x3bd9 == inherited CUserLogic::LoadGruntTypeTable.)

// (0x077dc0 C77dc0::Set re-homed to src/Gruntz/Brickz.cpp as BrickzGridDesc::SetCell
// - the flat grid-cell setter m_20[m_24[y]+x]=id; CTerrainTileLoader::Load reaches
// it via loader->m_24 (BrickzAttrMgr) -> m_5c. Same grid as C112bf0. See <Gruntz/Brickz.h>.)

// (0x099ba0 C99ba0::Ctor re-homed to src/Gruntz/AreaMgr.cpp as the real
// CAreaMgr::CAreaMgr - C99ba0 was a view of CAreaMgr {index, CSpawnList@+4};
// the 0x1b4867 sub Init is the embedded CObList block-size-10 ctor.)

// (0x09a420 C9a420::Clear re-homed to src/Gruntz/AreaMgr.cpp as the real
// CSpawnList::ClearFlags - the walked chain is the CSpawnList node list and the
// cleared back-pointer word is CSpawnEntry::m_flag. See <Gruntz/SpawnList.h>
// for the six-view unification proof.)


// (0x0b4c40 C0b4c40::Handle re-homed to src/Gruntz/GameObjectCtors.cpp as the REAL
// CUFO::SerializeMove - vtable slot 1 (thunk 0x3fb7 -> 0xb4c40). The 0x3035 chain
// resolves to CUFO::Serialize (0xb4d30); m_10 == the bound CGameObject, +0x58/+0x50/
// +0x54 == m_drawActive/m_drawFillCmd/m_fillFraction. The base slot-1 re-signature
// (CUserBase::SerializeMove 4-arg) that blocked this is now done. See <Gruntz/Ufo.h>.)

// ===========================================================================
// 0x0bd450 - init: run the base ctor (0x3625) then open the "c:\gruntz.log" log
// (0x1983). __thiscall.
// ===========================================================================
RVA(0x000bd450, 0x16)
void Cbd450::Init() {
    Base3625();
    OpenLog1983("c:\\gruntz.log");
}


// (0x0d5e20 Cd5e20::M re-homed to src/Image/CImage.cpp as CImage::Slot17 - the
// vtable slot-17 thunk 0x1d1b jmps to 0xd5e20, so this IS CImage's slot-17 virtual
// that forwards its arg through Slot15 (+0x3c) then Slot16 (+0x40).)





// (0x0eb970 Ceb970::Serialize re-homed to src/Gruntz/SBI_WarlordHead.cpp as the REAL
// CSBI_WarlordHead::Serialize - vtable slot 1 (thunk 0x3cd8 -> 0xeb970) is authoritative.
// The attribution-conflict is RESOLVED: 0xe7cd0 (thunk 0x2829, was mis-named
// CSBI_WarlordHead::Serialize) is CSBI_ImageSetAni::Serialize -> re-homed to
// src/Gruntz/SBI_ImageSetAni.cpp. See gruntz sema class CSBI_WarlordHead / CSBI_ImageSetAni.)

// (0x0fa150 Cfa150::Cleanup re-homed to src/Gruntz/GameModeBase.cpp as
// CGameModeBase::BaseCleanup - the base cleanup all game-state ReleaseResources chain
// to (caller graph: CAttract/CBootyState/CMultiBootyState/CCreditsState). The +0x1c
// allocator is CDDrawPtrCollections::RemoveItemA (0x142160); the four blits are
// CDDSurface*. See <Gruntz/GameModeBase.h>.)

// (0x104c80 C104c80::Free re-homed to src/Gruntz/SBI_WellGoo.cpp as
// CSBI_WellGoo::Free - vtable slot-3 thunk 0x30b7 jmps to 0x104c80. See
// <Gruntz/SBI_WellGoo.h>.)




// (0x113860 Gate113860 re-homed to src/Gruntz/TileTriggerContainer.cpp - the
// __stdcall mode-gate helper SerializeApplyA / CTileTriggerFactory::Build call.)

// (0x113e70 C113e70::Serialize re-homed to src/Gruntz/TileTriggerSwitchLogic.cpp as
// CTileTriggerSwitchLogic::DeserializeMatrix - the READ mirror of SerializeMatrix
// (0x113dd0); ApplyByType dispatches to it as ApplyType7 (thunk 0x3cd3 -> 0x113e70).)

// (0x114ec0 Fwd114ec0 re-homed to src/Gruntz/GruntzMgrCmd.cpp - the __cdecl 6-arg
// forwarder the HandleCommand toolbar path calls; it tail-forwards to Fwd114f00.)

// ===========================================================================
// 0x114f00 - guarded forwarder: resolve a2->m_30->m_4->m_10->m_2c and, when live,
// forward it plus the six args to 0x267b. __cdecl(6 args). NOTE: this is the
// 0x21c1-thunk target that GruntzMgrCmd.cpp's Fwd114ec0 forwards to; its deref
// chain still needs real command-context types before it can home there.
// ===========================================================================
extern "C" void Func267b(void* v, i32 a1, i32 a2, i32 a3, i32 a4, i32 a5, i32 a6); // 0x267b
// @early-stop
// identical-return-epilogue tail-merge wall (docs/patterns): cl shares one pop;ret
// tail across the two null guards; retail emits the inline ret at each site. Deref
// chain + 6-arg re-push forward are byte-faithful.
RVA(0x00114f00, 0x3e)
void Fwd114f00(i32 a1, i32 a2, i32 a3, i32 a4, i32 a5, i32 a6) {
    CObj114f* obj = ((CArg114f*)a2)->m_30->m_4->m_10;
    if (obj == 0) {
        return;
    }
    if (obj->m_2c == 0) {
        return;
    }
    Func267b(obj->m_2c, a1, a2, a3, a4, a5, a6);
}

// ===========================================================================
// 0x1181d0 - bounds-grow: reject when the new (+0x04,+0x08) pair does not exceed
// the +0xb8 box; else store it, notify (0x3661) and stash +0xd4. __thiscall(3).
// ===========================================================================
extern "C" void Func3661(CBox118* p); // 0x3661
RVA(0x001181d0, 0x70)
i32 C1181d0::Update(i32 a1, i32 a2, i32 a3) {
    if (a1 == 0) {
        return 0;
    }
    if (a2 == 0) {
        return 0;
    }
    CBox118* b = &m_bounds;
    if (b == 0) {
        return 0;
    }
    if (b->m_4 > a1) {
        return 0;
    }
    if (b->m_4 == a1 && b->m_8 < a2) {
        return 0;
    }
    b->m_4 = a1;
    b->m_8 = a2;
    Func3661(b);
    m_d4 = a3;
    return 1;
}

// ===========================================================================
// 0x118260 - copy-if-grow: reject when the source box does not exceed +0xb8;
// else copy the 7-dword box in and stash +0xd4. __thiscall(src, arg2) ret 8.
// ===========================================================================
RVA(0x00118260, 0x63)
i32 C118260::Update(CRect118* src, i32 arg2) {
    if (src == 0) {
        return 0;
    }
    CRect118* dst = &m_bounds;
    if (dst == 0) {
        return 0;
    }
    if (dst->m_4 > src->m_4) {
        return 0;
    }
    if (dst->m_4 == src->m_4 && dst->m_8 < src->m_8) {
        return 0;
    }
    *dst = *src;
    m_d4 = arg2;
    return 1;
}
