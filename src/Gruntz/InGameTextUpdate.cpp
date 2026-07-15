// InGameTextUpdate.cpp - CInGameText::Update (0x997c0), the in-game text/help
// object's per-frame tick (C:\Proj\Gruntz), re-homed from src/Stub/Backlog.cpp.
//
// Ghidra symbol cluster proves CInGameText ownership: 0x997c0 sits inside the
// CInGameText method run (ctor 0x99110, InitActReg 0x993e0, Dispatch 0x99460,
// RegisterTextLogic 0x995c0, Update 0x997c0, Serialize 0x99a30). The class is the
// canonical <Gruntz/InGameText.h> model (folded here - no per-TU view): the two
// receivers the update path drives (m_object @ +0x10, m_38 @ +0x38) are the shared
// CGameObject the ctor binds (both == obj), reached through their real CGameObject
// fields (m_screenX/m_screenY/m_124/m_stateFlags + the +0x1a0 per-leaf anim sub).
// Only offsets / code bytes are load-bearing; the engine sub-object helpers below
// (hit-test result chain, sound chain, type-key cache) are reloc-masked externals.
#include <Gruntz/InGameText.h> // the canonical CInGameText : CUserLogic model
#include <Wap32/ZVec.h>
#include <Gruntz/Grunt.h>
#include <Gruntz/TriggerMgr.h>
#include <Gruntz/GameRegistry.h> // canonical *0x64556c singleton (CGameRegistry; m_68/m_world/view bounds)
#include <Gruntz/TypeKeyColl.h> // the shared CTypeKeyColl (g_typeColl @0x6bf650)
#include <rva.h>
#include <string.h> // strcmp (inlined /O2)

// The per-leaf anim sub-object embedded at CGameObject+0x1a0: its Advance @0x15c360 is
// CAniAdvanceCursor::Advance (header-less; the canonical class is a CLoadable,
// see <Gruntz/AniAdvanceCursor.h>). Local minimal decl - only the non-virtual advance
// call is reached here (via a cast), no fields/vtable.
#include <Gruntz/AniAdvanceCursor.h> // canonical CAniAdvanceCursor (Advance)
#include <Gruntz/LeafCue.h>          // LeafCue (the looked-up sound cue: m_10/m_14/m_18)
// The shared sound chain is fully canonical (same as Projectile/VideoConfig/BootyState):
// g_gameReg->m_world->m_28 is the CSndHost (SoundCue.h, pulled by GameRegistry.h) whose
// +0x10 CMapStringToPtr cue registry maps "GAME_HELPBOOK" to a LeafCue, whose +0x10
// CSoundCueMgr plays it (ConfigureItem @0x1360d0). No per-TU view - the real types.
// The *0x64556c singleton is the canonical CGameRegistry: m_cmdGrid the CTriggerMgr cell
// hit-tester, m_world the resource holder (its +0x28 CSndHost is the cue host),
// m_viewOriginL/T/R/B (+0x13c..+0x148) the on-screen view bounds.
// g_gameReg: plain extern; its one canonical DATA(0x0024556c) definition lives in GruntzMgr.cpp
extern "C" CGameRegistry* g_gameReg;
// The 4-byte default-constructed CString cache nodes (FUN_001b9b93 == CString
// default ctor; matched array-touch loop). g_typeColl.m_alloc is the base pointer.
SIZE_UNKNOWN(EngStr4);
struct EngStr4 {
    char* m_pszData; // +0x00 (4 bytes so the loop's `p++` advances by 4)
    void Ctor();     // FUN_001b9b93 __thiscall (CString default ctor)
};
// CTypeKeyColl (IndexToPtr == thunk 0x403864 -> node) is the shared
// <Gruntz/TypeKeyColl.h> shape.
DATA(0x002bf650)
extern CTypeKeyColl g_typeColl;
DATA(0x002bf3bc)
extern "C" i32 g_engineFrameDelta; // sub-logic clock fed to CAniAdvanceCursor::Advance
DATA(0x002bf3c0)
extern "C" u32 g_killCueClock; // wrap-safe draw clock
extern i32 g_sndEnabled; // cue enable gate
extern i32 g_sndCueTag;     // ?g_sndCueTag@@3HA (HELPBOOK sound token)
DATA(0x0020d7f8)
char s_codeK[] = "K"; // "K" (0x60d7f8) - the anim type-code literal

// @early-stop
// regalloc/scheduling wall (~76%): complete + correct, verified instruction-by-
// instruction vs retail (the whole front half is byte-exact modulo reloc names).
// Residual: retail loads the __thiscall receivers into ecx eagerly and schedules the
// member loads earlier, while this /O2 recompile defers the receiver to ecx after the
// arg pushes - forcing a spill/reload at the Play call and an m_10-reg swap at
// LoadPickupSprites - cascading regalloc through both epilogues. Not source-steerable.
// See docs/patterns/pin-local-for-callee-saved-reg.md.
RVA(0x000997c0, 0x1e7)
i32 CInGameText::Update() {
    ((CAniAdvanceCursor*)((char*)m_38 + 0x1a0))->Advance((i32)g_engineFrameDelta);

    i32 areaId;
    i32 subId;
    CGrunt* found = (CGrunt*)g_gameReg->m_cmdGrid
                        ->HitTestCell(m_object->m_screenX, m_object->m_screenY, &areaId, &subId, 1);
    if (found == 0) {
        m_cachedSubId = -1;
        m_38->m_stateFlags &= ~1;
        return 0;
    }
    if (areaId != g_curPlayer) {
        return 0;
    }
    if (m_cachedSubId != -1 && areaId == m_cachedAreaId && subId == m_cachedSubId) {
        return 0;
    }

    char** node = (char**)((_zvec*)&g_typeColl)->IndexToPtr((i32)found->m_14->m_1c);
    EngStr4* p =
        (EngStr4*)g_typeColl.m_alloc; // m_alloc is the i32-typed slot base (the _zvec spelling)
    i32 n = g_typeColl.m_grown;
    while (n-- != 0) {
        if (p != 0) {
            p->Ctor();
        }
        p++;
    }
    bool eq = (strcmp(*node, s_codeK) == 0);
    if (eq) {
        return 0;
    }

    if (!found->LoadPickupSprites(0x5e, 0, m_object->m_124, 0, 1)) {
        return 0;
    }

    CGameObject* o = m_object;
    i32 x = o->m_screenX;
    i32 y = o->m_screenY;
    if (x < g_gameReg->m_viewOriginR && x >= g_gameReg->m_viewOriginL
        && y < g_gameReg->m_viewOriginB && y >= g_gameReg->m_viewOriginT) {
        CSndHost* set = g_gameReg->m_world->m_28;
        if (set->m_30 == 0) {
            void* res_ob = 0; // CMapStringToPtr::Lookup (0x1b8438) takes a void&
            set->m_10.Lookup("GAME_HELPBOOK", res_ob);
            LeafCue* res = (LeafCue*)res_ob;
            if (res != 0) {
                i32 enable = g_sndEnabled;
                i32 token = g_sndCueTag;
                if (enable != 0) {
                    u32 now = g_killCueClock;
                    if ((u32)(now - res->m_14) >= (u32)res->m_18) {
                        res->m_14 = now;
                        res->m_10->ConfigureItem(token, 0, 0, 0);
                    }
                }
            }
        }
    }

    m_cachedAreaId = areaId;
    m_cachedSubId = subId;
    m_38->m_stateFlags |= 1;
    return 0;
}
