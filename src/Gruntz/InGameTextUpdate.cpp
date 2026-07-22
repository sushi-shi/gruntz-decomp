#include <Rez/FrameClock.h> // g_engineFrameDelta/g_killCueClock (the clock band)
#include <Gruntz/InGameText.h> // the canonical CInGameText : CUserLogic model
#include <Gruntz/GameRegMfcPtr.h> // g_gameReg at its REAL type (CGruntzMgr)
#include <Gruntz/GruntzMgr.h>
#include <Gruntz/SoundState.h> // g_sndEnabled/g_sndCueTag
#include <Wap32/ZVec.h>
#include <Gruntz/Grunt.h>
#include <Gruntz/TriggerMgr.h>
#include <Gruntz/GameRegistry.h> // canonical *0x64556c singleton (CGameRegistry; m_68/m_world/view bounds)
#include <Gruntz/TypeKeyColl.h> // the shared zDArray (g_typeColl @0x6bf650)
#include <rva.h>
#include <string.h> // strcmp (inlined /O2)

#include <Gruntz/AniAdvanceCursor.h> // canonical CAniAdvanceCursor (Advance)
#include <Gruntz/LeafCue.h>          // LeafCue (the looked-up sound cue: m_10/m_14/m_18)
DATA(0x0020d7f8)
char s_codeK[] = "K"; // "K" (0x60d7f8) - the anim type-code literal
DATA_SYMBOL(0x002bf3bc, 0x4, _g_engineFrameDelta)
DATA_SYMBOL(0x002bf3c0, 0x4, _g_killCueClock)

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
    m_38->m_1a0.Advance(static_cast<i32>(g_engineFrameDelta));

    i32 areaId;
    i32 subId;
    CGrunt* found = reinterpret_cast<CGrunt*>(g_gameReg->m_cmdGrid
                        ->HitTestCell(m_object->m_screenX, m_object->m_screenY, &areaId, &subId, 1));
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

    char** node = reinterpret_cast<char**>(g_typeColl._zvec::IndexToPtr(reinterpret_cast<i32>(found->m_objAux->m_1c)));
    CString* p =
        reinterpret_cast<CString*>(g_typeColl.m_alloc); // m_alloc is the i32-typed slot base (the _zvec spelling)
    i32 n = g_typeColl.m_grown;
    while (n-- != 0) {
        if (p != 0) {
            p->CString::CString();
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

    CWwdGameObjectA* o = m_object;
    i32 x = o->m_screenX;
    i32 y = o->m_screenY;
    if (x < g_gameReg->m_viewOriginR && x >= g_gameReg->m_viewOriginL
        && y < g_gameReg->m_viewOriginB && y >= g_gameReg->m_viewOriginT) {
        CSndHost* set = g_gameReg->m_world->m_soundRegistry;
        if (set->m_emitGate == 0) {
            void* res_ob = 0; // CMapStringToPtr::Lookup (0x1b8438) takes a void&
            set->m_10.Lookup("GAME_HELPBOOK", res_ob);
            LeafCue* res = static_cast<LeafCue*>(res_ob);
            if (res != 0) {
                i32 enable = g_sndEnabled;
                i32 token = g_sndCueTag;
                if (enable != 0) {
                    u32 now = g_killCueClock;
                    if (static_cast<u32>((now - res->m_14)) >= static_cast<u32>(res->m_18)) {
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
