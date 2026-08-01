#include <Rez/FrameClock.h>
#include <Gruntz/InGameText.h>
#include <Gruntz/GameRegMfcPtr.h>
#include <Gruntz/GruntzMgr.h>
#include <Gruntz/SoundState.h>
#include <Wap32/ZVec.h>
#include <Gruntz/Grunt.h>
#include <Gruntz/TriggerMgr.h>
#include <Gruntz/GameRegistry.h>
#include <Gruntz/TypeKeyColl.h>
#include <rva.h>
#include <string.h>

#include <Gruntz/AniAdvanceCursor.h>
#include <Gruntz/LeafCue.h>
DATA(0x0020d7f8)
char s_codeK[] = "K";
DATA_SYMBOL(0x002bf3bc, 0x4, _g_engineFrameDelta)
DATA_SYMBOL(0x002bf3c0, 0x4, _g_killCueClock)

// @early-stop
RVA(0x000997c0, 0x1e7)
i32 CInGameText::Update() {
    m_38->m_1a0.Advance(static_cast<i32>(g_engineFrameDelta));

    i32 areaId;
    i32 subId;
    CGrunt* found = g_gameReg->m_cmdGrid
                        ->HitTestCell(m_object->m_screenX, m_object->m_screenY, &areaId, &subId, 1);

    if (found != 0) {
        if (areaId != g_curPlayer) {
            return 0;
        }
        if (m_cachedSubId != -1 && areaId == m_cachedAreaId && subId == m_cachedSubId) {
            return 0;
        }

        CString* node = g_typeColl.ScratchResolve(found->m_objAux->ActKey());

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
        i32 y = o->m_screenY;
        i32 x = o->m_screenX;
        CGruntzMgr* reg = g_gameReg;
        if (x < reg->m_viewBounds.right && x >= reg->m_viewBounds.left
            && y < reg->m_viewBounds.bottom && y >= reg->m_viewBounds.top) {
            CDDrawSubMgrLeafScan* set = reg->m_world->m_soundRegistry;
            if (set->m_emitGate == 0) {
                void* res_ob = 0;
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
