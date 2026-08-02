#include <rva.h>

#include <Gruntz/RainCloud.h>

#include <Bute/ButeMgr.h>
#include <Gruntz/GruntzMgr.h>
#include <Gruntz/LightFxMgr.h>
#include <Gruntz/SoundState.h>
#include <Gruntz/TriggerMgr.h>
#include <Rez/FrameClock.h>

VTBL(CRainCloud, 0x001e7324);

RVA_COMPGEN(0x00013310, 0x1e, ??_GCRainCloud@@UAEPAXI@Z)
RVA_COMPGEN(0x00013340, 0x44, ??1CRainCloud@@UAE@XZ)

RVA(0x000b4640, 0x104)
i32 CRainCloud::HitTest(i32 a, i32 b) {
    m_strikeArmed = 1;
    m_strike.m_window = static_cast<i64>(
        static_cast<u32>(g_buteMgr.GetDwordDef("Hazardz", "RainCloudFlashTime", 0x7d0))
    );
    m_strike.m_deadline = static_cast<i64>(static_cast<u32>(g_frameTime));
    g_gameReg->m_cmdGrid->CellDispatch(a, b, 9, -1);

    CWwdGameObjectA* obj = m_object;
    CGruntzMgr* reg = g_gameReg;
    i32 y = obj->m_screenY;
    i32 x = obj->m_screenX;
    if (x < reg->m_viewBounds.right && x >= reg->m_viewBounds.left && y < reg->m_viewBounds.bottom
        && y >= reg->m_viewBounds.top) {
        CDDrawSubMgrLeafScan* host = reg->m_world->m_soundRegistry;
        if (host->m_emitGate == 0) {
            void* out_ob = 0;
            host->m_cues.Lookup("LEVEL_CLOUDHAZARDKILL", out_ob);
            LeafCue* out = static_cast<LeafCue*>(out_ob);
            if (out != 0) {
                i32 enabled = g_sndEnabled;
                i32 tag = g_sndCueTag;
                if (enabled != 0) {
                    u32 now = g_killCueClock;
                    if (static_cast<u32>((now - out->m_lastPlayTime)) >= out->m_replayDelay) {
                        out->m_lastPlayTime = now;
                        out->m_sound->ConfigureItem(tag, 0, 0, 0);
                    }
                }
            }
        }
    }
    return 1;
}

RVA(0x000b49b0, 0xa8)
CRainCloud::CRainCloud(CGameObject* obj) : CPathHazard(obj) {
    CWwdGameObjectA* o = m_object;
    CShadeTable* n = g_gameReg->m_logicPump->m_tables[5];
    o->m_drawActive = 1;
    o->m_drawFillCmd = 0x7;
    o->m_drawFillArg = n;
    m_value = m_wwdObject->m_animCursor.m_animation;
    m_wwdObject->ApplyLookupGeometry("LEVEL_RAINCLOUD", 0);
    m_object->m_area.left = 1;
    m_object->m_area.right = 1;
    m_object->m_area.top = 1;
    m_object->m_area.bottom = 1;
}

RVA(0x000b4cb0, 0x56)
i32 CRainCloud::SerializeMove(CFileMemBase* stream, i32 tag, i32 c, CGameObject* d) {
    if (!CPathHazard::SerializeMove(stream, tag, c, d)) {
        return 0;
    }
    if (tag == 8) {
        CShadeTable* x = g_gameReg->m_logicPump->m_tables[5];
        CWwdGameObjectA* o = m_object;
        o->m_drawActive = 1;
        o->m_drawFillCmd = 7;
        o->m_drawFillArg = x;
    }
    return 1;
}
