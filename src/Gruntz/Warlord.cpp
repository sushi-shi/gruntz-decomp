// Warlord.cpp - the AI fort-warlord game object (RTTI .?AVCWarlord@@), a
// CUserLogic-derived leaf. Home TU for three __thiscall methods, in ascending
// retail-RVA order:
//   0x0107f0  ~CWarlord       the dtor: destruct the +0x54 CString, then the
//                             CUserLogic base (link ~EngStr + base vptr stores).
//                             The destructible members force the /GX EH frame.
//   0x044640  ResolveState    slot-4 override: a bounds-checked lookup into a
//                             file-static CArray of per-state handler vtables,
//                             growing it on miss, then dispatching `(*elem)(this)`.
//   0x044bb0  RearmMoving      re-arm the geometry sub-player off the global geo
//                             source, then resolve the moving animation when the
//                             sub's state words say so; returns 0.
//
// CUserBase / CUserLogic / EngStr / CGameObject come from <Gruntz/UserLogic.h>;
// MFC CString from <Mfc.h>. Engine callees/globals are reloc-masked (no body).
#include <Gruntz/Warlord.h>
#include <Gruntz/AniAdvanceCursor.h>
#include <Gruntz/ActReg.h>      // the shared CActReg (g_actionTable @0x644610)
#include <Gruntz/TypeKeyColl.h> // the shared CTypeKeyColl (g_typeColl @0x6bf650)

#include <Bute/ButeTree.h> // the real CButeTree (g_buteTree @0x6bf620)

#include <rva.h>

#include <new>      // placement new (the inlined ConstructElements grow loop)
#include <stdlib.h> // rand (CRT PRNG, reloc-masked)
#include <Wap32/ZVec.h>
#include <Wap32/ZDArrayDerived.h>

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
// Builds the CUserLogic base, names the warlord's threat string, then a 4-way
// switch over the owner-index selects the king/napolean/patton/viking config and
// seeds the per-state idle/battlecry/death/moving animation table off the
// "Gruntz" bute group. ~0x73e bytes.
//
// @early-stop
// STUB (>512B, complete-body-OR-regress wall - gx-this-esi-via-cache-store-pressure.md:
// only the COMPLETE body pins this->esi and snaps retail's scopetable prologue
// `push -1; push h; mov eax,fs:0; push eax; sub esp,0x1c; mov esi,ecx`; a partial
// provably REGRESSES, so it stays stubbed pending a dedicated leaf-first pass).
// Structure fully MAPPED (matcher-5) for that pass:
//   1. CUserBaseLink base ctor (0x16d710) at this+0x18.
//   2. this+0xc/+0x10 = the bound game object (link result, edi); +0x14 = [obj+0x7c].
//   3. AddLogicHit/Attack/Bump("Logic{Hit,Attack,Bump}") -> 0x150f50/0x151030/0x151110
//      on this+0x10; then member inits m_28=0x3e9, m_2c=2, m_34/m_38=obj, m_3c=[obj+0x7c];
//      ~CString(m_54); zero m_88..m_a8.
//   4. level object (this+0x10) bit-fixups: [lvl+0x5c] &=~0x1f |=0x10, [lvl+0x60] likewise,
//      [lvl+0x74]=0xc3500 + flag |=0x20000, geo enable, difficulty read at [lvl+0x124].
//   5. 4-way owner switch (WARLORDZ_{KING,NAPOLEAN,PATTON,VIKING}) -> m_ac = 0x442..0x445.
//   6. 11 UNROLLED anim-key lookups: zBitVec key = "GRUNTZ_"+ownerPrefix+suffix over
//      {_IDLE1.._IDLE4,_BATTLECRY1.._BATTLECRY3,_DEATH,_JOY,_MOVING,_PANIC}; resolve via
//      [m_38]->+0xc->+0x2c(+0x10)(key); store handle into m_58,m_5c,..,m_80. Helpers:
//      zBitVec ctor 0x16d3a0(const char*,int) / op= 0x16d2f0 / ~zBitVec 0x16d2a0;
//      ~CString 0x1b9cde (x22 = 2 temps/anim). Trailing panic-timer arm + return this.
RVA(0x00042d40, 0x73e)
CWarlord::CWarlord(i32) {}

// @early-stop
// 0x43670 = CWarlord::SerializeMove (vtable slot 1, +0x4; origin CUserBase). Homed
// from src/Stub/GapFunctions.cpp (matcher-5); attribution vtable-proven (??_7CWarlord
// +0x4). LARGE (3104 B) bute/archive serialize round-trip (uses g_serialCounter): a
// deep switch + engine string-table + inline bute-config expansion. Homed pending a
// dedicated leaf-first pass (>512 B, per-field archive read/write chain bottom-up).
RVA(0x00043670, 0xc20)
i32 Gap_043670(void) {
    return 0;
}

// The file-static per-action handler dispatch array (g_actionTable @0x644610) is
// the shared CActReg archetype (<Gruntz/ActReg.h>): InitActReg builds it over the
// fixed [2000, 2010] range via Construct (0x408710); Lookup (0x3864) resolves a
// per-type slot (used by RegisterWarlordActions below).
DATA(0x00244610)
extern CActReg g_actionTable; // 0x644610

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

// ===========================================================================
// RegisterWarlordActions  (0x0447a0)  - a free function, NOT a CWarlord method
// ===========================================================================
// Registers six single-letter Gruntz action-type keys ("A".."F") into the global
// bute-name -> type-id tree (g_buteTree), growing the parallel type-key string
// collection (g_typeColl, backed by g_typeNodes/g_typeCount) on a miss, then
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
DATA(0x002bf620)
extern CButeTree g_buteTree; // 0x6bf620 (?g_buteTree@@3VCButeTree@@A)

// CTypeKeyColl (SetAtGrow == grow + assign, inlined in retail) is the shared
// <Gruntz/TypeKeyColl.h> shape.
DATA(0x002bf650)
extern CTypeKeyColl g_typeColl; // 0x6bf650 (?g_typeColl@@3UCKSlimeColl@@A)

DATA(0x0021aea8)
extern i32 g_typeCounter; // 0x61aea8 (next free type id)

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
            CString* p_ = (CString*)g_typeColl.m_cursor;                                           \
            for (i32 n_ = g_typeColl.m_count; n_--; p_++) {                                        \
                ::new ((void*)p_) CString;                                                         \
            }                                                                                      \
            *slot_ = key;                                                                          \
            ++g_typeCounter;                                                                       \
        }                                                                                          \
        void** aslot_ = (void**)((_zvec*)&g_actionTable)->IndexToPtr(id_);                         \
        *aslot_ = (void*)(handler);                                                                \
    } while (0)

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
    ((CAniAdvanceCursor*)((char*)m_38 + 0x1a0))->Advance_15c360(g_defaultGeo);
    CWarlordAnimSub* sub = (CWarlordAnimSub*)((char*)m_38 + 0x1a0);
    if (sub->m_28 != 0 && sub->m_20 == 0) {
        ResolveMovingAnimation();
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
    if (((CAniAdvanceCursor*)((char*)m_38 + 0x1a0))->Advance_15c360(g_defaultGeo) != 1) {
        return 0;
    }

    CGameRegistry* reg = g_gameReg;
    if (reg->m_134 != 1) {
        CGameObject* o = m_object;
        i32 dist = ((CRegThreatHelper*)reg->m_cmdGrid)
                       ->NearestEnemyDist(o->m_124, o->m_screenX, o->m_screenY);
        if (dist < g_buteMgr.GetIntDef("Warlordz", "PanicRadius", 0x40)) {
            NotifyFortUnderAttack();
            return 0;
        }
    }

    if ((i64)(u32)g_645588 - *(i64*)&m_cooldownStampLo >= *(i64*)&m_cooldownWindowLo) {
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
// in ecx, mirror-swapping g_645588 into the other scratch reg. A pure scratch
// ecx<->edx coin-flip - no source lever flips it (tried inline vs named helper,
// m_2c-chain split; all no-change at the same ~91% plateau).
RVA(0x00044d10, 0x106)
i32 CWarlord::LoadAttributes2() {
    if (((CAniAdvanceCursor*)((char*)m_38 + 0x1a0))->Advance_15c360(g_defaultGeo) != 1) {
        return 0;
    }

    CGameRegistry* reg = g_gameReg;
    if (reg->m_134 != 1) {
        CGameObject* o = m_object;
        i32 dist = ((CRegThreatHelper*)reg->m_cmdGrid)
                       ->NearestEnemyDist(o->m_124, o->m_screenX, o->m_screenY);
        if (dist >= g_buteMgr.GetIntDef("Warlordz", "PanicRadius", 0x40)) {
            RaiseBattleAlert();
            return 0;
        }
    } else {
        if (((CWarlordMission*)reg->m_curState)->m_3f4->m_4c == 0) {
            ResolveMovingAnimation();
            return 0;
        }
        if ((i64)(u32)g_645588 - *(i64*)&m_cooldownStampLo >= *(i64*)&m_cooldownWindowLo) {
            ((CRegBattleEvent*)reg->m_cueSink)->PostBattleEvent(m_object->m_188, 0x436, -1, -1, -1);
            m_cooldownWindowLo = 0x7530;
            m_cooldownWindowHi = 0;
            m_cooldownStampLo = g_645588;
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
// (clear m_2a0, window m_298=0x3e8, start-stamp m_290=g_645588, zero the hi/window-
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
extern i32 g_curPlayer; // 0x644c54 (current local player index)
RVA(0x00044e70, 0x87)
i32 CWarlord::AdvanceMovingAnim() {
    ((CAniAdvanceCursor*)((char*)m_38 + 0x1a0))->Advance_15c360(g_defaultGeo);
    CWarlordAnimSub* sub = (CWarlordAnimSub*)((char*)m_38 + 0x1a0);
    if (sub->m_28 == 0 || sub->m_20 != 0) {
        return 0;
    }
    CRegThreatHelper* h = (CRegThreatHelper*)g_gameReg->m_cmdGrid;
    if (h->m_288 != 0 && m_object->m_124 == g_curPlayer) {
        h->m_2a0 = 0;
        CRegThreatHelper* h2 = (CRegThreatHelper*)g_gameReg->m_cmdGrid;
        h2->m_window = 0x3e8;
        h2->m_stamp = (u32)g_645588;
    }
    ResolveMovingAnimation();
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
    ((CAniAdvanceCursor*)((char*)m_38 + 0x1a0))->Advance_15c360(g_defaultGeo);
    CWarlordAnimSub* sub = (CWarlordAnimSub*)((char*)m_38 + 0x1a0);
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
//   ((CAniAdvanceCursor*)sub)->Advance_15c360(g_defaultGeo);                      // m_38+0x1a0, 0x15c360
//   if (sub->m_28 == 0 || sub->m_20 != 0) return;         // ready-to-move gate
//   CGameObject* o = m_10; i32 x=o->m_5c, y=o->m_60;
//   if (x in [reg->m_13c, reg->m_144) && y in [reg->m_140, reg->m_148)) {
//     spr = reg->m_30->m_08->CreateSprite(0, x-30, y+10, 0xcf84f, "..."@0x60a96c, 0x40003);
//     if (spr) { spr->CacheFirstFrame("..."@0x60d30c);    // 0x150540
//               spr->ApplyLookupGeometry("..."@0x60d30c, 0); } // 0x1505b0
//   }
//   ... panic sub reg->m_68 (m_288/m_2a0/m_290 timer arm to 0x3e8/g_645588) ...
//   ... reg->m_150[o->m_124 * 0x48].m_0c = 0 (owner-array slot) ...
// @early-stop
// DEFERRED to the final sweep WITH the sprite models: the effect spawn is
// CSpriteFactory::CreateSprite (0x1597b0) + CGruntSprite::CacheFirstFrame (0x150540)
// + CGruntAnimPlayer::ApplyLookupGeometry (0x1505b0) - all in the sprite-worker-owned
// spriteresource domain (excluded from this brief). Best reconstructed alongside
// those class models; homed so the RVA is labeled.
RVA(0x00044f80, 0x127)
void CWarlord::BuildFortSplashParticles() {}

// ===========================================================================
// CWarlord::NotifyFortUnderAttack  (0x045270)
// ===========================================================================
// Raise the fort-under-attack alert when an enemy breaches the panic radius.
// @early-stop
// DEFERRED to the final sweep (big function, 0x2a8 > 512B, STOP-EARLY). Homed so
// the RVA + its LoadAttributes caller pair; full body left for a leaf-first redo.
RVA(0x00045270, 0x2a8)
void CWarlord::NotifyFortUnderAttack() {}

// class-metadata SIZE sweep (misc-Gruntz A-C): matching-neutral, hosted at
// .cpp EOF (see docs/class-metadata-sweep-log.md). SIZE_UNKNOWN = size not yet pinned.
SIZE_UNKNOWN(CRegBattleEvent);
SIZE_UNKNOWN(CRegThreatHelper);
SIZE_UNKNOWN(CWarlordAnimSub);
SIZE_UNKNOWN(CWarlordMission);
SIZE_UNKNOWN(CWarlordObjective);
