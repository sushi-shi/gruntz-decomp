// Warlord.cpp - the ORIGINAL warlord TU (RTTI .?AVCWarlord@@), a CUserLogic-
// derived leaf, PLUS the five anim-resolver methods currently labeled CGrunt::
// Resolve*Animation - the retail obj spans 0x42d40-0x45cc1 and those five
// resolvers are TEXT-WOVEN into it (wave3-I grunt-region partition):
//   * text A-B-A: ResolveMovingAnimation @0x45100 sits BETWEEN
//     BuildFortSplashParticles @0x44f80 and NotifyFortUnderAttack @0x45270 -
//     impossible for separate objs at first link.
//   * private .data weave: the warlord ctor's statics band (0x20d218-0x20d374)
//     interleaves cell-by-cell with the resolvers' statics (0x20d220 = ctor +
//     0x45100; 0x20d234 = ctor + 0x457b0; 0x20d22c = ctor + 0x455f0/0x48470/
//     0x49c60; 0x20d36c/0x20d374 = 0x45960/0x45b60) ahead of the fortressflag
//     ctor's band (0x20d384).
//   * init frag i302 @0x445a0 (before InitActReg @0x445c0) is this obj's.
// @identity-TODO: the resolvers' mangled owner (CGrunt) is the historical
// attribution; Warlord.h declares same-named CWarlord methods that the warlord
// handlers call (reloc-masked aliases of the SAME retail bodies). Whether the
// original class was CGrunt (shared layout) or CWarlord is unresolved - the
// byte match is owner-independent; kept as-is pending an identity pass.
//
// CUserBase / CUserLogic / EngStr / CGameObject come from <Gruntz/UserLogic.h>;
// MFC CString from <Mfc.h>. Engine callees/globals are reloc-masked (no body).
#include <Gruntz/Warlord.h>
#include <Gruntz/AniAdvanceCursor.h>
#include <Gruntz/ActReg.h>            // the shared CActReg (g_actionTable @0x644610)
#include <Gruntz/TypeKeyColl.h>       // the shared CTypeKeyColl (g_typeColl @0x6bf650)
#include <Gruntz/Grunt.h>             // CGrunt + CGruntHud/g_buteTree/GruntRand
#include <DDrawMgr/DDrawSurfaceMgr.h> // m_38->m_0c (the world root)
#include <DDrawMgr/DDrawSubMgrLeaf.h> // m_0c->m_animRegistry (the anim-key catalog; Lookup 0x1b8438)
#include <DDrawMgr/AniAdvance.h>      // CAniDesc (the descriptor record; ex CAnimElem)
                                      // (the five Resolve*Animation bodies below)
#include <Gruntz/AniElement.h>        // full CAniElement (ResolveIdleAnimation's desc walk)
#include <Gruntz/TriggerMgr.h>     // CTriggerMgr::NearestCellDist (0x7d1d0) - the m_cmdGrid helper
#include <Gruntz/GruntzMgr.h>      // CGruntzMgr (the RTTI-true singleton; ReportError @0x8dc60)
#include <Gruntz/SpriteRefTable.h> // CSpriteRefTable::GetSel (g_gameReg->m_spriteFactory)
#include <Gruntz/State.h> // CState::BuildAssetNamespacePrefixes (ex CNamespaceLoader facet, m_curState)

#include <Bute/ButeTree.h> // the real CButeTree (g_buteTree @0x6bf620)

#include <rva.h>

// The five anim-resolvers' key-string statics (this TU's own .data copies - the
// 0x20d218-0x20d374 private band the warlord ctor shares).
static const char s_GRUNTZ_[] = "GRUNTZ_";
static const char s__MOVING[] = "_MOVING";
static const char s__DEATH[] = "_DEATH";
static const char s__JOY[] = "_JOY";
static const char s__IDLE[] = "_IDLE";
static const char s__BATTLECRY[] = "_BATTLECRY";
// The ctor's own eleven per-state anim-key suffixes + the four owner-name prefixes
// (the same 0x20d218-0x20d374 private .data band, reloc-masked).
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
#include <Wap32/ZDArrayDerived.h>

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

// The Gruntz type-registry globals (.data). g_buteTree (the real shared CButeTree)
// maps an action-key string to a 1-based type id (0 = absent, via Find/Insert);
// g_typeColl is the parallel growable key collection; g_actionTable holds the
// per-type action-handler pointer slots.

// CTypeKeyColl (SetAtGrow == grow + assign, inlined in retail) is the shared
// <Gruntz/TypeKeyColl.h> shape.

// g_actionTable (CActionTable @0x644610) is declared above, near InitActReg.

// The six action-type handler entry points (reloc-masked code addresses; their
// mid-function LAB_ addresses are stored as raw dispatch pointers).
extern "C" void Act_A(); // 0x403ba7
extern "C" void Act_B(); // 0x401ce9
extern "C" void Act_C(); // 0x4024f0
extern "C" void Act_D(); // 0x403422
extern "C" void Act_E(); // 0x40431d
extern "C" void Act_F(); // 0x402725

// Find-or-create one action-key -> handler binding. Retail INLINES all six blocks
// (SetAtGrow is expanded to IndexToPtr + the placement-new grow loop + the CString
// key assign); a macro forces the 6x inlining cl declines for a helper this large.
// The placement-new null guard (`if (p) ctor(p)`) is retail's `test esi,esi; je`.
#define REGISTER_ACTION(key, handler)                                                              \
    do {                                                                                           \
        i32 id_ = (i32)g_buteTree.Find(key);                                                       \
        if (id_ == 0) {                                                                            \
            g_buteTree.Insert(key, (void*)g_typeCounter);                                          \
            id_ = g_typeCounter;                                                                   \
            CString* slot_ = (CString*)((_zvec*)&g_typeColl)->IndexToPtr(id_);                     \
            CString* p_ = (CString*)g_typeColl.m_alloc;                                            \
            for (i32 n_ = g_typeColl.m_grown; n_--; p_++) {                                        \
                ::new ((void*)p_) CString;                                                         \
            }                                                                                      \
            *slot_ = key;                                                                          \
            ++g_typeCounter;                                                                       \
        }                                                                                          \
        void** aslot_ = (void**)((_zvec*)&g_actionTable)->IndexToPtr(id_);                         \
        *aslot_ = (void*)(handler);                                                                \
    } while (0)
// ===========================================================================
// CWarlord::~CWarlord  (0x0107f0)
// ===========================================================================
// CWarlord adds one destructible member past the CUserLogic base - the +0x54
// CString. The empty body lets MSVC emit the canonical most-derived teardown:
//   1. ~CString(m_54)                      (retail EH state 1)
//   2. store the CUserLogic vptr (0x5e705c); inline-destruct the +0x18 link's
//      ~EngStr                             (retail EH state 2)
//   3. store the CUserBase vptr (0x5e70b4)
// The destructible members force the /GX frame. The empty body is enough; the
// teardown BODY (lea+call x2 + the two base vptr stores) is byte-identical.
//
// @early-stop
// Two intersecting /GX EH-machine walls, no source lever (body is byte-exact):
//   (a) eh-dtor-vptr-restamp-presence.md - because the FIRST destruction is the
//       leaf's OWN +0x54 CString (vs the single-destructible leaves whose first
//       destruction is the base +0x18 link), cl re-stamps ??_7CWarlord at entry;
//       retail elided that store entirely.
//   (b) eh-state-numbering-base.md - retail numbers the two trylevels 1/2 over a
//       deeper frame (extra `push ecx`, state slot [esp+0x10], add esp,0x10),
//       ours uses 0/1 over [esp+0xc] / add esp,0xc. A cl EH-state-machine choice
//       for a 2-destructible-member dtor; neither member order nor the vptr model
//       (polymorphic vs manual) flips it. ~74%, deferred to the final sweep.
RVA(0x000107f0, 0x55)
CWarlord::~CWarlord() {}

// ===========================================================================
// CWarlord::CWarlord(int)  (0x042d40)  - the warlord ctor
// ===========================================================================
// Folds the shared CUserLogic(obj) base init (base vptr, the +0x18 link, the
// empty-string threat name, the one-shot logic-type table, the three built-in
// handler registrations), seeds the tile-logic tail + the two cooldown timers,
// then: snaps the bound object onto the tile grid, latches its warlord anim id,
// resolves a per-owner sprite selector, and a 4-way switch over the owner index
// names the warlord ("WARLORDZ_KING".."WARLORDZ_VIKING") + its battle-event tag.
// For each of the eleven per-state keys ("GRUNTZ_<owner>_<state>") it looks the
// handle up in the object's embedded name->handle map and stashes it (m_58..m_80).
// Finally re-zeros the second timer and resolves the initial moving animation.
// The +0x18 throwing link + the destructible m_54 CString drive the /GX EH frame.
//
// The owner index (the bound object's m_124) selects both the per-owner focus-slot
// config row and the switch arm.
typedef enum WarlordOwner {
    WARLORDZ_KING = 0,
    WARLORDZ_NAPOLEAN = 1,
    WARLORDZ_PATTON = 2,
    WARLORDZ_VIKING = 3,
} WarlordOwner;

// The per-owner warlord battle-event tag stored at m_ownerTag (retail 0x442..0x445).
typedef enum WarlordBattleTag {
    WARLORD_TAG_KING = 0x442,
    WARLORD_TAG_NAPOLEAN = 0x443,
    WARLORD_TAG_PATTON = 0x444,
    WARLORD_TAG_VIKING = 0x445,
} WarlordBattleTag;

// One unrolled anim-key lookup on the bound object's embedded animation
// name->handle map: the object's typed world slot (CGameObject::m_0c, the
// CDDrawSurfaceMgr) -> m_animRegistry (+0x2c, CDDrawSubMgrLeaf) -> its CMapStringToPtr
// m_10 (retail Lookup 0x1b8438). Build "GRUNTZ_" + m_54 + suffix (two CString temps), look it up
// (out-param zeroed first so a miss stores 0), stash the handle. The chain stays
// in the macro (not a cached local) so cl reloads m_38 per unrolled lookup, as retail.
#define WARLORD_ANIM_LOOKUP(dst, suffix)                                                           \
    {                                                                                              \
        void* h = 0;                                                                               \
        m_38->m_0c->m_animRegistry->m_10.Lookup(s_GRUNTZ_ + m_54 + (suffix), h);                   \
        dst = (CAniElement*)h;                                                                     \
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
//     (cl: `and ecx,-0x20`), the m_10 reload-vs-cache choices, and the bl EH-const.
//   * emission order: retail seeds the tile-tail (m_34/m_38/m_3c) BEFORE the m_54
//     CString member ctor; with `: CUserLogic` + a body TILE_LOGIC_SEED the seed
//     necessarily emits AFTER the member ctor. Deriving CWarlord from the byte-
//     neutral CTileLogic intermediate (its ctor seeds the tail before the member
//     ctor) recovers exactly this order and measured +1.4% (79.15 -> 80.53) - an
//     inheritance change owned by the Fable lane; left as a hand-off (see report).
RVA(0x00042d40, 0x73e)
// NB the arg is `int` in retail's mangling (??0CWarlord@@QAE@H@Z), not CGameObject* -
// the 1997 dev declared the ctor `CWarlord(int)` and used the int as the bound game
// object handle; the cast to CGameObject* reproduces that (kept i32 so the mangled
// symbol still binds to 0x42d40). Sibling leaf ctors (CGruntVoice, ...) took a real
// CGameObject*; this one did not.
CWarlord::CWarlord(i32 arg) : CUserLogic((CGameObject*)arg) {
    CGameObject* obj = (CGameObject*)arg;
    TILE_LOGIC_SEED(obj);

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
    if (m_object->m_latchedAnimId != 0xc3500) {
        m_object->m_latchedAnimId = 0xc3500;
        m_object->m_flags |= 0x20000;
    }
    m_38->m_flags |= 0x2000002;

    // Resolve the per-owner sprite selector from the focus-slot config row (clamped to
    // [0,0x11); fall back to row 1 when the selector resolves empty).
    i32 owner = m_object->m_124;
    i32 cfg = g_gameReg->m_focusSlots[owner].m_08;
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
            ((CGruntzMgr*)g_gameReg)->ReportError(0x8009, 0x3e9);
            return;
    }

    // Register the warlord's asset namespace, then resolve every per-state handle.
    g_gameReg->m_curState->BuildAssetNamespacePrefixes(m_54, 1, 0, 0);

    WARLORD_ANIM_LOOKUP(m_animIdle1, s__IDLE1);
    WARLORD_ANIM_LOOKUP(m_animIdle2, s__IDLE2);
    WARLORD_ANIM_LOOKUP(m_animIdle3, s__IDLE3);
    WARLORD_ANIM_LOOKUP(m_animIdle4, s__IDLE4);
    WARLORD_ANIM_LOOKUP(m_animBattlecry1, s__BATTLECRY1);
    WARLORD_ANIM_LOOKUP(m_animBattlecry2, s__BATTLECRY2);
    WARLORD_ANIM_LOOKUP(m_animBattlecry3, s__BATTLECRY3);
    WARLORD_ANIM_LOOKUP(m_animJoy, s__JOY);
    WARLORD_ANIM_LOOKUP(m_animDeath, s__DEATH);
    WARLORD_ANIM_LOOKUP(m_animMoving, s__MOVING);
    WARLORD_ANIM_LOOKUP(m_animPanic, s__PANIC);

    m_timer2StampLo = 0;
    m_timer2WindowLo = 0;
    m_timer2StampHi = 0;
    m_timer2WindowHi = 0;
    m_a8 = 0;
    ((CGrunt*)this)->ResolveMovingAnimation();
}
#undef WARLORD_ANIM_LOOKUP

// @early-stop  (STUB - kept at 0% rather than regress; see the FRAME WALL below)
// 0x43670 = CWarlord::SerializeMove (vtable slot 1, +0x4; origin CUserBase). Homed
// from src/Stub/GapFunctions.cpp (matcher-5); attribution vtable-proven (??_7CWarlord
// +0x4). A 3104-byte archive save/load round-trip.
//
// FULLY DECODED (R3, this session) - the complete body is understood; it is NOT a
// blind stub. Signature: i32 SerializeMove(CGruntArchive* ar, i32 mode, i32 a3, i32 a4)
// where ar == CFileMemBase (Read @vtbl+0x2c / Write @vtbl+0x30), a4 is the referenced
// object (int in the mangling, a CGameObject*). Structure:
//   1. if (!((CMovingLogicBase*)this)->Serialize(ar,mode,a3,a4)) return 0;   (0x16e7f0)
//   2. if (!ar) return 0;   (retail SHARES this ret-0 with the save-body null check @43c5c)
//   3. header field (m_40 handle + m_44 0x10 blob):
//        mode 7 LOAD : ar->Read(hbuf,0x80); ar->Read(&m_44,0x10); m_34=m_38=a4;
//                      m_3c=a4->m_7c; m_40 = strlen(hbuf)? reg->m_10.Lookup(hbuf):0
//        mode 4 SAVE : memset(buf); if(m_40) strcpy(buf, reg->KeyOfValue_152d30(m_40));
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
//   ->m_0c (CDDrawSurfaceMgr) ->m_animRegistry (CDDrawSubMgrLeaf) - its ::CMapStringToPtr m_10
//   forward-Lookups a key (0x1b8438) and KeyOfValue_152d30 (RVO CString) reverses it.
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
i32 Gap_043670(void) {
    return 0;
}

// The file-static per-action handler dispatch array (g_actionTable @0x644610) is
// the shared CActReg archetype (<Gruntz/ActReg.h>): InitActReg builds it over the
// fixed [2000, 2010] range via Construct (0x408710); Lookup (0x3864) resolves a
// per-type slot (used by RegisterWarlordActions below).
DATA(0x00244610)
CActReg g_actionTable; // 0x644610 (owner-TU definition; its 0x24-byte CActReg extent
                       // covers interior fields 0x244614..0x244630, bind as g_obj+offset)

// ===========================================================================
// CWarlord::InitActReg  (0x0445c0)
// ===========================================================================
// Construct the file-static per-action handler table (g_actionTable @0x644610)
// over the fixed range [2000, 2010] via the shared registry ctor (0x408710).
// Free init thunk; the SAME archetype as the eyecandy classes' InitActReg.
RVA(0x000445c0, 0x15)
void CWarlord::InitActReg() {
    ((CZDArrayDerived*)&g_actionTable)->Construct(2000, 2010);
}

// ===========================================================================
// CWarlord::ResolveState  (0x044640)  - slot-4 override (the animation dispatcher)
// ===========================================================================
// Resolve the per-state handler slot for `key` in the g_actionTable registry, and
// if it holds a handler, dispatch it on `this`. The lookup is the shared
// CActReg::ResolveEntry (fast [lo,hi] range -> slow Find -> Insert rebuild); it is
// side-effecting (seeds m_scratch, the Insert breadcrumb), so cl inlines it TWICE -
// once to test `*slot != 0`, once to fetch the slot for dispatch. When the slot is
// empty, the ResolveEntry return pointer falls straight out in eax as the result.
RVA(0x00044640, 0x102)
i32 CWarlord::ResolveState(i32 key) {
    void** slot = (void**)g_actionTable.ResolveEntry(key);
    if (*slot != 0) {
        // the handler is a __thiscall dispatched on this warlord (`mov ecx,this;
        // call [slot2]`); a complete-class PMF gives the plain 4-byte code-ptr call.
        typedef i32 (CWarlord::*StateHandler)();
        StateHandler h = *(StateHandler*)g_actionTable.ResolveEntry(key);
        return (this->*h)();
    }
    return (i32)slot;
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

// ===========================================================================
// CWarlord::RearmMoving  (0x044bb0)
// ===========================================================================
// Re-arm the geometry sub-player at m_38->m_1a0 against the global default geo
// source (result discarded). Then, if the sub's state words say it is ready to
// move (m_28 != 0 && m_20 == 0), resolve the moving animation. Returns 0.
RVA(0x00044bb0, 0x38)
i32 CWarlord::RearmMoving() {
    m_38->m_1a0.Advance(g_engineFrameDelta);
    CWarlordAnimSub* sub = (CWarlordAnimSub*)((char*)m_38 + 0x1a0);
    if (sub->m_28 != 0 && sub->m_20 == 0) {
        ((CGrunt*)this)->ResolveMovingAnimation();
    }
    return 0;
}

// ===========================================================================
// CWarlord::LoadAttributes  (0x044c00)  - the warlord's per-tick threat update
// ===========================================================================
// Re-arm the geometry sub-player off the global geo source (bail if not ready);
// in multiplayer, measure the nearest-enemy distance vs the "Warlordz/PanicRadius"
// config (default 64) and raise the fort alert when inside the radius; otherwise,
// past the 64-bit cooldown window, randomly resolve an idle / battlecry animation.
// Returns int 0 on every path. Plain /O2 leaf (no destructible local, no /GX use).
RVA(0x00044c00, 0xc6)
i32 CWarlord::LoadAttributes() {
    if (m_38->m_1a0.Advance(g_engineFrameDelta) != 1) {
        return 0;
    }

    CGameRegistry* reg = g_gameReg;
    if (reg->m_134 != 1) {
        CGameObject* o = m_object;
        i32 dist =
            ((CTriggerMgr*)reg->m_cmdGrid)->NearestCellDist(o->m_124, o->m_screenX, o->m_screenY);
        if (dist < g_buteMgr.GetIntDef("Warlordz", "PanicRadius", 0x40)) {
            NotifyFortUnderAttack();
            return 0;
        }
    }

    if (static_cast<i64>(static_cast<u32>(g_frameTime)) - *(i64*)&m_cooldownStampLo >= *(i64*)&m_cooldownWindowLo) {
        if (rand() % 10 < 5) {
            ((CGrunt*)this)->ResolveIdleAnimation();
            return 0;
        }
        ((CGrunt*)this)->ResolveBattlecryAnimation();
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

    CGameRegistry* reg = g_gameReg;
    if (reg->m_134 != 1) {
        CGameObject* o = m_object;
        i32 dist =
            ((CTriggerMgr*)reg->m_cmdGrid)->NearestCellDist(o->m_124, o->m_screenX, o->m_screenY);
        if (dist >= g_buteMgr.GetIntDef("Warlordz", "PanicRadius", 0x40)) {
            RaiseBattleAlert();
            return 0;
        }
    } else {
        if (((CWarlordMission*)reg->m_curState)->m_objective->m_4c == 0) {
            ((CGrunt*)this)->ResolveMovingAnimation();
            return 0;
        }
        if (static_cast<i64>(static_cast<u32>(g_frameTime)) - *(i64*)&m_cooldownStampLo >= *(i64*)&m_cooldownWindowLo) {
            ((CRegBattleEvent*)reg->m_cueSink)->PostBattleEvent(m_object->m_188, 0x436, -1, -1, -1);
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
    CWarlordAnimSub* sub = (CWarlordAnimSub*)((char*)m_38 + 0x1a0);
    if (sub->m_28 == 0 || sub->m_20 != 0) {
        return 0;
    }
    CRegThreatHelper* h = (CRegThreatHelper*)g_gameReg->m_cmdGrid;
    if (h->m_288 != 0 && m_object->m_124 == g_curPlayer) {
        h->m_2a0 = 0;
        CRegThreatHelper* h2 = (CRegThreatHelper*)g_gameReg->m_cmdGrid;
        h2->m_window = 0x3e8;
        h2->m_stamp = static_cast<u32>(g_frameTime);
    }
    ((CGrunt*)this)->ResolveMovingAnimation();
    return 0;
}

// ===========================================================================
// CWarlord::RearmMoving2  (0x044f30)
// ===========================================================================
// A second per-state moving-anim re-arm handler dispatched from the warlord
// anim-state table; the body is byte-identical to RearmMoving (0x44bb0): re-arm
// the +0x1a0 geo sub-player off the global geo source, then resolve the moving
// animation when the sub's state words say it is ready. Returns 0.
RVA(0x00044f30, 0x38)
i32 CWarlord::RearmMoving2() {
    m_38->m_1a0.Advance(g_engineFrameDelta);
    CWarlordAnimSub* sub = (CWarlordAnimSub*)((char*)m_38 + 0x1a0);
    if (sub->m_28 != 0 && sub->m_20 == 0) {
        ((CGrunt*)this)->ResolveMovingAnimation();
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
//   CGameObject* o = m_10; i32 x=o->m_5c, y=o->m_60;
//   if (x in [reg->m_13c, reg->m_144) && y in [reg->m_140, reg->m_148)) {
//     spr = reg->m_30->m_08->CreateSprite(0, x-30, y+10, 0xcf84f, "..."@0x60a96c, 0x40003);
//     if (spr) { spr->ApplyName("..."@0x60d30c);    // 0x150540
//               spr->ApplyLookupGeometry("..."@0x60d30c, 0); } // 0x1505b0
//   }
//   ... panic sub reg->m_68 (m_288/m_2a0/m_290 timer arm to 0x3e8/g_frameTime) ...
//   ... reg->m_150[o->m_124 * 0x48].m_0c = 0 (owner-array slot) ...
// @early-stop
// DEFERRED to the final sweep WITH the sprite models: the effect spawn is
// CDDrawChildGroup::CreateSprite (0x1597b0) + CGruntSprite::CacheFirstFrame (0x150540)
// + CGruntAnimPlayer::ApplyLookupGeometry (0x1505b0) - all in the sprite-worker-owned
// spriteresource domain (excluded from this brief). Best reconstructed alongside
// those class models; homed so the RVA is labeled.
RVA(0x00044f80, 0x127)
void CWarlord::BuildFortSplashParticles() {}

// ---------------------------------------------------------------------------
// CGrunt::ResolveMovingAnimation()  (0x045100)  [moved from Grunt.cpp - this
// obj's text; see the file header]
// Gate: m_animResolved == 0 (else return 0). Feed key "GRUNTZ_<type>_MOVING" + geometry
// m_movingGeoSrc into the player; look up tree key "B"; then randomize the move-start time
// (m_moveStartTime = (rand()%0x5dc1 + 0x1770)*10) and seed m_moveSeed/m_moveTimeHi/m_moveSeedHi.
RVA(0x00045100, 0x112)
i32 CGrunt::ResolveMovingAnimation() {
    if (m_animResolved != 0) {
        return 0;
    }

    m_38->ApplyName(s_GRUNTZ_ + TypeName() + s__MOVING);

    m_activeAnimDesc = m_38->m_1a0.m_14;
    m_38->m_1a0.Setup_15c2d0(m_movingGeoSrc);

    m_prevAnimSetNode = m_14->m_1c;
    m_14->m_1c = g_buteTree.Find(s_keyB);

    m_moveStartTime = (GruntRand() % 0x5dc1 + 0x1770) * 10;
    m_moveSeedHi = 0;
    m_moveSeed = g_movingSeed;
    m_moveTimeHi = 0;
    return 1;
}

// ===========================================================================
// CWarlord::NotifyFortUnderAttack  (0x045270)  - resolve the fort-panic animation
// ===========================================================================
// Raise the fort-under-attack alert + resolve the warlord's panic animation. A /GX
// leaf (frame 0x14; NO SerializeMove-style frame wall). FULLY DECODED (R3), left as a
// STUB only for budget (a >512B multi-subsystem body); the next pass can land it from
// this decode:
//   gate:  if (m_a8 != 0) return 0;
//          if (strcmp(*(char**)g_typeColl.IndexToPtr(m_14->m_1c), s_codeD) == 0) return 0;
//                                              // 0x310f0 IndexToPtr; skip if already "D"/death
//   cue + cooldown:
//     if (g_gameReg->m_134 == 1) {            // on-screen / single-player
//         g_gameReg->m_cueSink->Cue(m_object->m_188, 0x436, -1,-1,-1);   // 0x11b7c0
//         m_cooldownWindowLo = 0x7530; m_cooldownWindowHi = 0; m_cooldownStampLo = g_frameTime;
//     } else {                                 // multiplayer
//         if ((i64)(u32)g_frameTime - *(i64*)&m_timer2StampLo >= *(i64*)&m_timer2WindowLo
//             && ((CRegThreatHelper*)g_gameReg->m_cmdGrid)->m_2a0 == this) {
//             g_gameReg->m_cueSink->Cue(m_object->m_188, 0x440, -1,-1,-1);
//             if ((g_notifyShownFlag & 1) == 0) {                        // g_6445bc, one-shot
//                 g_notifyShownFlag |= 1;
//                 g_notifyAlertStr = CString("ALERT - Your Fort is under attack"@0x60d340); // 0x1b9d4c @ 0x6446fc
//                 FUN_0011f490(&LAB_004455d0);                            // 0x11f490 (queue the banner)
//             }
//             CString notifyMsg; g_buteMgr.GetStringDef("Warlordz","NotifyString",&notifyMsg); // 0x173180
//             ((CFontConfig*)g_gameReg->m_5c)->AddItem(notifyMsg, ...);   // 0x21c60 HUD line
//             m_timer2WindowLo = g_buteMgr.GetIntDef("Warlordz","NotifyTimer",0x1770); // 0x171aa0
//             m_timer2WindowHi = 0; m_timer2StampLo = g_frameTime; m_timer2StampHi = 0;
//         }
//         // (MP cooldown re-arm:) m_cooldownWindowLo = (GruntRand()%0x5dc1 + 0x1770)*10; ...
//     }
//     m_cooldownStampHi = 0;
//   resolve panic anim (shared tail):
//     m_activeAnimDesc = m_38->m_1a0.m_14;
//     m_38->m_1a0.Setup_15c2d0((i32)m_animPanic);                          // 0x15c2d0
//     m_38->ApplyName(s_GRUNTZ_ + m_54 + s__PANIC);                       // 0x150540
//     m_prevAnimSetNode = m_14->m_1c; m_14->m_1c = g_buteTree.Find(s_codeD);  // 0x16d190
//     return 1;
// New models the next pass needs (lean reloc-masked shells + externs): CFontConfig::
// AddItem @0x21c60, FUN_0011f490, the global CString g_notifyAlertStr @0x6446fc + its
// ctor 0x1b9d4c, the one-shot byte g_notifyShownFlag @0x6445bc, and s_codeD (shared
// ?s_codeD@@3PADA @0x60cca4, "D"). g_typeColl/g_buteMgr/g_buteTree/cueSink are already
// modeled. Risk: the inline strcmp gate + the two-way (SP/MP) cooldown-store schedule.
RVA(0x00045270, 0x2a8)
void CWarlord::NotifyFortUnderAttack() {}

// ---------------------------------------------------------------------------
// CGrunt::ResolveDeathAnimation()  (0x0455f0)  [moved from Grunt.cpp]
// Gate: m_animResolved == 0 (else return 0); then latch m_animResolved = 1. Fire the on-screen cue
// (arg2 = m_deathCueArg), feed geometry m_deathGeoSrc then key "GRUNTZ_<type>_DEATH", look up "C".
RVA(0x000455f0, 0x15b)
i32 CGrunt::ResolveDeathAnimation() {
    if (m_animResolved != 0) {
        return 0;
    }
    m_animResolved = 1;

    CGameRegistry* g = g_gameReg;
    if (g->m_134 == 1) {
        CGameObject* h = m_10;
        i32 x = h->m_screenX;
        i32 y = h->m_screenY;
        if (x < g->m_viewOriginR && x >= g->m_viewOriginL && y < g->m_viewOriginB
            && y >= g->m_viewOriginT) {
            g->m_cueSink->Cue(h->m_188, m_deathCueArg, -1, -1, -1);
        }
    } else {
        g->m_cueSink->Cue(m_10->m_188, m_deathCueArg, -1, -1, -1);
    }

    m_activeAnimDesc = m_38->m_1a0.m_14;
    m_38->m_1a0.Setup_15c2d0(m_deathGeoSrc);

    m_38->ApplyName(s_GRUNTZ_ + TypeName() + s__DEATH);

    m_prevAnimSetNode = m_14->m_1c;
    m_14->m_1c = g_buteTree.Find(s_keyC);
    return 1;
}

// ---------------------------------------------------------------------------
// CGrunt::ResolveAnimation()  (0x0457b0, generic "_JOY")  [moved from Grunt.cpp]
// Gate: m_animResolved == 0 (else return 0). The cue arg2 is a fixed constant (0x435 when
// on-screen / 0x43f otherwise). Geometry m_joyGeoSrc; key "GRUNTZ_<type>_JOY"; look "E".
RVA(0x000457b0, 0x14c)
i32 CGrunt::ResolveAnimation() {
    if (m_animResolved != 0) {
        return 0;
    }

    CGameRegistry* g = g_gameReg;
    if (g->m_134 == 1) {
        CGameObject* h = m_10;
        i32 x = h->m_screenX;
        i32 y = h->m_screenY;
        if (x < g->m_viewOriginR && x >= g->m_viewOriginL && y < g->m_viewOriginB
            && y >= g->m_viewOriginT) {
            g->m_cueSink->Cue(h->m_188, 0x435, -1, -1, -1);
        }
    } else {
        g->m_cueSink->Cue(m_10->m_188, 0x43f, -1, -1, -1);
    }

    m_activeAnimDesc = m_38->m_1a0.m_14;
    m_38->m_1a0.Setup_15c2d0(m_joyGeoSrc);

    m_38->ApplyName(s_GRUNTZ_ + TypeName() + s__JOY);

    m_prevAnimSetNode = m_14->m_1c;
    m_14->m_1c = g_buteTree.Find(s_keyE);
    return 1;
}

// ---------------------------------------------------------------------------
// CGrunt::ResolveIdleAnimation()  (0x045960)  [moved from Grunt.cpp]
// Gate: m_animResolved == 0 (else return 0). Pick idx = rand()%3 + 1 (1..3); cue arg2 =
// idx+0x431 / idx+0x43b; geometry m_idleGeoSrc[idx]; then read the active-anim
// descriptor's first element's m_14 as a 2nd lookup arg (SetAnimEx); key
// "GRUNTZ_<type>_IDLE"; look up "A".
RVA(0x00045960, 0x181)
i32 CGrunt::ResolveIdleAnimation() {
    if (m_animResolved != 0) {
        return 0;
    }

    i32 idx = GruntRand() % 3 + 1;

    CGameRegistry* g = g_gameReg;
    if (g->m_134 == 1) {
        CGameObject* h = m_10;
        i32 x = h->m_screenX;
        i32 y = h->m_screenY;
        if (x < g->m_viewOriginR && x >= g->m_viewOriginL && y < g->m_viewOriginB
            && y >= g->m_viewOriginT) {
            g->m_cueSink->Cue(h->m_188, idx + 0x431, -1, -1, -1);
        }
    } else {
        g->m_cueSink->Cue(m_10->m_188, idx + 0x43b, -1, -1, -1);
    }

    m_activeAnimDesc = m_38->m_1a0.m_14;
    m_38->m_1a0.Setup_15c2d0(m_idleGeoSrc[idx]);

    CAniElement* desc = m_38->m_1a0.m_14;
    CAniDesc* elem = desc->m_records.m_nSize > 0 ? (CAniDesc*)*desc->m_records.m_pData : 0;
    i32 frame = elem->m_param;

    m_38->ApplyLookupSprite(s_GRUNTZ_ + TypeName() + s__IDLE, frame);

    m_prevAnimSetNode = m_14->m_1c;
    m_14->m_1c = g_buteTree.Find(s_keyA);
    return 1;
}

// ---------------------------------------------------------------------------
// CGrunt::ResolveBattlecryAnimation()  (0x045b60)  [moved from Grunt.cpp]
// Gate: m_animResolved == 0 (else return 0). Pick idx = rand()%3 (0..2); cue arg2 =
// idx+0x42e / idx+0x438; geometry m_battlecryGeoSrc[idx]; key "GRUNTZ_<type>_BATTLECRY";
// look up "F".
RVA(0x00045b60, 0x161)
i32 CGrunt::ResolveBattlecryAnimation() {
    if (m_animResolved != 0) {
        return 0;
    }

    i32 idx = GruntRand() % 3;

    CGameRegistry* g = g_gameReg;
    if (g->m_134 == 1) {
        CGameObject* h = m_10;
        i32 x = h->m_screenX;
        i32 y = h->m_screenY;
        if (x < g->m_viewOriginR && x >= g->m_viewOriginL && y < g->m_viewOriginB
            && y >= g->m_viewOriginT) {
            g->m_cueSink->Cue(h->m_188, idx + 0x42e, -1, -1, -1);
        }
    } else {
        g->m_cueSink->Cue(m_10->m_188, idx + 0x438, -1, -1, -1);
    }

    m_activeAnimDesc = m_38->m_1a0.m_14;
    m_38->m_1a0.Setup_15c2d0(m_battlecryGeoSrc[idx]);

    m_38->ApplyName(s_GRUNTZ_ + TypeName() + s__BATTLECRY);

    m_prevAnimSetNode = m_14->m_1c;
    m_14->m_1c = g_buteTree.Find(s_keyF);
    return 1;
}

// class-metadata SIZE sweep (misc-Gruntz A-C): matching-neutral, hosted at
// .cpp EOF (see docs/class-metadata-sweep-log.md). SIZE_UNKNOWN = size not yet pinned.
SIZE_UNKNOWN(CRegBattleEvent);
SIZE_UNKNOWN(CRegThreatHelper);
SIZE_UNKNOWN(CWarlordAnimSub);
SIZE_UNKNOWN(CWarlordMission);
SIZE_UNKNOWN(CWarlordObjective);
