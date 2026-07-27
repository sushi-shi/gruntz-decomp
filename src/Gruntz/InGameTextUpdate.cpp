#include <Rez/FrameClock.h>       // g_engineFrameDelta/g_killCueClock (the clock band)
#include <Gruntz/InGameText.h>    // the canonical CInGameText : CUserLogic model
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
// 96.6% - the old note called the whole residual a "regalloc wall"; three quarters of it
// was source shape and is fixed (the miss handler's positive gate, the y-before-x screen
// load, and taking g_gameReg into `reg` AFTER them so cl stops hoisting the singleton
// above the coord loads: 79.5 -> 93.8 -> 95.6 -> 96.6). What is LEFT is genuinely
// scheduling, in three spots, none of which moved under any tried spelling:
//   * LoadPickupSprites - retail sets the receiver (`mov ecx,edi`) between the flag
//     pushes and uses eax for the m_124 temp; cl uses ecx for the temp and reloads the
//     receiver last. Hoisting m_124 into a local changes nothing.
//   * the CMapStringToPtr::Lookup out-param - retail zeroes the slot AFTER both arg
//     pushes (`mov [esp+0x18],0`), cl zeroes it before (out-param zero-init scheduling,
//     docs/patterns/).
//   * the success tail loads subId before areaId (source order is areaId first, as in
//     retail) and the miss tail hoists the m_38 load above the `m_cachedSubId = -1`
//     store. Both are pure instruction order; the bytes of every op match.
RVA(0x000997c0, 0x1e7)
i32 CInGameText::Update() {
    m_38->m_1a0.Advance(static_cast<i32>(g_engineFrameDelta));

    i32 areaId;
    i32 subId;
    CGrunt* found =
        g_gameReg->m_cmdGrid
            ->HitTestCell(m_object->m_screenX, m_object->m_screenY, &areaId, &subId, 1);
    // POSITIVE GATE: retail parks the miss handler at the very END of the function
    // (0x9998f) where it falls into the `return 0` epilogue the three inner early
    // exits also tail-merge into. Written as an early return it lands inline right
    // after the test with its own 12-instruction epilogue, one block too many.
    // docs/patterns/positive-gate-enables-shrink-wrap.md
    if (found != 0) {
        if (areaId != g_curPlayer) {
            return 0;
        }
        if (m_cachedSubId != -1 && areaId == m_cachedAreaId && subId == m_cachedSubId) {
            return 0;
        }

        // ScratchResolve IS the base _zvec::IndexToPtr call (0x312a0) with the band's
        // CString element type put back on at that one accessor seam.
        CString* node = g_typeColl.ScratchResolve(found->m_objAux->ActKey());
        // m_alloc is the i32-typed slot base (the _zvec spelling)
        CString* p = g_typeColl.Slots();
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
        i32 y = o->m_screenY; // y BEFORE x - retail loads +0x60 then +0x5c
        i32 x = o->m_screenX;
        CGruntzMgr* reg = g_gameReg; // ... and the singleton AFTER both, not hoisted
        if (x < reg->m_viewBounds.right && x >= reg->m_viewBounds.left
            && y < reg->m_viewBounds.bottom && y >= reg->m_viewBounds.top) {
            CDDrawSubMgrLeafScan* set = reg->m_world->m_soundRegistry;
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
    m_cachedSubId = -1;
    m_38->m_stateFlags &= ~1;
    return 0;
}
