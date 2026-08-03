#include <rva.h>

#include <Gruntz/Ufo.h>

#include <DDrawMgr/DDrawChildGroup.h>
#include <Gruntz/GruntzMgr.h>
#include <Gruntz/LightFxMgr.h>
#include <Gruntz/LogicTypeId.h>
#include <Gruntz/SerialArchive.h>
#include <Gruntz/SpotLight.h>
#include <Io/FileMem.h>

VTBL(CUFO, 0x001e72b4);
RVA_COMPGEN(0x000133d0, 0x1e, ??_GCUFO@@UAEPAXI@Z)
RVA(0x000b4330, 0x8)
i32 CUFO::Tick() {
    CPathHazard::Tick();
    return 0;
}

// @early-stop
RVA(0x000b4a90, 0x145)
CUFO::CUFO(CGameObject* obj) : CPathHazard(obj) {
    CWwdGameObjectA* o = m_object;
    i32 sx = o->m_screenX;
    i32 sy = o->m_screenY;
    m_value = m_wwdObject->m_animCursor.m_animation;
    m_wwdObject->ApplyLookupGeometry("LEVEL_UFO", 0);
    for (i32 i = 0; i < 2; ++i) {
        CWwdGameObjectA* sl =
            g_gameReg->m_world->m_childGroup->CreateSprite(0, sx, 0, 0, "SpotLight", 0x40003);
        if (sl != 0) {
            sl->ApplyName("LEVEL_SPOTLIGHT");
            AnimWorkerObj* sub = sl->m_animWorker;
            sl->m_score = 1;
            sl->m_direction = 0;
            sl->m_smarts = 2;
            sl->m_powerup = 0;
            sl->m_points = i;
            sl->m_damage = m_object->m_faceDirection;
            sub->m_notify(sl);

            (static_cast<CSpotLight*>(sl->m_animWorker->m_logic))->m_focus = m_object;
        }
    }
    m_object->m_drawActive = 1;
    m_object->m_drawFillCmd = SHADE_ALPHA_16;
    m_object->m_fillFraction = 0x80;
    m_object->m_area.left = 0;
    m_object->m_area.right = 0;
    m_object->m_area.top = 0;
    m_object->m_area.bottom = 0;
}

RVA(0x000b4c40, 0x4b)
i32 CUFO::SerializeMove(CFileMemBase* ar, SerialMode mode, LogicTypeId c, CGameObject* d) {
    if (!CPathHazard::SerializeMove(ar, mode, c, d)) {
        return 0;
    }
    if (mode == SERIAL_POSTLOAD) {
        CWwdGameObjectA* o = m_object;
        o->m_drawActive = 1;
        // Two domains, one slot, and the SHAPE is byte-evidenced: retail stores
        // the register holding `mode` (`mov [eax+0x50],edi`), not an immediate.
        // SERIAL_POSTLOAD and SHADE_ALPHA_16 are both 8, and CUFO's ctor sets
        // that same SHADE_ALPHA_16 / 0x80 pair on this object.
        o->m_drawFillCmd = static_cast<ShadeMode>(mode);
        o->m_fillFraction = 0x80;
    }
    return 1;
}

static inline void SerQuadPair(CFileMemBase* s, i32 tag, CHazardTimer* p) {
    if (tag != 4) {
        if (tag == 7) {
            s->Read(&p->m_deadline, 8);
            s->Read(&p->m_window, 8);
        }
    } else {
        s->Write(&p->m_deadline, 8);
        s->Write(&p->m_window, 8);
    }
}

RVA(0x000b4d30, 0x287)
i32 CPathHazard::SerializeMove(
    CFileMemBase* stream,
    SerialMode tag,
    LogicTypeId c,
    CGameObject* d
) {
    CFileMemBase* s = stream;
    if (CUserLogic::SerializeMove(stream, tag, c, d) == 0) {
        return 0;
    }
    if (Chain(static_cast<CFileMemBase*>(stream), tag, c, d) == 0) {
        return 0;
    }
    SerQuadPair(s, tag, &m_leg);
    SerQuadPair(s, tag, &m_strike);
    if (tag != SERIAL_SAVE) {
        if (tag == SERIAL_LOAD) {
            s->Read(&m_speed, 8);
            s->Read(&m_posX, 8);
            s->Read(&m_posY, 8);
            s->Read(&m_unitX, 8);
            s->Read(&m_unitY, 8);
            s->Read(&m_roundBiasX, 8);
            s->Read(&m_roundBiasY, 8);
            CPathWaypoint* p = m_wp;
            i32 n = 13;
            do {
                s->Read(p, 8);
                p += 1;
            } while (--n != 0);
            s->Read(&m_wpIndex, 4);
            s->Read(&m_wpX, 4);
            s->Read(&m_wpY, 4);
            s->Read(&m_wpCount, 4);
            s->Read(&m_strikeArmed, 4);
        }
    } else {
        s->Write(&m_speed, 8);
        s->Write(&m_posX, 8);
        s->Write(&m_posY, 8);
        s->Write(&m_unitX, 8);
        s->Write(&m_unitY, 8);
        s->Write(&m_roundBiasX, 8);
        s->Write(&m_roundBiasY, 8);
        CPathWaypoint* p = m_wp;
        i32 n = 13;
        do {
            s->Write(p, 8);
            p += 1;
        } while (--n != 0);
        s->Write(&m_wpIndex, 4);
        s->Write(&m_wpX, 4);
        s->Write(&m_wpY, 4);
        s->Write(&m_wpCount, 4);
        s->Write(&m_strikeArmed, 4);
    }
    return 1;
}
