#include <rva.h>

#include <Gruntz/Ufo.h>

#include <DDrawMgr/DDrawChildGroup.h>
#include <Gruntz/GruntzMgr.h>
#include <Gruntz/LightFxMgr.h>
#include <Gruntz/LogicTypeId.h>
#include <Gruntz/SerialArchive.h>
#include <Gruntz/SpotLight.h>
#include <Io/FileMem.h>

#include <stddef.h>

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
        if (sl != NULL) {
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
            s->Read(&p->m_deadline, sizeof(p->m_deadline));
            s->Read(&p->m_window, sizeof(p->m_window));
        }
    } else {
        s->Write(&p->m_deadline, sizeof(p->m_deadline));
        s->Write(&p->m_window, sizeof(p->m_window));
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
            s->Read(&m_speed, sizeof(m_speed));
            s->Read(&m_posX, sizeof(m_posX));
            s->Read(&m_posY, sizeof(m_posY));
            s->Read(&m_unitX, sizeof(m_unitX));
            s->Read(&m_unitY, sizeof(m_unitY));
            s->Read(&m_roundBiasX, sizeof(m_roundBiasX));
            s->Read(&m_roundBiasY, sizeof(m_roundBiasY));
            CPathWaypoint* p = m_wp;
            i32 n = 13;
            do {
                s->Read(p, sizeof(*p));
                p += 1;
            } while (--n != 0);
            s->Read(&m_wpIndex, sizeof(m_wpIndex));
            s->Read(&m_wpX, sizeof(m_wpX));
            s->Read(&m_wpY, sizeof(m_wpY));
            s->Read(&m_wpCount, sizeof(m_wpCount));
            s->Read(&m_strikeArmed, sizeof(m_strikeArmed));
        }
    } else {
        s->Write(&m_speed, sizeof(m_speed));
        s->Write(&m_posX, sizeof(m_posX));
        s->Write(&m_posY, sizeof(m_posY));
        s->Write(&m_unitX, sizeof(m_unitX));
        s->Write(&m_unitY, sizeof(m_unitY));
        s->Write(&m_roundBiasX, sizeof(m_roundBiasX));
        s->Write(&m_roundBiasY, sizeof(m_roundBiasY));
        CPathWaypoint* p = m_wp;
        i32 n = 13;
        do {
            s->Write(p, sizeof(*p));
            p += 1;
        } while (--n != 0);
        s->Write(&m_wpIndex, sizeof(m_wpIndex));
        s->Write(&m_wpX, sizeof(m_wpX));
        s->Write(&m_wpY, sizeof(m_wpY));
        s->Write(&m_wpCount, sizeof(m_wpCount));
        s->Write(&m_strikeArmed, sizeof(m_strikeArmed));
    }
    return 1;
}
