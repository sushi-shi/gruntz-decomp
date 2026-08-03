#ifndef GRUNTZ_PROJECTILE_H
#define GRUNTZ_PROJECTILE_H

#include <rva.h>

#include <Mfc.h>

#include <Gruntz/ActReg.h>
#include <Gruntz/HaznColl.h>
#include <Gruntz/LogicTypeId.h>
#include <Gruntz/MovingLogic.h>
#include <Gruntz/PickupType.h>
#include <Gruntz/SerialArchive.h>

class CLightFx;

class DirectSoundMgr;

class CProjectile : public CMovingLogic, public CWapX {
public:
    RVA(0x000e0d40, 0x6c2)
    virtual i32
    SerializeMove(CFileMemBase* s, SerialMode mode, LogicTypeId typeId, CGameObject* pObj)
        OVERRIDE {
        CDDrawSurfaceMgr* reg = g_gameReg->m_world;
        if (reg == 0) {
            return 0;
        }

        char buf[0x80];

        switch (mode) {
            case SERIAL_LOAD: {
                m_sound = 0;
                s->Read(&m_kind, 4);
                s->Read(&m_srcRow, 4);
                s->Read(&m_srcCol, 4);
                s->Read(&m_targetX, 4);
                s->Read(&m_targetY, 4);
                s->Read(&m_flightDist, 8);
                s->Read(&m_timePerTile, 4);
                s->Read(&m_velScale, 8);
                s->Read(&m_posX, 8);
                s->Read(&m_posY, 8);
                s->Read(&m_velX, 8);
                s->Read(&m_velY, 8);
                s->Read(&m_roundX, 8);
                s->Read(&m_roundY, 8);
                s->Read(&m_curX, 4);
                s->Read(&m_curY, 4);
                s->Read(&m_isArcing, 4);
                s->Read(&m_arrived, 4);
                s->Read(&m_targetId, 4);
                s->Read(&m_ownerId, 4);

                void* out;
                for (i32 ni = 0; ni < 7; ni++) {
                    g_serialCounter++;
                    s->Read(buf, 0x80);
                    if (strlen(buf) != 0) {
                        out = 0;
                        reg->m_animRegistry->m_animations.Lookup(buf, out);
                        m_frames[ni] = static_cast<CAniElement*>(out);
                    } else {
                        m_frames[ni] = 0;
                    }
                }

                g_serialCounter++;
                i32 key;
                s->Read(&key, 4);
                out = 0;
                CGameObject* r;
                if (MapLookupById(reg->m_childGroup->m_map48, key, out) == 0) {
                    r = 0;
                } else if (out == 0) {
                    r = 0;
                } else {

                    r = (static_cast<CGameObject*>(out)->GetClassId() == CLASSID_SERIALREF)
                            ? static_cast<CGameObject*>(out)
                            : 0;
                }
                m_shadow = static_cast<CWwdGameObjectA*>(r);
                if (m_shadow == 0 && key != 0) {
                    return 0;
                }

                i32 cnt;
                s->Read(&cnt, 4);
                for (i32 ci = 0; ci < cnt; ci++) {
                    CoordPoolNode* node = static_cast<CoordPoolNode*>(g_coordPool.m_freeHead);
                    void* payload = 0;
                    if (node->m_next != 0) {
                        g_coordPool.m_freeHead = node->m_next;
                        payload = &node->m_coord;
                    }
                    s->Read(payload, 8);
                    m_hitList.AddTail(payload);
                }
                break;
            }

            case SERIAL_SAVE: {
                s->Write(&m_kind, 4);
                s->Write(&m_srcRow, 4);
                s->Write(&m_srcCol, 4);
                s->Write(&m_targetX, 4);
                s->Write(&m_targetY, 4);
                s->Write(&m_flightDist, 8);
                s->Write(&m_timePerTile, 4);
                s->Write(&m_velScale, 8);
                s->Write(&m_posX, 8);
                s->Write(&m_posY, 8);
                s->Write(&m_velX, 8);
                s->Write(&m_velY, 8);
                s->Write(&m_roundX, 8);
                s->Write(&m_roundY, 8);
                s->Write(&m_curX, 4);
                s->Write(&m_curY, 4);
                s->Write(&m_isArcing, 4);
                s->Write(&m_arrived, 4);
                s->Write(&m_targetId, 4);
                s->Write(&m_ownerId, 4);

                CAniElement** fp = m_frames;
                for (i32 fi = 0; fi < 7; fi++) {
                    g_serialCounter++;
                    memset(buf, 0, sizeof(buf));
                    if (*fp != 0) {
                        strcpy(buf, reg->m_animRegistry->KeyOfValue(*fp));
                    }
                    s->Write(buf, 0x80);
                    fp++;
                }

                g_serialCounter++;
                i32 n = 0;
                if (m_shadow != 0) {
                    n = m_shadow->m_objectId;
                }
                s->Write(&n, 4);

                i32 v2 = m_hitList.GetCount();
                s->Write(&v2, 4);

                POSITION pos = m_hitList.GetHeadPosition();
                while (pos != 0) {
                    s->Write(m_hitList.GetNext(pos), 8);
                }
                break;
            }
        }

        if (CMovingLogic::SerializeMove(s, mode, typeId, pObj) == 0) {
            return 0;
        }
        if (s == 0) {
            return 0;
        }

        switch (mode) {
            case SERIAL_LOAD: {
                s->Read(buf, 0x80);
                s->Read(m_blob, 0x10);
                CGameObject* obj = pObj;
                m_gameObject = obj;
                m_wwdObject = static_cast<CWwdGameObjectA*>(obj);
                m_animWorker = obj->m_animWorker;
                if (strlen(buf) == 0) {
                    m_value = 0;
                    return 1;
                }
                void* out = 0;
                m_animWorker->m_ownerCtx->m_animRegistry->m_animations.Lookup(buf, out);
                m_value = static_cast<CAniElement*>(out);
                return 1;
            }
            case SERIAL_SAVE: {
                char blob[0x80];
                memset(blob, 0, sizeof(blob));
                if (m_value != 0) {
                    strcpy(blob, m_animWorker->m_ownerCtx->m_animRegistry->KeyOfValue(m_value));
                }
                s->Write(blob, 0x80);
                s->Write(m_blob, 0x10);
                return 1;
            }
        }
        return 1;
    }

    static inline CActHandler* TBombLookup(i32 coord) {
        return (CActRegPool<CTimeBomb>::s_table.ResolveEntry(coord));
    }

    static inline CString* ActNameSlots() {
        return g_typeColl.Slots();
    }

    static inline CString* ActNameLookup(i32 id) {
        g_typeColl.m_grown = 0;
        if (id >= g_typeColl.m_lo && id <= g_typeColl.m_hi) {
            return g_typeColl.Elem(id);
        }
        if ((static_cast<_zvec*>(&g_typeColl))->GrowTo(id, 0) != 0) {
            return g_typeColl.Elem(id);
        }
        char* msg = g_errOutOfMem;
        g_retAddrBreadcrumb = GetRetAddr();
        g_typeColl.m_errSink->Set(&g_typeColl, msg, 0xc);
        return g_typeColl.Scratch();
    }
    RVA(0x00012960, 0x6)
    virtual LogicTypeId GetTypeTag() OVERRIDE {
        return LOGIC_PROJECTILE;
    }
    CProjectile() {}
    CProjectile(CGameObject* owner);
    virtual ~CProjectile() OVERRIDE;

    virtual i32
    LoadProjectileSprites(PickupType kind, i32 a, i32 b, i32 sx, i32 sy, i32 t0, i32 t1);

    virtual void FireActivation(i32 id) OVERRIDE;
    static void RegisterType();

    i32 DetachRenderObj();
    void ScanTargets(i32 impact);
    i32 LaunchSound(const char* key);
    virtual void AdvanceMotion() OVERRIDE;

    PickupType m_kind;
    i32 m_srcRow, m_srcCol;
    i32 m_targetX, m_targetY;
    double m_flightDist;
    i32 m_timePerTile;
    double m_velScale;
    double m_posX;
    double m_posY;
    double m_velX;
    double m_velY;
    double m_roundX;
    double m_roundY;
    i32 m_curX, m_curY;
    i32 m_isArcing;
    i32 m_arrived;

    enum {
        PF_IMPACT = 5,
        PF_FALL = 6
    };
    CAniElement* m_frames[7];
    CWwdGameObjectA* m_shadow;
    DirectSoundMgr* m_sound;
    CPtrList m_hitList;
    i32 m_targetId, m_ownerId;
};

SIZE_UNKNOWN();

extern const double g_movingLogicMax;

extern const double g_projPhase1;
#endif // GRUNTZ_PROJECTILE_H
