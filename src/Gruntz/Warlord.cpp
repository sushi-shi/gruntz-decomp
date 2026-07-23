#include <Gruntz/GruntSpawnConfig.h> // the +0x60 cue-sink/spawn-config object (complete type for the cue calls)
#include <Gruntz/Warlord.h>
#include <Gruntz/AniAdvanceCursor.h>
#include <Gruntz/ActReg.h>            // the shared CActReg (g_actionTable @0x644610)
#include <Gruntz/TypeKeyColl.h>       // the shared zDArray (g_typeColl @0x6bf650)
#include <Gruntz/Grunt.h>             // CGrunt + CGruntHud/g_buteTree/GruntRand
#include <DDrawMgr/DDrawSurfaceMgr.h> // m_38->m_0c (the world root)
#include <DDrawMgr/DDrawSubMgrLeaf.h> // m_0c->m_animRegistry (the anim-key catalog; Lookup 0x1b8438)
#include <DDrawMgr/AniAdvance.h>      // CAniDesc (the descriptor record; ex CAnimElem)
#include <Gruntz/AniElement.h>        // full CAniElement (ResolveIdleAnimation's desc walk)
#include <Gruntz/TriggerMgr.h>     // CTriggerMgr::NearestCellDist (0x7d1d0) - the m_cmdGrid helper
#include <Gruntz/GruntzMgr.h>      // CGruntzMgr (the RTTI-true singleton; ReportError @0x8dc60)
#include <Gruntz/SpriteRefTable.h> // CSpriteRefTable::GetSel (g_gameReg->m_spriteFactory)
#include <Gruntz/State.h> // CState::BuildAssetNamespacePrefixes (ex CNamespaceLoader facet, m_curState)
#include <Gruntz/Play.h>  // CPlay - m_curState real class (m_frameMarker timer)
#include <Gruntz/Timer.h> // CTimer - the frame-marker (m_currentMs)

#include <Bute/ButeTree.h> // the real CButeTree (g_buteTree @0x6bf620)

#include <rva.h>

static const char s_GRUNTZ_[] = "GRUNTZ_";
static const char s__MOVING[] = "_MOVING";
static const char s__DEATH[] = "_DEATH";
static const char s__JOY[] = "_JOY";
static const char s__IDLE[] = "_IDLE";
static const char s__BATTLECRY[] = "_BATTLECRY";
static const char s__IDLE1[] = "_IDLE1";
static const char s__IDLE2[] = "_IDLE2";
static const char s__IDLE3[] = "_IDLE3";
static const char s__IDLE4[] = "_IDLE4";
static const char s__BATTLECRY1[] = "_BATTLECRY1";
static const char s__BATTLECRY2[] = "_BATTLECRY2";
static const char s__BATTLECRY3[] = "_BATTLECRY3";
static const char s__PANIC[] = "_PANIC";
static const char s_WARLORDZ_KING[] = "WARLORDZ_KING";
static const char s_WARLORDZ_NAPOLEAN[] = "WARLORDZ_NAPOLEAN";
static const char s_WARLORDZ_PATTON[] = "WARLORDZ_PATTON";
static const char s_WARLORDZ_VIKING[] = "WARLORDZ_VIKING";
static const char s_keyB[] = "B";
static const char s_keyC[] = "C";
static const char s_keyE[] = "E";
static const char s_keyA[] = "A";
static const char s_keyF[] = "F";

#include <new>      // placement new (the inlined ConstructElements grow loop)
#include <stdlib.h> // rand (CRT PRNG, reloc-masked)
#include <Wap32/ZVec.h>

template<> DATA(0x00244610)
CActReg CActRegPool<CWarlord>::s_table(2000, 2010);

// ===========================================================================
// RegisterWarlordActions  (0x0447a0)  - a free function, NOT a CWarlord method
// ===========================================================================
// Registers six single-letter Gruntz action-type keys ("A".."F") into the global
// bute-name -> type-id tree (g_buteTree), growing the parallel type-key string
// collection (g_typeColl, backed by g_typeColl.m_alloc/g_typeColl.m_grown) on a miss, then
// stamps each resolved type-id's slot in the action-handler dispatch array
// (g_actionTable @0x644610) with that action's handler entry point. The six
// (key, handler) pairs are emitted inline (the same find-or-create block x6, via
// the REGISTER_ACTION macro since cl declines to inline a helper this large). The
// inlined SetAtGrow expands to IndexToPtr + the placement-new ConstructElements
// grow loop (`::new(p) CString` = retail's `test esi,esi; je` null guard + the
// `for(; n--; p++)` lea-recover trip count) + the CString key assign.
//
// @early-stop  (~96.9%, logic + structure byte-exact)
// Two residual scratch-register coin-flips, no source lever:
//   (a) create-path id load: retail funnels g_typeCounter through eax to merge
//       with the FindType path's `mov edi,eax` (`mov eax,[g_typeCounter]; push eax;
//       mov edi,eax`), while our cl coalesces id_ straight into edi (`mov edi,
//       [g_typeCounter]; push edi`) - our cl is strictly MORE optimal, same family
//       as the dead-global-read-spill "our cl is smarter" wall; can't cleanly
//       de-optimize from source (the fresh-var restructure regressed 96.9->84.9%).
//   (b) the count-guard copy register alternates ecx/edx across the 6 blocks
//       (global scheduling); logic identical. Deferred to the final sweep.

#define REGISTER_ACTION(key, handler)                                                              \
    do {                                                                                           \
        i32 id_ = reinterpret_cast<i32>(g_buteTree.Find(key));                                     \
        if (id_ == 0) {                                                                            \
            g_buteTree.Insert(key, reinterpret_cast<void*>(g_typeCounter));                        \
            id_ = g_typeCounter;                                                                   \
            CString* slot_ = reinterpret_cast<CString*>(g_typeColl.IndexToPtr(id_));               \
            CString* p_ = reinterpret_cast<CString*>(g_typeColl.m_alloc);                          \
            for (i32 n_ = g_typeColl.m_grown; n_--; p_++) {                                        \
                ::new (static_cast<void*>(p_)) CString;                                            \
            }                                                                                      \
            *slot_ = key;                                                                          \
            ++g_typeCounter;                                                                       \
        }                                                                                          \
        void** aslot_ = reinterpret_cast<void**>(CActRegPool<CWarlord>::s_table.Resolve(id_));     \
        *aslot_ = reinterpret_cast<void*>(handler);                                                \
    } while (0)
// ===========================================================================
// CWarlord::~CWarlord  (0x0107f0)  - COMPILER-GENERATED, no source body
// ===========================================================================
// CWarlord adds one destructible member past the CUserLogic base - the +0x54
// CString - so the IMPLICIT dtor emits the canonical most-derived teardown:
//   1. ~CString(m_54)                      (retail EH state 1)
//   2. store the CUserLogic vptr (0x5e705c); inline-destruct the +0x18 link's
//      ~EngStr                             (retail EH state 2)
//   3. store the CUserBase vptr (0x5e70b4)
//
// CWarlord declares NO destructor (see Warlord.h). Retail's dtor does not
// re-stamp ??_7CWarlord at entry, and cl 5.0 only elides that store for an
// IMPLICIT dtor - a user-declared one, even `~CWarlord() {}`, always emits it
// (MEASURED both ways with cl 5.0 /O2 /GX). Declaring the dtor purely to hang an
// RVA() on was the mis-model; the label moves to the RVA_COMPGEN pin below.
// This TU's ctor emits ??_7CWarlord -> ??_GCWarlord -> ??1CWarlord, so the
// implicit body is a COMDAT in this obj and the pin resolves against it.
// docs/patterns/eh-dtor-vptr-restamp-presence.md
//
// @early-stop
// The restamp wall is DEAD (implicit dtor above): 73.95% -> 85.43%. What remains is
// NOT a codegen wall - it is a MISSING BASE CLASS, and the residual is its symptom:
//   retail  push ecx (a dedicated this-spill slot); EH states 1/2; add esp,0x10
//   ours    no spill slot;                          EH states 0/1; add esp,0xc
// PROVEN from retail's own EH tables + RTTI (not inferred from codegen):
//   * ~CWarlord pushes handler 0x5d85a0 -> `mov eax,0x5f8298; jmp __CxxFrameHandler`,
//     and FuncInfo @0x5f8298 has magic 0x19930520 and **maxState = 3** (unwind map
//     @0x5f82b8): state0(toState -1), state1(toState 0), state2(toState -1). We
//     compile only 2 states -> we are missing one destructible SUBOBJECT.
//   * state 1's funclet @0x1d8578 is `p = this ? this+0x34 : 0; ~T(p)` - the
//     null-check this-adjust cl emits for a NON-PRIMARY BASE, i.e. a base at +0x34.
//   * CWarlord's RTTI ClassHierarchyDescriptor @0x5f3818 says **attributes=1
//     (MULTIPLE INHERITANCE)**, numBaseClasses=4:
//         CWarlord +0x00 | CUserLogic +0x00 | CUserBase +0x00 | **CWapX +0x34**
//     while CUserLogic's own CHD @0x5f1fd8 is attributes=0 (single-inh, 2 bases) -
//     so CWapX is NOT inherited through CUserLogic: it is a SECOND DIRECT BASE of
//     CWarlord at +0x34. Our header spells that subobject as anonymous padding
//     (m_pad40 / the TILE_LOGIC_TAIL m_34/m_38/m_3c injection, 0x34..0x40 = 12 B).
// So the true shape is `class CWarlord : public CUserLogic, public CWapX`. Landing
// it needs two things this TU does not own: (1) CWapX modeled (no `class CWapX`
// exists in include/ yet) and (2) SIZE(CUserLogic, 0x30) revisited - a base at
// +0x34 requires the primary base to occupy 0x00..0x34, whereas UserLogic.h pins
// 0x30 from "the base ctor's highest write is [esi+0x2c]" (which bounds the
// INITIALIZED fields, not the size). CBehindCandy/CBehindCandyAni are flagged <MI>
// too, so this is a ~50-leaf tile-logic-wide structural item, not a CWarlord one.
// Also entangled: unwind action(0) calls ??1L_8860@@UAE@XZ - still an L_<rva>
// placeholder shell (src/Gruntz/WorldSoundSet.cpp).
RVA_COMPGEN(0x000107f0, 0x55, ??1CWarlord@@UAE@XZ)

typedef enum WarlordOwner {
    WARLORDZ_KING = 0,
    WARLORDZ_NAPOLEAN = 1,
    WARLORDZ_PATTON = 2,
    WARLORDZ_VIKING = 3,
} WarlordOwner;

typedef enum WarlordBattleTag {
    WARLORD_TAG_KING = 0x442,
    WARLORD_TAG_NAPOLEAN = 0x443,
    WARLORD_TAG_PATTON = 0x444,
    WARLORD_TAG_VIKING = 0x445,
} WarlordBattleTag;

#define WARLORD_ANIM_LOOKUP(dst, suffix)                                                           \
    {                                                                                              \
        void* h = 0;                                                                               \
        m_38->OwnerMgr()->m_animRegistry->m_10.Lookup(s_GRUNTZ_ + m_54 + (suffix), h);             \
        dst = reinterpret_cast<CAniElement*>(h);                                                   \
    }

// @early-stop  (~79%; complete correct body, up from a 3.7% stub)
// The whole 1854-byte body is reconstructed and byte-faithful in logic/offsets/
// calls/control-flow (base init, grid-snap, per-owner selector, 4-way owner switch,
// the eleven unrolled name->handle lookups, the tail timer + moving-anim resolve).
// The residual is the classic /GX-heavy-ctor regalloc + EH-state wall in the ~50-
// instruction prologue (the eleven lookups + tail objdiff-match):
//   * prologue regalloc: retail pins the bound object (arg) in edi across the whole
//     base init and holds &m_link in ebx; cl keeps &m_link in edi and reloads arg
//     from [esp+0x3c] - a register-role coin-flip in the shared inline CUserLogic(obj)
//     under this leaf's higher pressure, cascading to the grid-snap `and al,0xe0`
//     (cl: `and ecx,-0x20`), the m_object reload-vs-cache choices, and the bl EH-const.
//   * emission order: retail seeds the tile-tail (m_34/m_38/m_3c) BEFORE the m_54
//     CString member ctor; with `: CUserLogic` + a body TILE_LOGIC_SEED the seed
//     necessarily emits AFTER the member ctor. Deriving CWarlord from the byte-
//     neutral CTileLogic intermediate (its ctor seeds the tail before the member
//     ctor) recovers exactly this order and measured +1.4% (79.15 -> 80.53) - an
//     inheritance change owned by the Fable lane; left as a hand-off (see report).
RVA(0x00042d40, 0x73e)
CWarlord::CWarlord(i32 arg)
    : CUserLogic(reinterpret_cast<CGameObject*>(arg)), CWapX(reinterpret_cast<CGameObject*>(arg)) {
    CGameObject* obj = reinterpret_cast<CGameObject*>(arg);

    // Two 64-bit stamp/window cooldown timers, cleared.
    m_cooldownStampLo = 0;
    m_cooldownWindowLo = 0;
    m_cooldownStampHi = 0;
    m_cooldownWindowHi = 0;
    m_timer2StampLo = 0;
    m_timer2WindowLo = 0;
    m_timer2StampHi = 0;
    m_timer2WindowHi = 0;

    // Snap the bound object onto the 32px tile grid (centered) + latch the warlord
    // anim id and mark the geometry z-key dirty.
    m_object->m_screenX = (m_object->m_screenX & ~0x1f) + 0x10;
    m_object->m_screenY = (m_object->m_screenY & ~0x1f) + 0x10;
    if (m_object->m_sortKey != 0xc3500) {
        m_object->m_sortKey = 0xc3500;
        m_object->m_flags |= 0x20000;
    }
    m_38->m_flags |= 0x2000002;

    // Resolve the per-owner sprite selector from the focus-slot config row (clamped to
    // [0,0x11); fall back to row 1 when the selector resolves empty).
    i32 owner = m_object->m_124;
    i32 cfg = g_gameReg->m_options[owner].m_008;
    if (cfg < 0 || cfg >= 0x11) {
        cfg = 0;
    }
    i32 sel = g_gameReg->m_spriteFactory->GetSel(cfg, 0);
    if (sel == 0) {
        sel = g_gameReg->m_spriteFactory->GetSel(1, 0);
    }
    m_object->m_drawActive = 1;
    m_object->m_drawFillCmd = 0xa;
    m_object->m_drawFillArg = sel;

    switch (owner) {
        case WARLORDZ_KING:
            m_54 = s_WARLORDZ_KING;
            m_ownerTag = WARLORD_TAG_KING;
            break;
        case WARLORDZ_NAPOLEAN:
            m_54 = s_WARLORDZ_NAPOLEAN;
            m_ownerTag = WARLORD_TAG_NAPOLEAN;
            break;
        case WARLORDZ_PATTON:
            m_54 = s_WARLORDZ_PATTON;
            m_ownerTag = WARLORD_TAG_PATTON;
            break;
        case WARLORDZ_VIKING:
            m_54 = s_WARLORDZ_VIKING;
            m_ownerTag = WARLORD_TAG_VIKING;
            break;
        default:
            // 0x8009 / 0x3e9 = the status-bar report id/tag (meaning unproven, kept literal).
            // Dual-view bridge: the singleton IS the RTTI-true CGruntzMgr, whose
            // ReportError @0x8dc60 (WPARAM,LPARAM) is the real symbol the rel32 binds
            // (the CGameRegistry facet's (i32,i32) name resolved to nothing).
            (g_gameReg)->ReportError(0x8009, 0x3e9);
            return;
    }

    // Register the warlord's asset namespace, then resolve every per-state handle.
    g_gameReg->m_curState->BuildAssetNamespacePrefixes(m_54, 1, 0, 0);

    WARLORD_ANIM_LOOKUP(m_idleAnims[0], s__IDLE1);
    WARLORD_ANIM_LOOKUP(m_idleAnims[1], s__IDLE2);
    WARLORD_ANIM_LOOKUP(m_idleAnims[2], s__IDLE3);
    WARLORD_ANIM_LOOKUP(m_idleAnims[3], s__IDLE4);
    WARLORD_ANIM_LOOKUP(m_battlecryAnims[0], s__BATTLECRY1);
    WARLORD_ANIM_LOOKUP(m_battlecryAnims[1], s__BATTLECRY2);
    WARLORD_ANIM_LOOKUP(m_battlecryAnims[2], s__BATTLECRY3);
    WARLORD_ANIM_LOOKUP(m_animJoy, s__JOY);
    WARLORD_ANIM_LOOKUP(m_animDeath, s__DEATH);
    WARLORD_ANIM_LOOKUP(m_animMoving, s__MOVING);
    WARLORD_ANIM_LOOKUP(m_animPanic, s__PANIC);

    m_timer2StampLo = 0;
    m_timer2WindowLo = 0;
    m_timer2StampHi = 0;
    m_timer2WindowHi = 0;
    m_a8 = 0;
    ResolveMovingAnimation();
}
#undef WARLORD_ANIM_LOOKUP

// @early-stop  (STUB - kept at 0% rather than regress; see the FRAME WALL below)
// 0x43670 = CWarlord::SerializeMove (vtable slot 1, +0x4; origin CUserBase). Homed
// from src/Stub/GapFunctions.cpp (matcher-5); attribution vtable-proven (??_7CWarlord
// +0x4). A 3104-byte archive save/load round-trip.
// WIRED (VT1): was the free fn `Gap_043670` - the identity above was already proven but
// never joined to the slot, so ??_7CWarlord+0x4's reloc dangled onto a __cdecl free
// symbol while the class's own `virtual SerializeMove OVERRIDE` had no definition.
// Now the real override (gruntz.match.vtable_slot_binding).
//
// FULLY DECODED (R3, this session) - the complete body is understood; it is NOT a
// blind stub. Signature: i32 SerializeMove(CFileMemBase* ar, i32 mode, i32 a3, i32 a4)
// where ar == CFileMemBase (Read @vtbl+0x2c / Write @vtbl+0x30), a4 is the referenced
// object (int in the mangling, a CGameObject*). Structure:
//   1. if (!CUserLogic::SerializeMove(ar,mode,a3,a4)) return 0;   (0x16e7f0)
//   2. if (!ar) return 0;   (retail SHARES this ret-0 with the save-body null check @43c5c)
//   3. header field (m_40 handle + m_44 0x10 blob):
//        mode 7 LOAD : ar->Read(hbuf,0x80); ar->Read(&m_44,0x10); m_34=m_38=a4;
//                      m_3c=a4->m_7c; m_40 = strlen(hbuf)? reg->m_10.Lookup(hbuf):0
//        mode 4 SAVE : memset(buf); if(m_40) strcpy(buf, reg->KeyOfValue(m_40));
//                      ar->Write(buf,0x80); ar->Write(&m_44,0x10)
//   4. body (2nd switch on mode):
//        mode 7 LOAD : ++g_serialCounter;Read(buf,0x80);m_54=buf; then 11 handles
//                      m_58..m_80 by-name (Read name, Lookup or 0), then Read(&m_a8,4),
//                      Read(&m_ownerTag,4).
//        mode 4 SAVE : the same 11 handles reverse (KeyOfValue->name->Write), then
//                      Write(&m_a8,4), Write(&m_ownerTag,4).
//        mode 8 POST : re-derive the draw-fill selector (the ctor GetSel path, UNCLAMPED).
//   5. tail: the two i64 timers m_88/m_90 then m_98/m_a0, Read (7) / Write (4), ret 1.
//   The registry is the canonical CSerialObjRef.h chain: a4->m_7c (CSerialNameHolder)
//   ->m_0c (CDDrawSurfaceMgr) ->m_animRegistry (CDDrawSubMgrLeaf) - its ::CMapStringToPtr m_object
//   forward-Lookups a key (0x1b8438) and KeyOfValue (RVO CString) reverses it.
//   Every callee/field/mode/chain above was verified against the retail disasm.
//
// FRAME WALL (why it is parked at STUB, not landed): retail's frame is 0x130 because
// its /GX cl gives EACH of the eleven SAVE-side KeyOfValue CString temporaries its OWN
// stack slot ([esp+0x14] + [esp+0x98..0xbc]) and holds two disjoint 0x80 stream
// buffers ([esp+0x18] body + [esp+0xc0] mode-7 header). Our MSVC5 /O2 /GX COALESCES
// those eleven destructible temporaries into one slot (their lifetimes are disjoint -
// KeyOfValue -> inline strcpy -> ~CString, no throwing call spanning them), yielding a
// 0x108 frame. No source spelling defeats the coalescing (tried: unnamed temporary,
// eleven distinct named locals, function-scope buffers). The 0x28 frame delta shifts
// EVERY [esp+N] operand + the arg-load offsets, so objdiff cannot align the base with
// the target at all (match_percent = null, i.e. below the stub's own 0.14%). A complete
// body was written + built green but REVERTED here because it scores under the stub -
// committing it would regress. Needs a dedicated frame-exact pass that can reproduce
// the per-temp slot allocation (or a permuter run seeded with the decode above).
RVA(0x00043670, 0xc20)
i32 CWarlord::SerializeMove(CFileMemBase* ar, i32 mode, i32 a3, i32 a4) {
    return 0;
}

VTBL(CWarlord, 0x001e7404);

RVA(0x00044640, 0x102)
void CWarlord::FireActivation(i32 key) {
    void** slot = reinterpret_cast<void**>(CActRegPool<CWarlord>::s_table.ResolveEntry(key));
    if (*slot != 0) {
        // the handler is a __thiscall dispatched on this warlord (`mov ecx,this;
        // call [slot2]`); a complete-class PMF gives the plain 4-byte code-ptr call.
        typedef i32 (CUserLogic::*StateHandler)();
        StateHandler h =
            *reinterpret_cast<StateHandler*>(CActRegPool<CWarlord>::s_table.ResolveEntry(key));
        (this->*h)();
    }
}

RVA(0x000447a0, 0x333)
void RegisterWarlordActions() {
    REGISTER_ACTION("A", Act_A);
    REGISTER_ACTION("B", Act_B);
    REGISTER_ACTION("C", Act_C);
    REGISTER_ACTION("D", Act_D);
    REGISTER_ACTION("E", Act_E);
    REGISTER_ACTION("F", Act_F);
}

#undef REGISTER_ACTION

RVA(0x00044bb0, 0x38)
i32 CWarlord::RearmMoving() {
    m_38->m_1a0.Advance(g_engineFrameDelta);
    CAniAdvanceCursor* sub = &m_38->m_1a0;
    if (sub->m_28 != 0 && sub->m_20 == 0) {
        ResolveMovingAnimation();
    }
    return 0;
}

RVA(0x00044c00, 0xc6)
i32 CWarlord::LoadAttributes() {
    if (m_38->m_1a0.Advance(g_engineFrameDelta) != 1) {
        return 0;
    }

    CGruntzMgr* reg = g_gameReg;
    if (reg->m_134 != 1) {
        CWwdGameObjectA* o = m_object;
        i32 dist = (static_cast<CTriggerMgr*>(reg->m_cmdGrid))
                       ->NearestCellDist(o->m_124, o->m_screenX, o->m_screenY);
        if (dist < g_buteMgr.GetIntDef("Warlordz", "PanicRadius", 0x40)) {
            NotifyFortUnderAttack();
            return 0;
        }
    }

    if (static_cast<i64>(static_cast<u32>(g_frameTime))
            - *reinterpret_cast<i64*>(&m_cooldownStampLo)
        >= *reinterpret_cast<i64*>(&m_cooldownWindowLo)) {
        if (rand() % 10 < 5) {
            ResolveIdleAnimation();
            return 0;
        }
        ResolveBattlecryAnimation();
    }
    return 0;
}

// ===========================================================================
// CWarlord::LoadAttributes2  (0x044d10)  - the single-player-aware variant
// ===========================================================================
// Same geo-sub re-arm gate; multiplayer raises the battle alert when the nearest
// enemy is NOT inside the panic radius; single-player resolves the moving anim
// while the level objective is open, else posts a fort battle event past the
// cooldown window and re-arms a 0x7530 stamp. Returns int 0 on every path.
//
// @early-stop
// regalloc wall (topic:regalloc, docs/patterns/zero-register-pinning.md +
// pin-local-for-callee-saved-reg.md): structure/offsets/instruction-selection are
// byte-exact, but retail keeps g_gameReg in edx (live in BOTH the multiplayer and
// single-player branches, freeing ecx for the thiscall `this`) while cl parks it
// in ecx, mirror-swapping g_frameTime into the other scratch reg. A pure scratch
// ecx<->edx coin-flip - no source lever flips it (tried inline vs named helper,
// m_2c-chain split; all no-change at the same ~91% plateau).
RVA(0x00044d10, 0x106)
i32 CWarlord::LoadAttributes2() {
    if (m_38->m_1a0.Advance(g_engineFrameDelta) != 1) {
        return 0;
    }

    CGruntzMgr* reg = g_gameReg;
    if (reg->m_134 != 1) {
        CWwdGameObjectA* o = m_object;
        i32 dist = (static_cast<CTriggerMgr*>(reg->m_cmdGrid))
                       ->NearestCellDist(o->m_124, o->m_screenX, o->m_screenY);
        if (dist >= g_buteMgr.GetIntDef("Warlordz", "PanicRadius", 0x40)) {
            RaiseBattleAlert();
            return 0;
        }
    } else {
        // the play state's frame-marker timer: not yet running / expired
        if ((static_cast<CPlay*>(reg->m_curState))->m_frameMarker->m_currentMs == 0) {
            ResolveMovingAnimation();
            return 0;
        }
        if (static_cast<i64>(static_cast<u32>(g_frameTime))
                - *reinterpret_cast<i64*>(&m_cooldownStampLo)
            >= *reinterpret_cast<i64*>(&m_cooldownWindowLo)) {
            reg->m_cueSink->SpawnVoiceDriver(m_object->m_188, 0x436, -1, -1, -1);
            m_cooldownWindowLo = 0x7530;
            m_cooldownWindowHi = 0;
            m_cooldownStampLo = g_frameTime;
            m_cooldownStampHi = 0;
        }
    }
    return 0;
}

// ===========================================================================
// CWarlord::AdvanceMovingAnim  (0x044e70)  - per-frame moving-state handler
// ===========================================================================
// Advance the +0x1a0 anim sub-mgr off the global geo source; bail while it is
// still animating (m_28==0 || m_20!=0). Once idle, if the fort battle-cue is armed
// (h->m_288) and this warlord belongs to the local player, re-stamp the cue timer
// (clear m_2a0, window m_298=0x3e8, start-stamp m_290=g_frameTime, zero the hi/window-
// hi halves), then re-resolve the moving animation. Returns 0. The registry cue
// helper is g_gameReg->m_cmdGrid viewed as the warlord threat/cue helper (the same
// +0x68 multi-view slot LoadAttributes casts). Reached only through the action table.
//
// @early-stop
// regalloc/scheduling wall (~90%, topic:regalloc topic:scoring-artifact): structure/
// offsets/values/control-flow are byte-faithful (the cue store ORDER now matches with
// the i64 stamp/window model). Two residuals, neither source-steerable: (a) retail
// shrink-wraps `push edi` past the m_28/m_20 gate (edi = g_curPlayer, live only in the
// inner block) where cl saves it at entry; (b) retail rebases `add eax,0x290` to reach
// the cue stores with disp8 while cl keeps disp32 absolute [eax+0x29c/0x290/0x294] -
// same instruction count, an MSVC5 addressing-mode scheduling coin-flip.
RVA(0x00044e70, 0x87)
i32 CWarlord::AdvanceMovingAnim() {
    m_38->m_1a0.Advance(g_engineFrameDelta);
    CAniAdvanceCursor* sub = &m_38->m_1a0;
    if (sub->m_28 == 0 || sub->m_20 != 0) {
        return 0;
    }
    CTriggerMgr* h = g_gameReg->m_cmdGrid;
    if (h->m_phase != 0 && m_object->m_124 == g_curPlayer) {
        h->m_pendingFx = 0;
        CTriggerMgr* h2 = g_gameReg->m_cmdGrid;
        h2->m_timerWindow = 0x3e8;
        h2->m_timerBase = static_cast<u32>(g_frameTime);
    }
    ResolveMovingAnimation();
    return 0;
}

RVA(0x00044f30, 0x38)
i32 CWarlord::RearmMoving2() {
    m_38->m_1a0.Advance(g_engineFrameDelta);
    CAniAdvanceCursor* sub = &m_38->m_1a0;
    if (sub->m_28 != 0 && sub->m_20 == 0) {
        ResolveMovingAnimation();
    }
    return 0;
}

// ===========================================================================
// CWarlord::BuildFortSplashParticles  (0x044f80)
// ===========================================================================
// Re-arm the geo sub-player, and when ready-to-move, spawn the fort splash
// particle at the warlord's clamped screen position (registry effect dispatch),
// arm the panic timer on the registry sub-object, then flag the anim player.
// DECODED (for the final sweep):
//   ((CAniAdvanceCursor*)sub)->Advance(g_engineFrameDelta);                      // m_38+0x1a0, 0x15c360
//   if (sub->m_28 == 0 || sub->m_20 != 0) return;         // ready-to-move gate
//   CWwdGameObjectA* o = m_object; i32 x=o->m_5c, y=o->m_60;
//   if (x in [reg->m_13c, reg->m_144) && y in [reg->m_140, reg->m_148)) {
//     spr = reg->m_30->m_08->CreateSprite(0, x-30, y+10, 0xcf84f, "..."@0x60a96c, 0x40003);
//     if (spr) { spr->ApplyName("..."@0x60d30c);    // 0x150540
//               spr->ApplyLookupGeometry("..."@0x60d30c, 0); } // 0x1505b0
//   }
//   ... panic sub reg->m_68 (m_288/m_2a0/m_290 timer arm to 0x3e8/g_frameTime) ...
//   ... reg->m_150[o->m_124 * 0x48].m_0c = 0 (owner-array slot) ...
// Reconstructed from the decode + the CProjectile water/death-splash template
// (CreateSprite(0,x,y,0xcf84f,"Particlez",0x40003) -> ApplyName/ApplyLookupGeometry)
// and the CWarlord::AdvanceMovingAnim panic-timer block (the typed m_cmdGrid
// members). The spawn is offset (screenX-30, screenY+10); the fort-splash
// template is "LEVEL_FORTSPLASH"; the owner slot is g_gameReg->m_options[m_124] (the
// 0x238-stride per-player record @+0x150).  1.2% (empty stub) -> 96.7%.
// @early-stop
// Residual is the same disp8/disp32 panic-stamp addressing coin-flip documented on
// the sibling AdvanceMovingAnim (retail rebases `add eax,0x290` then disp8 to the
// 0x290/0x294/0x29c stamp words, cl keeps disp32) + a y-load schedule slot. Not
// source-steerable.
RVA(0x00044f80, 0x127)
void CWarlord::BuildFortSplashParticles() {
    m_38->m_1a0.Advance(g_engineFrameDelta);
    CAniAdvanceCursor* sub = &m_38->m_1a0;
    if (sub->m_28 == 0 || sub->m_20 != 0) {
        return;
    }

    CWwdGameObjectA* o = m_object;
    i32 x = o->m_screenX;
    i32 y = o->m_screenY;
    if (x < g_gameReg->m_viewOriginR && x >= g_gameReg->m_viewOriginL
        && y < g_gameReg->m_viewOriginB && y >= g_gameReg->m_viewOriginT) {
        CWwdGameObjectA* fx = g_gameReg->m_world->m_childGroup
                                  ->CreateSprite(0, x - 30, y + 10, 0xcf84f, "Particlez", 0x40003);
        if (fx != 0) {
            fx->ApplyName("LEVEL_FORTSPLASH");
            fx->ApplyLookupGeometry("LEVEL_FORTSPLASH", 0);
        }
    }

    CTriggerMgr* h = g_gameReg->m_cmdGrid;
    if (h->m_phase != 0 && m_object->m_124 == g_curPlayer) {
        h->m_pendingFx = 0;
        CTriggerMgr* h2 = g_gameReg->m_cmdGrid;
        h2->m_timerWindow = 0x3e8;
        h2->m_timerBase = static_cast<u32>(g_frameTime);
    }

    GruntzPlayer* slot = &g_gameReg->m_options[m_object->m_124];
    if (slot != 0) {
        slot->m_00c = 0;
    }
    m_38->m_flags |= 0x10000;
}

RVA(0x00045100, 0x112)
i32 CWarlord::ResolveMovingAnimation() {
    if (m_a8 != 0) {
        return 0;
    }

    m_38->ApplyName(s_GRUNTZ_ + m_54 + s__MOVING);

    m_value = m_38->m_1a0.m_14;
    m_38->m_1a0.Setup(m_animMoving);

    m_prevAnimSetNode = m_objAux->m_1c;
    m_objAux->m_1c = g_buteTree.Find(s_keyB);

    m_cooldownWindowLo = (GruntRand() % 0x5dc1 + 0x1770) * 10;
    m_cooldownWindowHi = 0;
    m_cooldownStampLo = g_movingSeed;
    m_cooldownStampHi = 0;
    return 1;
}

RVA(0x00045270, 0x2a8)
void CWarlord::NotifyFortUnderAttack() {}

RVA(0x000455f0, 0x15b)
i32 CWarlord::ResolveDeathAnimation() {
    if (m_a8 != 0) {
        return 0;
    }
    m_a8 = 1;

    CGruntzMgr* g = g_gameReg;
    if (g->m_134 == 1) {
        CWwdGameObjectA* h = m_object;
        i32 x = h->m_screenX;
        i32 y = h->m_screenY;
        if (x < g->m_viewOriginR && x >= g->m_viewOriginL && y < g->m_viewOriginB
            && y >= g->m_viewOriginT) {
            g->m_cueSink->SpawnVoiceDriver(h->m_188, m_ownerTag, -1, -1, -1);
        }
    } else {
        g->m_cueSink->SpawnVoiceDriver(m_object->m_188, m_ownerTag, -1, -1, -1);
    }

    m_value = m_38->m_1a0.m_14;
    m_38->m_1a0.Setup(m_animDeath);

    m_38->ApplyName(s_GRUNTZ_ + m_54 + s__DEATH);

    m_prevAnimSetNode = m_objAux->m_1c;
    m_objAux->m_1c = g_buteTree.Find(s_keyC);
    return 1;
}

RVA(0x000457b0, 0x14c)
i32 CWarlord::RaiseBattleAlert() {
    if (m_a8 != 0) {
        return 0;
    }

    CGruntzMgr* g = g_gameReg;
    if (g->m_134 == 1) {
        CWwdGameObjectA* h = m_object;
        i32 x = h->m_screenX;
        i32 y = h->m_screenY;
        if (x < g->m_viewOriginR && x >= g->m_viewOriginL && y < g->m_viewOriginB
            && y >= g->m_viewOriginT) {
            g->m_cueSink->SpawnVoiceDriver(h->m_188, 0x435, -1, -1, -1);
        }
    } else {
        g->m_cueSink->SpawnVoiceDriver(m_object->m_188, 0x43f, -1, -1, -1);
    }

    m_value = m_38->m_1a0.m_14;
    m_38->m_1a0.Setup(m_animJoy);

    m_38->ApplyName(s_GRUNTZ_ + m_54 + s__JOY);

    m_prevAnimSetNode = m_objAux->m_1c;
    m_objAux->m_1c = g_buteTree.Find(s_keyE);
    return 1;
}

RVA(0x00045960, 0x181)
i32 CWarlord::ResolveIdleAnimation() {
    if (m_a8 != 0) {
        return 0;
    }

    i32 idx = GruntRand() % 3 + 1;

    CGruntzMgr* g = g_gameReg;
    if (g->m_134 == 1) {
        CWwdGameObjectA* h = m_object;
        i32 x = h->m_screenX;
        i32 y = h->m_screenY;
        if (x < g->m_viewOriginR && x >= g->m_viewOriginL && y < g->m_viewOriginB
            && y >= g->m_viewOriginT) {
            g->m_cueSink->SpawnVoiceDriver(h->m_188, idx + 0x431, -1, -1, -1);
        }
    } else {
        g->m_cueSink->SpawnVoiceDriver(m_object->m_188, idx + 0x43b, -1, -1, -1);
    }

    m_value = m_38->m_1a0.m_14;
    m_38->m_1a0.Setup(m_idleAnims[idx]);

    CAniElement* desc = m_38->m_1a0.m_14;
    CAniDesc* elem =
        desc->m_records.GetSize() > 0 ? static_cast<CAniDesc*>(desc->m_records.GetAt(0)) : 0;
    i32 frame = elem->m_param;

    m_38->ApplyLookupSprite(s_GRUNTZ_ + m_54 + s__IDLE, frame);

    m_prevAnimSetNode = m_objAux->m_1c;
    m_objAux->m_1c = g_buteTree.Find(s_keyA);
    return 1;
}

RVA(0x00045b60, 0x161)
i32 CWarlord::ResolveBattlecryAnimation() {
    if (m_a8 != 0) {
        return 0;
    }

    i32 idx = GruntRand() % 3;

    CGruntzMgr* g = g_gameReg;
    if (g->m_134 == 1) {
        CWwdGameObjectA* h = m_object;
        i32 x = h->m_screenX;
        i32 y = h->m_screenY;
        if (x < g->m_viewOriginR && x >= g->m_viewOriginL && y < g->m_viewOriginB
            && y >= g->m_viewOriginT) {
            g->m_cueSink->SpawnVoiceDriver(h->m_188, idx + 0x42e, -1, -1, -1);
        }
    } else {
        g->m_cueSink->SpawnVoiceDriver(m_object->m_188, idx + 0x438, -1, -1, -1);
    }

    m_value = m_38->m_1a0.m_14;
    m_38->m_1a0.Setup(m_battlecryAnims[idx]);

    m_38->ApplyName(s_GRUNTZ_ + m_54 + s__BATTLECRY);

    m_prevAnimSetNode = m_objAux->m_1c;
    m_objAux->m_1c = g_buteTree.Find(s_keyF);
    return 1;
}
