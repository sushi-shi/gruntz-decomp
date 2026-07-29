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

#include <Io/FileMem.h> // CFileMemBase - SerializeMove's archive (Read +0x2c / Write +0x30)
#include <Gruntz/SerialCounter.h> // g_serialCounter (bumped per serialized field)
#include <Bute/ButeTree.h>        // the real CButeTree (g_buteTree @0x6bf620)
#include <Gruntz/FontConfig.h>    // CFontConfig - g_gameReg->m_chatLog (AddItem @0x21c60)

#include <rva.h>
#include <new>      // placement new (the inlined ConstructElements grow loop)
#include <stdlib.h> // rand (CRT PRNG, reloc-masked)
#include <Wap32/ZVec.h>

static const char s_GRUNTZ_[] = "GRUNTZ_";
DATA(0x0020d220)
static const char s__MOVING[] = "_MOVING";
static const char s__DEATH[] = "_DEATH";
DATA(0x0020d234)
static const char s__JOY[] = "_JOY";
DATA(0x0020d36c)
static const char s__IDLE[] = "_IDLE";
DATA(0x0020d374)
static const char s__BATTLECRY[] = "_BATTLECRY";
static const char s__IDLE1[] = "_IDLE1";
static const char s__IDLE2[] = "_IDLE2";
static const char s__IDLE3[] = "_IDLE3";
static const char s__IDLE4[] = "_IDLE4";
DATA(0x0020d25c)
static const char s__BATTLECRY1[] = "_BATTLECRY1";
DATA(0x0020d24c)
static const char s__BATTLECRY2[] = "_BATTLECRY2";
DATA(0x0020d23c)
static const char s__BATTLECRY3[] = "_BATTLECRY3";
DATA(0x0020d218)
static const char s__PANIC[] = "_PANIC";
DATA(0x0020d2d8)
static const char s_WARLORDZ_KING[] = "WARLORDZ_KING";
DATA(0x0020d2c0)
static const char s_WARLORDZ_NAPOLEAN[] = "WARLORDZ_NAPOLEAN";
DATA(0x0020d2ac)
static const char s_WARLORDZ_PATTON[] = "WARLORDZ_PATTON";
DATA(0x0020d298)
static const char s_WARLORDZ_VIKING[] = "WARLORDZ_VIKING";
static const char s_keyB[] = "B";
static const char s_keyC[] = "C";
DATA(0x0020d2ec)
static const char s_keyE[] = "E";
static const char s_keyA[] = "A";
static const char s_keyF[] = "F";

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
// The six handlers are CWarlord methods, PROVEN by decoding each ILT jmp thunk the
// slot store references (`mov [eax],<ILT VA>`) and landing on this TU's own bodies
// (`gruntz sema xref <target> --tree` reports the same edge from the other side):
//   "A" thunk 0x003ba7 -> 0x044bb0 CWarlord::RearmMoving
//   "B" thunk 0x001ce9 -> 0x044c00 CWarlord::LoadAttributes
//   "C" thunk 0x0024f0 -> 0x044f80 CWarlord::BuildFortSplashParticles
//   "D" thunk 0x003422 -> 0x044d10 CWarlord::LoadAttributes2
//   "E" thunk 0x00431d -> 0x044e70 CWarlord::AdvanceMovingAnim
//   "F" thunk 0x002725 -> 0x044f30 CWarlord::RearmMoving2
// So they enter the table as ordinary member pointers (CUserLogic is CWarlord's
// primary base, so the static_cast to the base PMF is a zero-delta bit copy - the
// same spelling CObjectDropper::RegisterActs uses); no raw slot write is needed.
//
// The create path feeds the name-slot lookup the GLOBAL `g_typeCounter`, not the local
// `id_` copy - that is what produces retail's `mov eax,[g_typeCounter]; push eax;
// mov edi,eax`: cl CSEs the two reads (no call between them) into eax for the push
// and copies to callee-saved edi for `id_`, which must survive the call. Passing `id_`
// instead made cl coalesce straight into edi (`mov edi,[g_typeCounter]; push edi`) and
// cost the last 2.7% in every one of the six blocks. The old note called that a "our
// cl is smarter" regalloc wall; it was a source bug. 96.9% -> 100% EXACT.

// The find-or-create half. The name slot is the INLINED `_zdvec::IndexToPtr`: retail
// emits `call _zvec::IndexToPtr` (0x312a0, the plain byte accessor) followed by the
// per-slot CString construction expanded in line - so the source spells the base
// accessor and writes the ctor loop out, exactly as CGrunt's registrar does. Naming
// the derived `SlotOf`/`_zdvec::IndexToPtr` here would bind 0x310f0, which retail's
// warlord.obj never references.
#define REGISTER_NAME(key)                                                                         \
    i32 id_ = ActFindId(key);                                                                      \
    if (id_ == 0) {                                                                                \
        ActInsertId(key, g_typeCounter);                                                           \
        id_ = g_typeCounter;                                                                       \
        CString* slot_ = g_typeColl.ScratchResolve(g_typeCounter);                                 \
        CString* p_ = g_typeColl.Slots();                                                          \
        for (i32 n_ = g_typeColl.m_grown; n_--; p_++) {                                            \
            ::new (static_cast<void*>(p_)) CString;                                                \
        }                                                                                          \
        *slot_ = key;                                                                              \
        ++g_typeCounter;                                                                           \
    }

// Blocks "A".."E" reach the handler slot through the same plain byte accessor
// (`mov ecx,g_actionTable; call 0x312a0`), so the element type is re-applied at that
// one seam.
#define REGISTER_ACTION(key, handler)                                                              \
    do {                                                                                           \
        REGISTER_NAME(key)                                                                         \
        /* language-forced: the slot holds a pointer-to-MEMBER; the plain byte    */               \
        /* accessor above is the one seam where CActHandler goes back on.        */                \
        *reinterpret_cast<CActHandler*>(CActRegPool<CWarlord>::s_table._zvec::IndexToPtr(id_)) =   \
            static_cast<CActHandler>(handler);                                                     \
    } while (0)

// Block "F" alone reaches its slot through the TYPED zDArray<CActHandler>::Resolve
// (retail: `mov ecx,g_actionTable; call 0x3544` -> 0x464e0, where the other five
// blocks call 0x312a0 through 0x3864). Byte-identical codegen either way - the two
// COMDATs have the same 0x74 body and MSVC5 has no /OPT:ICF - but the reference must
// be bound to the address retail actually uses.
#define REGISTER_ACTION_TYPED(key, handler)                                                        \
    do {                                                                                           \
        REGISTER_NAME(key)                                                                         \
        *CActRegPool<CWarlord>::s_table.Resolve(id_) = static_cast<CActHandler>(handler);          \
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
RVA_COMPGEN(0x000107c0, 0x1e, ??_GCWarlord@@UAEPAXI@Z)
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
        /* CMapStringToPtr::Lookup's out-param is void*& - the element type is */                  \
        /* API-forced back on at the call, and void*->T* is a static_cast       */                 \
        dst = static_cast<CAniElement*>(h);                                                        \
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
// Closed since: the sortKey test + flags RMW share ONE m_object read (retail reuses the
// eax it already holds: `mov ecx,[eax+8] / or ecx,0x20000 / mov [eax+8],ecx`, not a
// memory RMW through a reloaded pointer), and so do the three draw-state stores.
// The COMDAT is code (0x73e) + a 2-byte align pad + the owner switch's 4-entry jump
// table at 0x43480 (the `jmp [eax*4+0x443480]` reloc target), so the span is 0x750:
// carved at 0x73e the delinked target obj lost the table and objdiff scored our 5 extra
// rows (the `mov edi,edi` pad + 4 dwords) as inserts.
RVA(0x00042d40, 0x750)
CWarlord::CWarlord(CGameObject* obj) : CUserLogic(obj), CWapX(obj) {

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
    // ONE m_object read serves the sortKey test AND the flags RMW (retail reuses the
    // eax it already loaded: `mov ecx,[eax+8] / or ecx,0x20000 / mov [eax+8],ecx`);
    // spelled as two `m_object->` chains cl re-loads and degrades to a memory RMW.
    CWwdGameObjectA* o = m_object;
    if (o->m_sortKey != 0xc3500) {
        o->m_sortKey = 0xc3500;
        o->m_flags |= 0x20000;
    }
    m_38->m_flags |= 0x2000002;

    // Resolve the per-owner sprite selector from the focus-slot config row (clamped to
    // [0,0x11); fall back to row 1 when the selector resolves empty).
    i32 owner = m_object->m_124;
    i32 cfg = g_gameReg->m_options[owner].m_008;
    if (cfg < 0 || cfg >= 0x11) {
        cfg = 0;
    }
    CShadeTable* sel = g_gameReg->m_spriteFactory->GetSel(cfg, 0);
    if (sel == 0) {
        sel = g_gameReg->m_spriteFactory->GetSel(1, 0);
    }
    CWwdGameObjectA* d = m_object; // one read for the three draw-state stores
    d->m_drawActive = 1;
    d->m_drawFillCmd = 0xa;
    d->m_drawFillArg = sel;

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
// blind stub. Signature: i32 SerializeMove(CFileMemBase* ar, i32 mode, i32 typeId, CGameObject* pObj)
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
// The 0xc20 body, reconstructed 2026-07-29. The old note parked this at a stub over a
// FRAME WALL: retail's frame is 0x130 because each of the eleven save-side KeyOfValue
// CString temporaries gets its OWN slot, and it claimed "our MSVC5 /O2 /GX COALESCES
// those eleven destructible temporaries into one slot ... No source spelling defeats the
// coalescing (tried: unnamed temporary, eleven distinct named locals, function-scope
// buffers)."
//
// The spelling that defeats it is the UNNAMED temporary consumed IN PLACE, and retail
// says so itself: every one of those blocks ends `call KeyOfValue; mov edi,[eax]` -
// reading the returned CString's m_pchData straight off the return-buffer pointer, which
// is what an unnamed temporary compiles to. A NAMED local stores it and reloads
// (`mov edi,[esp+N]`), and cl then coalesces the slots. Measured independently on
// CInGameIcon::SerializeMove @0x98c90 this session: two named locals share one slot,
// two unnamed temporaries take two. Frame accounting closes exactly:
//   0x10 (2 dwords) + 0x18..0x97 (body buf 0x80) + 0x98..0xbf (10 temp slots)
//   + 0xc0..0x13f (chain header buf 0x80) = 0x130.
//
// Structure, all read off the bytes: the CWapX::Chain half expanded in place (as in
// CInGameIcon), then a three-way mode dispatch - 4 stores the name plus the eleven anim
// keys, 7 loads them back through the anim registry, 8 re-seeds the bound object's fill
// shade from the player's sprite row - then two cursor-walked 64-bit timer pairs shared
// by both directions.
//
// Four more spellings were load-bearing, each measured:
//   * the `ar == 0` gate and the STORE arm's `world == 0` gate share ONE exit (`goto
//     fail`); the LOAD arm keeps its own, exactly as retail lays them out (91.8 -> 92.5);
//   * the mode-8 arm hoists m_object into a local - spelled `m_object->` per statement,
//     cl5 cannot rule out the stores aliasing the member and reloads before each of the
//     three (92.5 -> 93.3);
//   * the eleven LOAD blocks gate on `!= 0` so the lookup is on the fallthrough and the
//     zero-store goes out of line - the MIRROR of the chain half above, which wants
//     `== 0` (93.3 -> 99.5, the single biggest step);
//   * the lookup's map address is taken into a local before the out-slot is zeroed, so
//     the `0` store lands after both argument pushes as retail has it.
// @early-stop
// 99.52%. The only residue is the chain half's lookup block, and it is not this
// function's: CWapX::Chain @0x8c00 - the same code, out of line - carries the identical
// one at 92.65%. Whatever fixes it there fixes it here.
RVA(0x00043670, 0xc20)
i32 CWarlord::SerializeMove(CFileMemBase* ar, i32 mode, i32 a3, CGameObject* obj) {
    // Two 0x80 buffers: the body buffer every per-anim block formats through, and the
    // separate header buffer the chain half's READ arm fills.
    char buf[0x80];
    char hdr[0x80];

    if (CUserLogic::SerializeMove(ar, mode, a3, obj) == 0) {
        return 0;
    }
    if (ar == 0) {
        // shares the STORE arm's `world == 0` exit - retail reaches one block from both
        // (`test ebx,ebx; je 0x43c5c`); the LOAD arm keeps its own.
        goto fail;
    }

    // --- the inlined CWapX::Chain half ---
    switch (mode) {
        case 7: {
            ar->Read(hdr, 0x80);
            ar->Read(m_blob, 0x10);
            m_34 = obj;
            m_38 = static_cast<CWwdGameObjectA*>(obj);
            m_3c = obj->m_7c;
            if (strlen(hdr) == 0) {
                m_value = 0;
            } else {
                CMapStringToPtr* map = &m_3c->m_ownerCtx->m_animRegistry->m_10;
                void* v = 0;
                map->Lookup(hdr, v);
                m_value = static_cast<CAniElement*>(v);
            }
            break;
        }
        case 4: {
            memset(buf, 0, sizeof(buf));
            if (m_value != 0) {
                strcpy(
                    buf,
                    static_cast<const char*>(m_3c->m_ownerCtx->m_animRegistry->KeyOfValue(m_value))
                );
            }
            ar->Write(buf, 0x80);
            ar->Write(m_blob, 0x10);
            break;
        }
    }

    switch (mode) {
        case 4: {
            CDDrawSurfaceMgr* world = m_3c->m_ownerCtx;
            if (world == 0) {
                goto fail;
            }
            g_serialCounter++;
            memset(buf, 0, sizeof(buf));
            strcpy(buf, static_cast<const char*>(m_54));
            ar->Write(buf, 0x80);
            g_serialCounter++;
            memset(buf, 0, sizeof(buf));
            if (m_idleAnims[0] != 0) {
                strcpy(
                    buf,
                    static_cast<const char*>(world->m_animRegistry->KeyOfValue(m_idleAnims[0]))
                );
            }
            ar->Write(buf, 0x80);
            g_serialCounter++;
            memset(buf, 0, sizeof(buf));
            if (m_idleAnims[1] != 0) {
                strcpy(
                    buf,
                    static_cast<const char*>(world->m_animRegistry->KeyOfValue(m_idleAnims[1]))
                );
            }
            ar->Write(buf, 0x80);
            g_serialCounter++;
            memset(buf, 0, sizeof(buf));
            if (m_idleAnims[2] != 0) {
                strcpy(
                    buf,
                    static_cast<const char*>(world->m_animRegistry->KeyOfValue(m_idleAnims[2]))
                );
            }
            ar->Write(buf, 0x80);
            g_serialCounter++;
            memset(buf, 0, sizeof(buf));
            if (m_idleAnims[3] != 0) {
                strcpy(
                    buf,
                    static_cast<const char*>(world->m_animRegistry->KeyOfValue(m_idleAnims[3]))
                );
            }
            ar->Write(buf, 0x80);
            g_serialCounter++;
            memset(buf, 0, sizeof(buf));
            if (m_battlecryAnims[0] != 0) {
                strcpy(
                    buf,
                    static_cast<const char*>(world->m_animRegistry->KeyOfValue(m_battlecryAnims[0]))
                );
            }
            ar->Write(buf, 0x80);
            g_serialCounter++;
            memset(buf, 0, sizeof(buf));
            if (m_battlecryAnims[1] != 0) {
                strcpy(
                    buf,
                    static_cast<const char*>(world->m_animRegistry->KeyOfValue(m_battlecryAnims[1]))
                );
            }
            ar->Write(buf, 0x80);
            g_serialCounter++;
            memset(buf, 0, sizeof(buf));
            if (m_battlecryAnims[2] != 0) {
                strcpy(
                    buf,
                    static_cast<const char*>(world->m_animRegistry->KeyOfValue(m_battlecryAnims[2]))
                );
            }
            ar->Write(buf, 0x80);
            g_serialCounter++;
            memset(buf, 0, sizeof(buf));
            if (m_animJoy != 0) {
                strcpy(buf, static_cast<const char*>(world->m_animRegistry->KeyOfValue(m_animJoy)));
            }
            ar->Write(buf, 0x80);
            g_serialCounter++;
            memset(buf, 0, sizeof(buf));
            if (m_animDeath != 0) {
                strcpy(
                    buf,
                    static_cast<const char*>(world->m_animRegistry->KeyOfValue(m_animDeath))
                );
            }
            ar->Write(buf, 0x80);
            g_serialCounter++;
            memset(buf, 0, sizeof(buf));
            if (m_animMoving != 0) {
                strcpy(
                    buf,
                    static_cast<const char*>(world->m_animRegistry->KeyOfValue(m_animMoving))
                );
            }
            ar->Write(buf, 0x80);
            g_serialCounter++;
            memset(buf, 0, sizeof(buf));
            if (m_animPanic != 0) {
                strcpy(
                    buf,
                    static_cast<const char*>(world->m_animRegistry->KeyOfValue(m_animPanic))
                );
            }
            ar->Write(buf, 0x80);
            ar->Write(&m_a8, 4);
            ar->Write(&m_ownerTag, 4);
            break;
        }
        case 7: {
            CDDrawSurfaceMgr* world = m_3c->m_ownerCtx;
            if (world == 0) {
                return 0;
            }
            g_serialCounter++;
            ar->Read(buf, 0x80);
            m_54 = buf;
            // NOTE the eleven blocks below gate on `!= 0`, not `== 0` like the chain half
            // above: retail puts the LOOKUP on the fallthrough here (`dec ecx; je <zero>`)
            // and the zero-store out of line, the mirror of the chain half's shape.
            g_serialCounter++;
            ar->Read(buf, 0x80);
            if (strlen(buf) != 0) {
                void* v = 0;
                world->m_animRegistry->m_10.Lookup(buf, v);
                m_idleAnims[0] = static_cast<CAniElement*>(v);
            } else {
                m_idleAnims[0] = 0;
            }
            g_serialCounter++;
            ar->Read(buf, 0x80);
            if (strlen(buf) != 0) {
                void* v = 0;
                world->m_animRegistry->m_10.Lookup(buf, v);
                m_idleAnims[1] = static_cast<CAniElement*>(v);
            } else {
                m_idleAnims[1] = 0;
            }
            g_serialCounter++;
            ar->Read(buf, 0x80);
            if (strlen(buf) != 0) {
                void* v = 0;
                world->m_animRegistry->m_10.Lookup(buf, v);
                m_idleAnims[2] = static_cast<CAniElement*>(v);
            } else {
                m_idleAnims[2] = 0;
            }
            g_serialCounter++;
            ar->Read(buf, 0x80);
            if (strlen(buf) != 0) {
                void* v = 0;
                world->m_animRegistry->m_10.Lookup(buf, v);
                m_idleAnims[3] = static_cast<CAniElement*>(v);
            } else {
                m_idleAnims[3] = 0;
            }
            g_serialCounter++;
            ar->Read(buf, 0x80);
            if (strlen(buf) != 0) {
                void* v = 0;
                world->m_animRegistry->m_10.Lookup(buf, v);
                m_battlecryAnims[0] = static_cast<CAniElement*>(v);
            } else {
                m_battlecryAnims[0] = 0;
            }
            g_serialCounter++;
            ar->Read(buf, 0x80);
            if (strlen(buf) != 0) {
                void* v = 0;
                world->m_animRegistry->m_10.Lookup(buf, v);
                m_battlecryAnims[1] = static_cast<CAniElement*>(v);
            } else {
                m_battlecryAnims[1] = 0;
            }
            g_serialCounter++;
            ar->Read(buf, 0x80);
            if (strlen(buf) != 0) {
                void* v = 0;
                world->m_animRegistry->m_10.Lookup(buf, v);
                m_battlecryAnims[2] = static_cast<CAniElement*>(v);
            } else {
                m_battlecryAnims[2] = 0;
            }
            g_serialCounter++;
            ar->Read(buf, 0x80);
            if (strlen(buf) != 0) {
                void* v = 0;
                world->m_animRegistry->m_10.Lookup(buf, v);
                m_animJoy = static_cast<CAniElement*>(v);
            } else {
                m_animJoy = 0;
            }
            g_serialCounter++;
            ar->Read(buf, 0x80);
            if (strlen(buf) != 0) {
                void* v = 0;
                world->m_animRegistry->m_10.Lookup(buf, v);
                m_animDeath = static_cast<CAniElement*>(v);
            } else {
                m_animDeath = 0;
            }
            g_serialCounter++;
            ar->Read(buf, 0x80);
            if (strlen(buf) != 0) {
                void* v = 0;
                world->m_animRegistry->m_10.Lookup(buf, v);
                m_animMoving = static_cast<CAniElement*>(v);
            } else {
                m_animMoving = 0;
            }
            g_serialCounter++;
            ar->Read(buf, 0x80);
            if (strlen(buf) != 0) {
                void* v = 0;
                world->m_animRegistry->m_10.Lookup(buf, v);
                m_animPanic = static_cast<CAniElement*>(v);
            } else {
                m_animPanic = 0;
            }
            ar->Read(&m_a8, 4);
            ar->Read(&m_ownerTag, 4);
            break;
        }
        case 8: {
            // Re-seed the bound sprite's fill shade from this player's sprite row; if the
            // row has no table, fall back to row 1 and arm the decay fill-bar (cmd 0xa).
            if (g_gameReg->m_spriteFactory->GetSel(g_gameReg->m_options[m_object->m_124].m_008, 0)
                == 0) {
                CShadeTable* fallback = g_gameReg->m_spriteFactory->GetSel(1, 0);
                // hoisted: retail loads m_object ONCE here and does all three stores off
                // it. Spelled `m_object->` per statement, cl5 cannot rule out the stores
                // aliasing the member itself and reloads before each - 8 bytes long.
                CWwdGameObjectA* sprite = m_object;
                sprite->m_drawActive = 1;
                sprite->m_drawFillCmd = 0xa;
                sprite->m_drawFillArg = fallback;
            }
            break;
        }
    }

    // The two 64-bit timer pairs, each walked by ONE advancing cursor (retail hoists the
    // `lea` above the mode compare and steps it with `add r,8`). Braced so the `goto
    // fail` above does not jump past the cursor initialisations.
    {
        i32* cooldown = &m_cooldownStampLo;
        switch (mode) {
            case 7:
                ar->Read(cooldown, 8);
                cooldown += 2;
                ar->Read(cooldown, 8);
                break;
            case 4:
                ar->Write(cooldown, 8);
                cooldown += 2;
                ar->Write(cooldown, 8);
                break;
        }
        i32* timer2 = &m_timer2StampLo;
        switch (mode) {
            case 7:
                ar->Read(timer2, 8);
                timer2 += 2;
                ar->Read(timer2, 8);
                break;
            case 4:
                ar->Write(timer2, 8);
                timer2 += 2;
                ar->Write(timer2, 8);
                break;
        }
    }
    return 1;
fail:
    return 0;
}

VTBL(CWarlord, 0x001e7404);

RVA(0x00044640, 0x102)
void CWarlord::FireActivation(i32 key) {
    // the handler is a __thiscall dispatched on this warlord (`mov ecx,this;
    // call [slot2]`); a complete-class PMF gives the plain 4-byte code-ptr call.
    if (*CActRegPool<CWarlord>::s_table.ResolveEntry(key) != 0) {
        CActHandler h = *CActRegPool<CWarlord>::s_table.ResolveEntry(key);
        (this->*h)();
    }
}

RVA(0x000447a0, 0x333)
void RegisterWarlordActions() {
    REGISTER_ACTION("A", &CWarlord::RearmMoving);
    REGISTER_ACTION("B", &CWarlord::LoadAttributes);
    REGISTER_ACTION("C", &CWarlord::BuildFortSplashParticles);
    REGISTER_ACTION("D", &CWarlord::LoadAttributes2);
    REGISTER_ACTION("E", &CWarlord::AdvanceMovingAnim);
    REGISTER_ACTION_TYPED("F", &CWarlord::RearmMoving2);
}

#undef REGISTER_ACTION
#undef REGISTER_ACTION_TYPED
#undef REGISTER_NAME

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
        i32 dist = reg->m_cmdGrid->NearestCellDist(o->m_124, o->m_screenX, o->m_screenY);
        if (dist < g_buteMgr.GetIntDef("Warlordz", "PanicRadius", 0x40)) {
            NotifyFortUnderAttack();
            return 0;
        }
    }

    if (static_cast<i64>(static_cast<u32>(g_frameTime)) - m_cooldownStamp64 >= m_cooldownWindow64) {
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
// The old "scratch ecx<->edx coin-flip" note was wrong: there is no `reg` LOCAL here.
// Retail reaches the registry as `g_gameReg->...` at each of the four use sites; cl
// CSEs the global load into edx and keeps it live across BOTH branches, leaving ecx
// free for the thiscall receiver. Binding it to a named local instead makes the local
// the ecx tenant and the receiver load consume it - which is what mirror-swapped every
// scratch register in the body.
RVA(0x00044d10, 0x106)
i32 CWarlord::LoadAttributes2() {
    if (m_38->m_1a0.Advance(g_engineFrameDelta) != 1) {
        return 0;
    }

    if (g_gameReg->m_134 != 1) {
        CWwdGameObjectA* o = m_object;
        i32 dist = g_gameReg->m_cmdGrid->NearestCellDist(o->m_124, o->m_screenX, o->m_screenY);
        if (dist >= g_buteMgr.GetIntDef("Warlordz", "PanicRadius", 0x40)) {
            RaiseBattleAlert();
            return 0;
        }
    } else {
        // the play state's frame-marker timer: not yet running / expired
        if ((static_cast<CPlay*>(g_gameReg->m_curState))->m_frameMarker->m_currentMs == 0) {
            ResolveMovingAnimation();
            return 0;
        }
        if (static_cast<i64>(static_cast<u32>(g_frameTime)) - m_cooldownStamp64
            >= m_cooldownWindow64) {
            g_gameReg->m_cueSink->SpawnVoiceDriver(m_object->m_188, 0x436, -1, -1, -1);
            m_cooldownWindow64 = 0x7530;
            m_cooldownStamp64 = static_cast<u32>(g_frameTime);
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
// The gate is written in its POSITIVE form (`if (ready) { ... }` around the whole
// body) rather than as an early return: that is what makes cl shrink-wrap `push edi`
// past the gate exactly as retail does (the early-return spelling saved edi in the
// prologue and cost ~5.5%). Same lever as the sibling BuildFortSplashParticles.
//
// The cue timer is reached through a POINTER to the CueTimer sub-object at +0x290, not
// through four absolute member offsets: that is what makes cl materialize the base once
// (`add eax,0x290`) and reach the remaining three stores with disp8, which is retail's
// (5-byte-smaller) encoding. Same lever on BuildFortSplashParticles @0x44f80.
RVA(0x00044e70, 0x87)
i32 CWarlord::AdvanceMovingAnim() {
    m_38->m_1a0.Advance(g_engineFrameDelta);
    CAniAdvanceCursor* sub = &m_38->m_1a0;
    if (sub->m_28 != 0 && sub->m_20 == 0) {
        CTriggerMgr* h = g_gameReg->m_cmdGrid;
        if (h->m_phase != 0 && m_object->m_124 == g_curPlayer) {
            h->m_pendingFx = 0;
            CueTimer* tm = &g_gameReg->m_cmdGrid->m_cueTimer;
            tm->m_window = 0x3e8;
            tm->m_base = static_cast<u32>(g_frameTime);
        }
        ResolveMovingAnimation();
    }
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
// Reconstructed from the decode + the CProjectile water/death-splash template
// (CreateSprite(0,x,y,0xcf84f,"Particlez",0x40003) -> ApplyName/ApplyLookupGeometry)
// and the CWarlord::AdvanceMovingAnim panic-timer block (the typed m_cmdGrid
// members). The spawn is offset (screenX-30, screenY+10); the fort-splash
// template is "LEVEL_FORTSPLASH"; the owner slot is g_gameReg->m_options[m_124] (the
// 0x238-stride per-player record @+0x150).
//
// Returns i32 0, not void: retail exits `xor eax,eax; ret` on every path, and this is
// one of the six act-table handlers (registered as "C" - see RegisterWarlordActions),
// so it has the family's `i32 ()` signature.
// The ready gate is written in its POSITIVE form (`if (ready) { ... }` wrapping the
// body) rather than as an early return - that is what makes cl shrink-wrap `push esi`
// past the gate the way retail does (retail's early exit jumps to 0x450a2, BELOW the
// `pop esi`, proving the push is conditional). Positive form + declaring y before x
// took this 93.1% -> 98.1%.
//
// The cue timer goes through the CueTimer sub-object pointer - see AdvanceMovingAnim
// @0x44e70 for the addressing-mode lever.
RVA(0x00044f80, 0x127)
i32 CWarlord::BuildFortSplashParticles() {
    m_38->m_1a0.Advance(g_engineFrameDelta);
    CAniAdvanceCursor* sub = &m_38->m_1a0;
    if (sub->m_28 != 0 && sub->m_20 == 0) {
        CWwdGameObjectA* o = m_object;
        i32 y = o->m_screenY;
        i32 x = o->m_screenX;
        if (x < g_gameReg->m_viewBounds.right && x >= g_gameReg->m_viewBounds.left
            && y < g_gameReg->m_viewBounds.bottom && y >= g_gameReg->m_viewBounds.top) {
            CWwdGameObjectA* fx =
                g_gameReg->m_world->m_childGroup
                    ->CreateSprite(0, x - 30, y + 10, 0xcf84f, "Particlez", 0x40003);
            if (fx != 0) {
                fx->ApplyName("LEVEL_FORTSPLASH");
                fx->ApplyLookupGeometry("LEVEL_FORTSPLASH", 0);
            }
        }

        CTriggerMgr* h = g_gameReg->m_cmdGrid;
        if (h->m_phase != 0 && m_object->m_124 == g_curPlayer) {
            h->m_pendingFx = 0;
            CueTimer* tm = &g_gameReg->m_cmdGrid->m_cueTimer;
            tm->m_window = 0x3e8;
            tm->m_base = static_cast<u32>(g_frameTime);
        }

        GruntzPlayer* slot = &g_gameReg->m_options[m_object->m_124];
        if (slot != 0) {
            slot->m_00c = 0;
        }
        m_38->m_flags |= 0x10000;
    }
    return 0;
}

// @early-stop
// The two cooldown halves are single i64 stores (the zero-extension IS retail's
// `mov [hi],ebx`; two i32 stores let cl hoist the hi store above the divide).
// FIXED 2026-07-29: the phase-start stamp read the WRONG GLOBAL - `g_movingSeed`
// (an unbound `i32` in Grunt.cpp) where retail reads `_g_frameTime` @0x645588, the
// same clock LoadAttributes2/NotifyFortUnderAttack stamp into this pair. objdiff's
// reloc row named it outright.
// Residue is now exactly TWO rows: the `m_value = m_1a0.m_14` store lands one slot
// EARLY (before retail's `lea ecx,[m_38+0x1a0]` receiver setup instead of after it).
// Six spellings tried (anim local / no local / prev-temp / cursor pointer / statement
// swap / m_38 local) - all byte-identical bar the cursor-pointer form, which is worse.
// The same one-slot store swap is the whole residue of ResolveDeathAnimation @0x455f0
// and NotifyFortUnderAttack @0x45270, and it does NOT appear in the byte-identical
// twins RaiseBattleAlert/ResolveIdleAnimation/ResolveBattlecryAnimation.
RVA(0x00045100, 0x112)
i32 CWarlord::ResolveMovingAnimation() {
    if (m_a8 != 0) {
        return 0;
    }

    m_38->ApplyName(s_GRUNTZ_ + m_54 + s__MOVING);

    m_value = m_38->m_1a0.m_14;
    m_38->m_1a0.Setup(m_animMoving);

    m_prevAnimSetNode = m_objAux->m_1c;
    m_objAux->m_1c = ActFindId(s_keyB);

    // one i64 store per timer half: the zero-extension IS retail's `mov [hi],ebx`, and
    // spelling it as two i32 stores lets cl hoist the hi store above the divide.
    m_cooldownWindow64 = static_cast<u32>((GruntRand() % 0x5dc1 + 0x1770) * 10);
    m_cooldownStamp64 = static_cast<u32>(g_frameTime);
    return 1;
}

// ===========================================================================
// CWarlord::NotifyFortUnderAttack  (0x045270)  - the fort-under-attack alert
// ===========================================================================
// Skipped while the warlord is dead (m_a8) or already running act "D". Otherwise:
// in the pre-game/attract mode (g_gameReg->m_134 == 1) just fire cue 0x436 and set a
// flat 30 s cooldown; in play, once the alert rate-limiter (m_timer2*, seeded from
// "Warlordz"/"NotifyTimer", default 6000 ms) has expired AND this warlord is the local
// player's pending-fx warlord, fire cue 0x440, push the "Warlordz"/"NotifyString" text
// into the chat log (type 0, data 0x11) and re-arm the limiter. Either way the moving
// cooldown is re-rolled to (rand()%0x5dc1 + 0x1770)*10 ms, the "_PANIC" animation is
// applied, and act "D" is latched. /GX EH frame from the two CString temporaries the
// `"GRUNTZ_" + m_54 + "_PANIC"` concatenation builds.
// @early-stop
// 95.7% (from 0.55%): COMPLETE. The residual is ONE constant-materialisation choice -
// cl5 ALSO enregisters the literal 1 (`mov ebx,0x1`, then `cmp [edx+0x134],ebx` /
// `test bl,al` / `or al,bl` for the magic-static guard), which costs the extra
// `push ebp` retail does not need because retail spells all four 1s as immediates and
// pins only the 0. Nothing in C picks which literal MSVC5 enregisters
// (docs/patterns/const-materialize-into-reg-vs-immediate.md); the guard's two uses are
// compiler-generated, so the count cannot be reduced from source either.
RVA(0x00045270, 0x2a8)
i32 CWarlord::NotifyFortUnderAttack() {
    // Both guards are written POSITIVELY around the whole body, not as early returns:
    // retail has ONE shared `xor eax,eax` bail block and it sits PAST the success
    // epilogue (0x45504), which is what the single trailing `return 0` produces. The
    // early-return spelling duplicated the /GX teardown at each guard and cost a
    // callee-saved push (docs/patterns/positive-gate-enables-shrink-wrap.md).
    if (m_a8 == 0) {
        bool alreadyPanicking = (strcmp(*g_typeColl.GetNameRecord(m_objAux->m_1c), s_codeD) == 0);
        if (!alreadyPanicking) {
            if (g_gameReg->m_134 == 1) {
                g_gameReg->m_cueSink->SpawnVoiceDriver(m_object->m_188, 0x436, -1, -1, -1);
                m_cooldownWindow64 = 0x7530;
                m_cooldownStamp64 = static_cast<u32>(g_frameTime);
            } else {
                if (static_cast<i64>(static_cast<u32>(g_frameTime)) - m_timer2Stamp64
                        >= m_timer2Window64
                    && g_gameReg->m_cmdGrid->m_pendingFx == this) {
                    g_gameReg->m_cueSink->SpawnVoiceDriver(m_object->m_188, 0x440, -1, -1, -1);
                    static CString s_alert("ALERT - Your Fort is under attack!");
                    g_gameReg->m_chatLog->AddItem(
                        static_cast<LPCTSTR>(
                            *g_buteMgr.GetStringDef("Warlordz", "NotifyString", &s_alert)
                        ),
                        0,
                        0x11
                    );
                    m_timer2Window64 =
                        static_cast<u32>(g_buteMgr.GetIntDef("Warlordz", "NotifyTimer", 0x1770));
                    m_timer2Stamp64 = static_cast<u32>(g_frameTime);
                }
                m_cooldownWindow64 = static_cast<u32>((GruntRand() % 0x5dc1 + 0x1770) * 10);
                m_cooldownStamp64 = static_cast<u32>(g_frameTime);
            }

            m_value = m_38->m_1a0.m_14;
            m_38->m_1a0.Setup(m_animPanic);

            m_38->ApplyName(s_GRUNTZ_ + m_54 + s__PANIC);

            m_prevAnimSetNode = m_objAux->m_1c;
            m_objAux->m_1c = ActFindId(s_codeD);
            return 1;
        }
    }
    return 0;
}

// @early-stop
// One-slot store swap only (same as ResolveMovingAnimation @0x45100): retail emits
// `lea ecx,[m_38+0x1a0]` before `mov [this+0x40],edx`, cl the other way round.
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
        if (x < g->m_viewBounds.right && x >= g->m_viewBounds.left && y < g->m_viewBounds.bottom
            && y >= g->m_viewBounds.top) {
            g->m_cueSink->SpawnVoiceDriver(h->m_188, m_ownerTag, -1, -1, -1);
        }
    } else {
        g->m_cueSink->SpawnVoiceDriver(m_object->m_188, m_ownerTag, -1, -1, -1);
    }

    CAniElement* anim = m_animDeath;
    m_value = m_38->m_1a0.m_14;
    m_38->m_1a0.Setup(anim);

    m_38->ApplyName(s_GRUNTZ_ + m_54 + s__DEATH);

    m_prevAnimSetNode = m_objAux->m_1c;
    m_objAux->m_1c = ActFindId(s_keyC);
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
        if (x < g->m_viewBounds.right && x >= g->m_viewBounds.left && y < g->m_viewBounds.bottom
            && y >= g->m_viewBounds.top) {
            g->m_cueSink->SpawnVoiceDriver(h->m_188, 0x435, -1, -1, -1);
        }
    } else {
        g->m_cueSink->SpawnVoiceDriver(m_object->m_188, 0x43f, -1, -1, -1);
    }

    CAniElement* anim = m_animJoy;
    m_value = m_38->m_1a0.m_14;
    m_38->m_1a0.Setup(anim);

    m_38->ApplyName(s_GRUNTZ_ + m_54 + s__JOY);

    m_prevAnimSetNode = m_objAux->m_1c;
    m_objAux->m_1c = ActFindId(s_keyE);
    return 1;
}

// @early-stop
// the voice-cue LOCAL is byte-evidenced (retail `push ebx` + `lea ebx,[edi+0x431]` hoisted
// above the four viewport compares, and every [esp+N] home shifts by the extra push) and is
// KEPT even though objdiff's alignment score fell 92.1 -> 83.7: the prologue, the saved-reg
// set and the cue hoist now match retail exactly, and the residue is the same
// ecx/edx Setup-pair pool phase as ResolveMovingAnimation @0x45100 plus the register
// renames it cascades. MAX 92.06 is preserved by the ledger.
RVA(0x00045960, 0x181)
i32 CWarlord::ResolveIdleAnimation() {
    if (m_a8 != 0) {
        return 0;
    }

    i32 idx = GruntRand() % 3 + 1;

    CGruntzMgr* g = g_gameReg;
    if (g->m_134 == 1) {
        CWwdGameObjectA* h = m_object;
        // the cue id is a LOCAL computed before the viewport test: retail hoists
        // `lea ebx,[edi+0x431]` above the four compares (and pays a `push ebx` for it),
        // exactly as in ResolveBattlecryAnimation.
        i32 cue = idx + 0x431;
        i32 x = h->m_screenX;
        i32 y = h->m_screenY;
        if (x < g->m_viewBounds.right && x >= g->m_viewBounds.left && y < g->m_viewBounds.bottom
            && y >= g->m_viewBounds.top) {
            g->m_cueSink->SpawnVoiceDriver(h->m_188, cue, -1, -1, -1);
        }
    } else {
        g->m_cueSink->SpawnVoiceDriver(m_object->m_188, idx + 0x43b, -1, -1, -1);
    }

    CAniElement* anim = m_idleAnims[idx];
    m_value = m_38->m_1a0.m_14;
    m_38->m_1a0.Setup(anim);

    CAniElement* desc = m_38->m_1a0.m_14;
    CAniDesc* elem =
        desc->m_records.GetSize() > 0 ? static_cast<CAniDesc*>(desc->m_records.GetAt(0)) : 0;
    i32 frame = elem->m_param;

    m_38->ApplyLookupSprite(s_GRUNTZ_ + m_54 + s__IDLE, frame);

    m_prevAnimSetNode = m_objAux->m_1c;
    m_objAux->m_1c = ActFindId(s_keyA);
    return 1;
}

// @early-stop
// 90.3% -> better: the voice-cue id is a LOCAL computed before the viewport test (retail
// hoists `lea ebx,[idx+0x42e]` above the four compares, which costs it a `push ebx`).
// Residue is the same Setup-pair temp phase as ResolveMovingAnimation @0x45100.
RVA(0x00045b60, 0x161)
i32 CWarlord::ResolveBattlecryAnimation() {
    if (m_a8 != 0) {
        return 0;
    }

    i32 idx = GruntRand() % 3;

    CGruntzMgr* g = g_gameReg;
    if (g->m_134 == 1) {
        CWwdGameObjectA* h = m_object;
        i32 cue = idx + 0x42e;
        i32 x = h->m_screenX;
        i32 y = h->m_screenY;
        if (x < g->m_viewBounds.right && x >= g->m_viewBounds.left && y < g->m_viewBounds.bottom
            && y >= g->m_viewBounds.top) {
            g->m_cueSink->SpawnVoiceDriver(h->m_188, cue, -1, -1, -1);
        }
    } else {
        g->m_cueSink->SpawnVoiceDriver(m_object->m_188, idx + 0x438, -1, -1, -1);
    }

    CAniElement* anim = m_battlecryAnims[idx];
    m_value = m_38->m_1a0.m_14;
    m_38->m_1a0.Setup(anim);

    m_38->ApplyName(s_GRUNTZ_ + m_54 + s__BATTLECRY);

    m_prevAnimSetNode = m_objAux->m_1c;
    m_objAux->m_1c = ActFindId(s_keyF);
    return 1;
}
