#include <Gruntz/Ufo.h> // CUFO : CPathHazard (canonical; pulls PathHazard.h -> GameRegistry.h)
#include <Io/FileMem.h> // the serialize stream (CFileMemBase == the real CFileMemBase)
#include <DDrawMgr/DDrawChildGroup.h> // the ONE CDDrawChildGroup (CreateSprite @0x1597b0)
#include <Gruntz/SpotLight.h>         // CSpotLight - the spawned spotlight's bound logic leaf
#include <Gruntz/SerialArchive.h>     // the shared CFileMemBase stream (Read @+0x2c / Write @+0x30)
#include <Gruntz/SerialArchive.h> // CFileMemBase (the inherited CWapX::Chain arg; ex SerialObjRef.h)
#include <Gruntz/LightFxMgr.h>    // CLightFxMgr (g_gameReg->m_logicPump->m_tables[]) - Method_b4cb0
#include <Gruntz/GruntzMgr.h>     // complete CGruntzMgr
#include <rva.h>

VTBL(CUFO, 0x001e72b4); // vtable_names -> code (RTTI game class)
RVA_COMPGEN(0x000133d0, 0x1e, ??_GCUFO@@UAEPAXI@Z)
RVA(0x000b4330, 0x8)
i32 CUFO::Tick() {
    CPathHazard::Tick();
    return 0;
}

// @confidence: high
// @source: rtti-vptr
// @early-stop
// dead-spill frame-size wall (~81%): logic/offsets/CFG/the spotlight loop/CreateSprite
// arg marshaling all match retail (registry path is the canonical m_world->m_childGroup, byte-
// verified against 0x000b4a90). The single non-steerable /O2 residue: retail loads
// sy = o->m_screenY and spills it to [esp+0x30] but NEVER reloads it - a DEAD load+spill
// MSVC5 keeps in retail yet our recompile (same compiler) DCEs, so retail's frame
// reserves 8 bytes (sub esp,8) vs our 4, shifting every [esp+N] (incl. the EH trylevel
// slot) by 4 and cascading; making sy "used" would change semantics. Plus the
// eh-ctor-vptr-restamp-position late-stamp (docs/patterns/eh-ctor-vptr-restamp-position.md).
// Deferred to the final sweep.
RVA(0x000b4a90, 0x145)
CUFO::CUFO(CGameObject* obj) : CPathHazard(obj) {
    CWwdGameObjectA* o = m_object;
    i32 sx = o->m_screenX;
    i32 sy = o->m_screenY;
    m_value = m_38->m_1a0.m_14;
    m_38->ApplyLookupGeometry("LEVEL_UFO", 0);
    for (i32 i = 0; i < 2; ++i) {
        CWwdGameObjectA* sl =
            g_gameReg->m_world->m_childGroup->CreateSprite(0, sx, 0, 0, "SpotLight", 0x40003);
        if (sl != 0) {
            sl->ApplyName("LEVEL_SPOTLIGHT");
            AnimWorkerObj* sub = sl->m_7c;
            sl->m_114 = 1;
            sl->m_12c = 0;
            sl->m_124 = 2;
            sl->m_11c = 0;
            sl->m_118 = i;
            sl->m_120 = m_object->m_130;
            sub->m_notify(sl);
            // The spotlight's bound logic leaf (CSpotLight): stash the UFO's owner
            // game-object into its reused +0x98 focus slot (both CGameObject*).
            (static_cast<CSpotLight*>(sl->m_7c->m_logic))->m_focus = m_object;
        }
    }
    m_object->m_drawActive = 1;
    m_object->m_drawFillCmd = 0x8;
    m_object->m_fillFraction = 0x80;
    m_object->m_area.left = 0;
    m_object->m_area.right = 0;
    m_object->m_area.top = 0;
    m_object->m_area.bottom = 0;
}

RVA(0x000b4c40, 0x4b)
i32 CUFO::SerializeMove(CFileMemBase* ar, i32 mode, i32 c, CGameObject* d) {
    if (!CPathHazard::SerializeMove(ar, mode, c, d)) {
        return 0;
    }
    if (mode == 8) {
        CWwdGameObjectA* o = m_object;
        o->m_drawActive = 1;
        o->m_drawFillCmd = mode;
        o->m_fillFraction = 0x80;
    }
    return 1;
}

// (0xb4cb0 - CRainCloud::SerializeMove - moved to its owner RainCloud.cpp.)

// the +0x108/+0x120 deadline pairs stream as two adjacent 8-byte quads
static inline void SerQuadPair(CFileMemBase* s, i32 tag, i64* p) {
    if (tag != 4) {
        if (tag == 7) {
            s->Read(p, 8);
            s->Read(p + 1, 8);
        }
    } else {
        s->Write(p, 8);
        s->Write(p + 1, 8);
    }
}

RVA(0x000b4d30, 0x287)
i32 CPathHazard::SerializeMove(CFileMemBase* stream, i32 tag, i32 c, CGameObject* d) {
    CFileMemBase* s = stream;
    if (CUserLogic::SerializeMove(stream, tag, c, d) == 0) {
        return 0;
    }
    if (Chain(static_cast<CFileMemBase*>(stream), tag, c, d) == 0) {
        return 0;
    }
    SerQuadPair(s, tag, &m_legDeadline);
    SerQuadPair(s, tag, &m_strikeDeadline);
    if (tag != 4) {
        if (tag == 7) {
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
