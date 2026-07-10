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

// ===========================================================================
// 0x09cab0 - out-param wrapper: call the +0x10 sub's method (0x1b8008) with a
// zeroed local and return the filled local. __thiscall(arg).
// ===========================================================================
RVA(0x0009cab0, 0x23)
i32 C9cab0::M(i32 arg) {
    i32 local = 0;
    ((CMapStringToPtr*)&m_10)->Lookup((const char*)arg, (void*&)local);
    return local;
}

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

// ===========================================================================
// 0x0cef50 - teardown: flush the +0x04 owner's +0xc8 sub (0x1b9c69); when +0x1c0
// is live, run the +0x0c->+0x04 close (0x158d20 -> 0x158e40) and dispatch +0x04's
// 0x201d(3). __thiscall, returns 1.
// ===========================================================================
RVA(0x000cef50, 0x46)
i32 Ccef50::M() {
    ((CObList*)(m_4 + 0xc8))->~CObList();
    if (m_1c0 != 0) {
        if (((CDDrawSubMgrPages*)m_c->m_4)->Method_158d20() != 0) {
            ((CDDrawSubMgrPages*)m_c->m_4)->Method_158e40();
        }
        ((CGruntzMgr*)m_4)->ChangeState_8fab0(3);
    }
    return 1;
}

// (0x0d5e20 Cd5e20::M re-homed to src/Image/CImage.cpp as CImage::Slot17 - the
// vtable slot-17 thunk 0x1d1b jmps to 0xd5e20, so this IS CImage's slot-17 virtual
// that forwards its arg through Slot15 (+0x3c) then Slot16 (+0x40).)

// ===========================================================================
// 0x0db200 - swap the +0x08 holder to `arg`: no-op when already equal, else
// validate (0x11f9), toggle old off / new on (0x3bbb) and store. __thiscall.
// ===========================================================================
extern "C" i32 Check11f9(void* p);          // 0x11f9
extern "C" void Toggle3bbb(void* p, i32 f); // 0x3bbb
RVA(0x000db200, 0x51)
i32 Cdb200::M(void* arg) {
    if (m_8 == arg) {
        return 1;
    }
    if (Check11f9(arg)) {
        Toggle3bbb(m_8, 1);
        Toggle3bbb(arg, 0);
        m_8 = arg;
        return 1;
    }
    return 0;
}

// ===========================================================================
// 0x0db2f0 - finalize: when +0x20 is live, run the +0x38 teardown (0x40c5) iff
// +0x14 is clear, then reset +0x20. __thiscall, returns 1/0.
// ===========================================================================
RVA(0x000db2f0, 0x2b)
i32 Cdb2f0::M() {
    if (m_20 == 0) {
        return 0;
    }
    if (m_14 == 0) {
        ((CBattlezMapConfig*)&m_38)->Clear_02ade0();
    }
    m_20 = 0;
    return 1;
}

// ===========================================================================
// 0x0db750 - "LEVEL" config sync: on first call (arg==0) probe the +0x0c owner's
// +0x2c config for "LEVEL" (0x152c50); set it (0x1527d0), then resolve the +0x28
// parser entry (0x13bae0) and, if found, bind it back (0x152ad0). __thiscall.
// ===========================================================================
RVA(0x000db750, 0x70)
i32 Cdb750::M(void* arg) {
    if (m_c == 0) {
        return 0;
    }
    if (arg == 0) {
        if (m_c->m_2c->HasKeyPrefix_152c50("LEVEL") != 0) {
            return 1;
        }
    }
    m_c->m_2c->RemoveKeysEqual_1527d0("LEVEL", (const char*)&g_dat60b588);
    void* e = m_28->ResolvePath((const char*)&g_dat613054);
    if (e == 0) {
        return 0;
    }
    m_c->m_2c->ScanTree_152ad0(e, "LEVEL", &g_dat60b588);
    return 1;
}

// ===========================================================================
// 0x0ea170 - 2-bit selector over a +0x38 virtual: pick one of four fixed arg
// tuples by (arg1!=0, arg2!=0). __thiscall(arg1, arg2).
// ===========================================================================
RVA(0x000ea170, 0x5c)
void Cea170::M(i32 a1, i32 a2) {
    if (a1 == 0) {
        if (a2 == 0) {
            Dispatch(1, -1, 0, 0, -1);
        } else {
            Dispatch(-1, -1, -1, 0, -1);
        }
    } else {
        if (a2 == 0) {
            Dispatch(4, -1, 0, 0, -1);
        } else {
            Dispatch(-1, -1, 1, 0, -1);
        }
    }
}

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

// ===========================================================================
// 0x104dd0 - lazy-create the StatusBarSprite: clamp +0x24/+0x28 to the manager's
// screen bounds, then build it via the +0x0c factory (0x1597b0). __thiscall.
// ===========================================================================
// @early-stop
// scheduling wall: retail computes m_8c-0x22 via lea eax,[ecx-0x22] and loads m_x
// late; cl uses sub + an earlier m_x load. Clamp logic, the factory call and the
// StatusBarSprite literal are byte-faithful.
RVA(0x00104dd0, 0x6b)
i32 C104dd0::Create() {
    if (m_sprite != 0) {
        return 0;
    }
    i32 a = g_mgrSettings->m_modeW - 0x22;
    i32 d = g_mgrSettings->m_modeH;
    if (m_x > a) {
        m_x = a;
    }
    if (m_y > d - 9) {
        m_y = d - 0x22;
    }
    m_sprite = m_factoryHolder->m_8->CreateSprite(0, m_x, m_y, 0xf4240, "StatusBarSprite", 1);
    return m_sprite != 0;
}

// ===========================================================================
// 0x10bbe0 - getter: return +0x4cc when +0x528 is clear; else the active cell
// (+0x534[+0x52c]) when the +0x538 count exceeds the index, else 0. __thiscall.
// ===========================================================================
RVA(0x0010bbe0, 0x34)
i32 C10bbe0::M() {
    if (m_528 == 0) {
        return m_fallback;
    }
    if (m_count > 0 && m_count > m_index) {
        return *m_entries[m_index];
    }
    return 0;
}

// ===========================================================================
// 0x112bf0 - decrement the active grid cell (manager-owned plane) and re-publish
// it through the manager's +0x70 notifier (0x33f0). __thiscall, returns 1.
// ===========================================================================
// @early-stop
// strength-reduction wall: cl materializes m_row<<2 (shl ecx,2) and reuses scale-1
// addressing; retail keeps m_row in ecx and uses *4 scaled addressing in both cell
// stores. Logic/offsets byte-faithful; the shift vs scaled-index pick is not steerable.
RVA(0x00112bf0, 0x5e)
i32 C112bf0::M() {
    CGameRegistry* mg = g_mgrSettings;
    CGridData* g = ((CGridOuter*)mg->m_world)->m_24->m_5c;
    i32 v = g->cells[g->rows[m_row] + m_col] - 1;
    CGridData* g2 = ((CGridOuter*)mg->m_world)->m_24->m_5c;
    g2->cells[g2->rows[m_row] + m_col] = v;
    ((CBrickzGrid*)mg->m_tileGrid)->ComputeCellFlags(m_col, m_row, v);
    m_14 = 0;
    return 1;
}

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
