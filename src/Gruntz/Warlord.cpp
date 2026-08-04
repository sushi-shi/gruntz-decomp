#include <rva.h>

#include <Gruntz/Warlord.h>

#include <Bute/ButeTree.h>
#include <DDrawMgr/AniAdvance.h>
#include <DDrawMgr/DDrawSubMgrLeaf.h>
#include <DDrawMgr/DDrawSurfaceMgr.h>
#include <Enums.h>
#include <Gruntz/ActReg.h>
#include <Gruntz/AniAdvanceCursor.h>
#include <Gruntz/AniElement.h>
#include <Gruntz/FontConfig.h>
#include <Gruntz/GameModeId.h>
#include <Gruntz/Grunt.h>
#include <Gruntz/GruntSpawnConfig.h>
#include <Gruntz/GruntzMgr.h>
#include <Gruntz/LogicTypeId.h>
#include <Gruntz/Play.h>
#include <Gruntz/SerialArchive.h>
#include <Gruntz/SerialCounter.h>
#include <Gruntz/SortKeyLayer.h>
#include <Gruntz/SpriteRefTable.h>
#include <Gruntz/State.h>
#include <Gruntz/Timer.h>
#include <Gruntz/TriggerMgr.h>
#include <Gruntz/TypeKeyColl.h>
#include <Gruntz/WarlordOwner.h>
#include <Io/FileMem.h>
#include <Wap32/TileGeometry.h>
#include <Wap32/ZVec.h>

#include <new>
#include <stdlib.h>

DATA(0x0020d28c)
static const char s_GRUNTZ_[] = "GRUNTZ_";
DATA(0x0020d220)
static const char s__MOVING[] = "_MOVING";
DATA(0x0020d22c)
static const char s__DEATH[] = "_DEATH";
DATA(0x0020d234)
static const char s__JOY[] = "_JOY";
DATA(0x0020d36c)
static const char s__IDLE[] = "_IDLE";
DATA(0x0020d374)
static const char s__BATTLECRY[] = "_BATTLECRY";
DATA(0x0020d284)
static const char s__IDLE1[] = "_IDLE1";
DATA(0x0020d27c)
static const char s__IDLE2[] = "_IDLE2";
DATA(0x0020d274)
static const char s__IDLE3[] = "_IDLE3";
DATA(0x0020d26c)
static const char s__IDLE4[] = "_IDLE4";
DATA(0x0020d25c)
static const char s__BATTLECRY1[] = "_BATTLECRY1";
DATA(0x0020d24c)
static const char s__BATTLECRY2[] = "_BATTLECRY2";
DATA(0x0020d23c)
static const char s__BATTLECRY3[] = "_BATTLECRY3";
DATA(0x0020d218)
static const char s__PANIC[] = "_PANIC";
DATA(0x0020d2d8)
static const char s_WARLORDZ_KING[] = "WARLORDZ_KING";
DATA(0x0020d2c0)
static const char s_WARLORDZ_NAPOLEAN[] = "WARLORDZ_NAPOLEAN";
DATA(0x0020d2ac)
static const char s_WARLORDZ_PATTON[] = "WARLORDZ_PATTON";
DATA(0x0020d298)
static const char s_WARLORDZ_VIKING[] = "WARLORDZ_VIKING";
DATA(0x0020d1bc)
static const char s_keyB[] = "B";
static const char s_keyC[] = "C";
DATA(0x0020d2ec)
static const char s_keyE[] = "E";
static const char s_keyA[] = "A";
static const char s_keyF[] = "F";

template<> DATA(0x00244610)
CActReg CActRegPool<CWarlord>::s_table(ACT_ID_FIRST, ACT_ID_LAST);

#define REGISTER_NAME(key)                                                                         \
    i32 id_ = ActFindId(key);                                                                      \
    if (id_ == 0) {                                                                                \
        ActInsertId(key, g_typeCounter);                                                           \
        id_ = g_typeCounter;                                                                       \
        CString* slot_ = g_typeColl.ScratchResolve(g_typeCounter);                                 \
        CString* p_ = g_typeColl.Slots();                                                          \
        for (i32 n_ = g_typeColl.m_grown; n_--; p_++) {                                            \
            ::new (static_cast<void*>(p_)) CString;                                                \
        }                                                                                          \
        *slot_ = key;                                                                              \
        ++g_typeCounter;                                                                           \
    }

#define REGISTER_ACTION(key, handler)                                                              \
    do {                                                                                           \
        REGISTER_NAME(key)                                                                         \
        /* Language-forced member-function representation seam; the byte accessor */               \
        /* returns to CActHandler only here. */                                                    \
        *reinterpret_cast<CActHandler*>(CActRegPool<CWarlord>::s_table._zvec::IndexToPtr(id_)) =   \
            static_cast<CActHandler>(handler);                                                     \
    } while (0)

#define REGISTER_ACTION_TYPED(key, handler)                                                        \
    do {                                                                                           \
        REGISTER_NAME(key)                                                                         \
        *CActRegPool<CWarlord>::s_table.Resolve(id_) = static_cast<CActHandler>(handler);          \
    } while (0)

RVA_COMPGEN(0x000107c0, 0x1e, ??_GCWarlord@@UAEPAXI@Z)
RVA_COMPGEN(0x000107f0, 0x55, ??1CWarlord@@UAE@XZ)

typedef enum WarlordBattleTag {
    WARLORD_TAG_KING = 0x442,
    WARLORD_TAG_NAPOLEAN = 0x443,
    WARLORD_TAG_PATTON = 0x444,
    WARLORD_TAG_VIKING = 0x445,
} WarlordBattleTag;

#define WARLORD_ANIM_LOOKUP(dst, suffix)                                                           \
    {                                                                                              \
        void* h = 0;                                                                               \
        m_wwdObject->OwnerMgr()->m_animRegistry->m_animations.Lookup(                              \
            s_GRUNTZ_ + m_warlordName + (suffix),                                                  \
            h                                                                                      \
        );                                                                                         \
        /* Lookup exposes void*& at this API boundary; */                                          \
        /* reapply the element type after the call. */                                             \
        dst = static_cast<CAniElement*>(h);                                                        \
    }

// @early-stop

RVA(0x00042d40, 0x750)
CWarlord::CWarlord(CGameObject* obj) : CUserLogic(obj), CWapX(obj) {

    m_cooldownStamp = 0;
    m_cooldownWindow = 0;
    m_timer2Stamp = 0;
    m_timer2Window = 0;

    m_object->m_screenX = (m_object->m_screenX & ~TILE_MASK_PX) + TILE_HALF_PX;
    m_object->m_screenY = (m_object->m_screenY & ~TILE_MASK_PX) + TILE_HALF_PX;

    CWwdGameObjectA* o = m_object;
    if (o->m_sortKey != 0xc3500) {
        o->m_sortKey = 0xc3500;
        o->m_flags |= 0x20000;
    }
    m_wwdObject->m_flags |= 0x2000002;

    // The WWD `Smarts` slot is per-logic; for a warlord it is the owner id.
    WarlordOwner owner = static_cast<WarlordOwner>(m_object->m_smarts);
    i32 cfg = g_gameReg->m_options[owner].m_colorIndex;
    if (cfg < 0 || cfg >= 0x11) {
        cfg = 0;
    }
    CShadeTable* sel = g_gameReg->m_spriteFactory->GetSel(cfg, 0);
    if (sel == NULL) {
        sel = g_gameReg->m_spriteFactory->GetSel(1, 0);
    }
    CWwdGameObjectA* d = m_object;
    d->m_drawActive = 1;
    d->m_drawFillCmd = SHADE_PAL_16;
    d->m_drawFillArg = sel;

    switch (owner) {
        case WARLORDZ_KING:
            m_warlordName = s_WARLORDZ_KING;
            m_ownerTag = WARLORD_TAG_KING;
            break;
        case WARLORDZ_NAPOLEAN:
            m_warlordName = s_WARLORDZ_NAPOLEAN;
            m_ownerTag = WARLORD_TAG_NAPOLEAN;
            break;
        case WARLORDZ_PATTON:
            m_warlordName = s_WARLORDZ_PATTON;
            m_ownerTag = WARLORD_TAG_PATTON;
            break;
        case WARLORDZ_VIKING:
            m_warlordName = s_WARLORDZ_VIKING;
            m_ownerTag = WARLORD_TAG_VIKING;
            break;
        default:

            (g_gameReg)->ReportError(IDX(CMD_TOGGLE_SOUND), 0x3e9);
            return;
    }

    g_gameReg->m_curState->BuildAssetNamespacePrefixes(m_warlordName, 1, 0, 0);

    WARLORD_ANIM_LOOKUP(m_idleAnims[0], s__IDLE1);
    WARLORD_ANIM_LOOKUP(m_idleAnims[1], s__IDLE2);
    WARLORD_ANIM_LOOKUP(m_idleAnims[2], s__IDLE3);
    WARLORD_ANIM_LOOKUP(m_idleAnims[3], s__IDLE4);
    WARLORD_ANIM_LOOKUP(m_battlecryAnims[0], s__BATTLECRY1);
    WARLORD_ANIM_LOOKUP(m_battlecryAnims[1], s__BATTLECRY2);
    WARLORD_ANIM_LOOKUP(m_battlecryAnims[2], s__BATTLECRY3);
    WARLORD_ANIM_LOOKUP(m_animJoy, s__JOY);
    WARLORD_ANIM_LOOKUP(m_animDeath, s__DEATH);
    WARLORD_ANIM_LOOKUP(m_animMoving, s__MOVING);
    WARLORD_ANIM_LOOKUP(m_animPanic, s__PANIC);

    m_timer2Stamp = 0;
    m_timer2Window = 0;
    m_deathStarted = 0;
    ResolveMovingAnimation();
}
#undef WARLORD_ANIM_LOOKUP

// @early-stop
RVA(0x00043670, 0xc20)
i32 CWarlord::SerializeMove(CFileMemBase* ar, SerialMode mode, LogicTypeId a3, CGameObject* obj) {

    char buf[SERIAL_NAME_LEN];
    char hdr[SERIAL_NAME_LEN];

    if (CUserLogic::SerializeMove(ar, mode, a3, obj) == 0) {
        return 0;
    }
    if (ar == NULL) {

        goto fail;
    }

    switch (mode) {
        case SERIAL_LOAD: {
            ar->Read(hdr, SERIAL_NAME_LEN);
            ar->Read(m_blob, 0x10);
            m_gameObject = obj;
            m_wwdObject = static_cast<CWwdGameObjectA*>(obj);
            m_animWorker = obj->m_animWorker;
            if (strlen(hdr) == 0) {
                m_value = NULL;
            } else {
                CMapStringToPtr* map = &m_animWorker->m_ownerCtx->m_animRegistry->m_animations;
                void* v = 0;
                map->Lookup(hdr, v);
                m_value = static_cast<CAniElement*>(v);
            }
            break;
        }
        case SERIAL_SAVE: {
            memset(buf, 0, sizeof(buf));
            if (m_value != NULL) {
                strcpy(
                    buf,
                    static_cast<const char*>(
                        m_animWorker->m_ownerCtx->m_animRegistry->KeyOfValue(m_value)
                    )
                );
            }
            ar->Write(buf, SERIAL_NAME_LEN);
            ar->Write(m_blob, 0x10);
            break;
        }
    }

    switch (mode) {
        case SERIAL_SAVE: {
            CDDrawSurfaceMgr* world = m_animWorker->m_ownerCtx;
            if (world == NULL) {
                goto fail;
            }
            g_serialCounter++;
            memset(buf, 0, sizeof(buf));
            strcpy(buf, static_cast<const char*>(m_warlordName));
            ar->Write(buf, SERIAL_NAME_LEN);
            g_serialCounter++;
            memset(buf, 0, sizeof(buf));
            if (m_idleAnims[0] != NULL) {
                strcpy(
                    buf,
                    static_cast<const char*>(world->m_animRegistry->KeyOfValue(m_idleAnims[0]))
                );
            }
            ar->Write(buf, SERIAL_NAME_LEN);
            g_serialCounter++;
            memset(buf, 0, sizeof(buf));
            if (m_idleAnims[1] != NULL) {
                strcpy(
                    buf,
                    static_cast<const char*>(world->m_animRegistry->KeyOfValue(m_idleAnims[1]))
                );
            }
            ar->Write(buf, SERIAL_NAME_LEN);
            g_serialCounter++;
            memset(buf, 0, sizeof(buf));
            if (m_idleAnims[2] != NULL) {
                strcpy(
                    buf,
                    static_cast<const char*>(world->m_animRegistry->KeyOfValue(m_idleAnims[2]))
                );
            }
            ar->Write(buf, SERIAL_NAME_LEN);
            g_serialCounter++;
            memset(buf, 0, sizeof(buf));
            if (m_idleAnims[3] != NULL) {
                strcpy(
                    buf,
                    static_cast<const char*>(world->m_animRegistry->KeyOfValue(m_idleAnims[3]))
                );
            }
            ar->Write(buf, SERIAL_NAME_LEN);
            g_serialCounter++;
            memset(buf, 0, sizeof(buf));
            if (m_battlecryAnims[0] != NULL) {
                strcpy(
                    buf,
                    static_cast<const char*>(world->m_animRegistry->KeyOfValue(m_battlecryAnims[0]))
                );
            }
            ar->Write(buf, SERIAL_NAME_LEN);
            g_serialCounter++;
            memset(buf, 0, sizeof(buf));
            if (m_battlecryAnims[1] != NULL) {
                strcpy(
                    buf,
                    static_cast<const char*>(world->m_animRegistry->KeyOfValue(m_battlecryAnims[1]))
                );
            }
            ar->Write(buf, SERIAL_NAME_LEN);
            g_serialCounter++;
            memset(buf, 0, sizeof(buf));
            if (m_battlecryAnims[2] != NULL) {
                strcpy(
                    buf,
                    static_cast<const char*>(world->m_animRegistry->KeyOfValue(m_battlecryAnims[2]))
                );
            }
            ar->Write(buf, SERIAL_NAME_LEN);
            g_serialCounter++;
            memset(buf, 0, sizeof(buf));
            if (m_animJoy != NULL) {
                strcpy(buf, static_cast<const char*>(world->m_animRegistry->KeyOfValue(m_animJoy)));
            }
            ar->Write(buf, SERIAL_NAME_LEN);
            g_serialCounter++;
            memset(buf, 0, sizeof(buf));
            if (m_animDeath != NULL) {
                strcpy(
                    buf,
                    static_cast<const char*>(world->m_animRegistry->KeyOfValue(m_animDeath))
                );
            }
            ar->Write(buf, SERIAL_NAME_LEN);
            g_serialCounter++;
            memset(buf, 0, sizeof(buf));
            if (m_animMoving != NULL) {
                strcpy(
                    buf,
                    static_cast<const char*>(world->m_animRegistry->KeyOfValue(m_animMoving))
                );
            }
            ar->Write(buf, SERIAL_NAME_LEN);
            g_serialCounter++;
            memset(buf, 0, sizeof(buf));
            if (m_animPanic != NULL) {
                strcpy(
                    buf,
                    static_cast<const char*>(world->m_animRegistry->KeyOfValue(m_animPanic))
                );
            }
            ar->Write(buf, SERIAL_NAME_LEN);
            ar->Write(&m_deathStarted, sizeof(m_deathStarted));
            ar->Write(&m_ownerTag, sizeof(m_ownerTag));
            break;
        }
        case SERIAL_LOAD: {
            CDDrawSurfaceMgr* world = m_animWorker->m_ownerCtx;
            if (world == NULL) {
                return 0;
            }
            g_serialCounter++;
            ar->Read(buf, SERIAL_NAME_LEN);
            m_warlordName = buf;

            g_serialCounter++;
            ar->Read(buf, SERIAL_NAME_LEN);
            if (strlen(buf) != 0) {
                void* v = 0;
                world->m_animRegistry->m_animations.Lookup(buf, v);
                m_idleAnims[0] = static_cast<CAniElement*>(v);
            } else {
                m_idleAnims[0] = NULL;
            }
            g_serialCounter++;
            ar->Read(buf, SERIAL_NAME_LEN);
            if (strlen(buf) != 0) {
                void* v = 0;
                world->m_animRegistry->m_animations.Lookup(buf, v);
                m_idleAnims[1] = static_cast<CAniElement*>(v);
            } else {
                m_idleAnims[1] = NULL;
            }
            g_serialCounter++;
            ar->Read(buf, SERIAL_NAME_LEN);
            if (strlen(buf) != 0) {
                void* v = 0;
                world->m_animRegistry->m_animations.Lookup(buf, v);
                m_idleAnims[2] = static_cast<CAniElement*>(v);
            } else {
                m_idleAnims[2] = NULL;
            }
            g_serialCounter++;
            ar->Read(buf, SERIAL_NAME_LEN);
            if (strlen(buf) != 0) {
                void* v = 0;
                world->m_animRegistry->m_animations.Lookup(buf, v);
                m_idleAnims[3] = static_cast<CAniElement*>(v);
            } else {
                m_idleAnims[3] = NULL;
            }
            g_serialCounter++;
            ar->Read(buf, SERIAL_NAME_LEN);
            if (strlen(buf) != 0) {
                void* v = 0;
                world->m_animRegistry->m_animations.Lookup(buf, v);
                m_battlecryAnims[0] = static_cast<CAniElement*>(v);
            } else {
                m_battlecryAnims[0] = NULL;
            }
            g_serialCounter++;
            ar->Read(buf, SERIAL_NAME_LEN);
            if (strlen(buf) != 0) {
                void* v = 0;
                world->m_animRegistry->m_animations.Lookup(buf, v);
                m_battlecryAnims[1] = static_cast<CAniElement*>(v);
            } else {
                m_battlecryAnims[1] = NULL;
            }
            g_serialCounter++;
            ar->Read(buf, SERIAL_NAME_LEN);
            if (strlen(buf) != 0) {
                void* v = 0;
                world->m_animRegistry->m_animations.Lookup(buf, v);
                m_battlecryAnims[2] = static_cast<CAniElement*>(v);
            } else {
                m_battlecryAnims[2] = NULL;
            }
            g_serialCounter++;
            ar->Read(buf, SERIAL_NAME_LEN);
            if (strlen(buf) != 0) {
                void* v = 0;
                world->m_animRegistry->m_animations.Lookup(buf, v);
                m_animJoy = static_cast<CAniElement*>(v);
            } else {
                m_animJoy = NULL;
            }
            g_serialCounter++;
            ar->Read(buf, SERIAL_NAME_LEN);
            if (strlen(buf) != 0) {
                void* v = 0;
                world->m_animRegistry->m_animations.Lookup(buf, v);
                m_animDeath = static_cast<CAniElement*>(v);
            } else {
                m_animDeath = NULL;
            }
            g_serialCounter++;
            ar->Read(buf, SERIAL_NAME_LEN);
            if (strlen(buf) != 0) {
                void* v = 0;
                world->m_animRegistry->m_animations.Lookup(buf, v);
                m_animMoving = static_cast<CAniElement*>(v);
            } else {
                m_animMoving = NULL;
            }
            g_serialCounter++;
            ar->Read(buf, SERIAL_NAME_LEN);
            if (strlen(buf) != 0) {
                void* v = 0;
                world->m_animRegistry->m_animations.Lookup(buf, v);
                m_animPanic = static_cast<CAniElement*>(v);
            } else {
                m_animPanic = NULL;
            }
            ar->Read(&m_deathStarted, sizeof(m_deathStarted));
            ar->Read(&m_ownerTag, sizeof(m_ownerTag));
            break;
        }
        case SERIAL_POSTLOAD: {

            CShadeTable* sel = g_gameReg->m_spriteFactory->GetSel(
                g_gameReg->m_options[m_object->m_smarts].m_colorIndex,
                0
            );
            if (sel == NULL) {
                sel = g_gameReg->m_spriteFactory->GetSel(1, 0);
            }

            CWwdGameObjectA* sprite = m_object;
            sprite->m_drawActive = 1;
            sprite->m_drawFillCmd = SHADE_PAL_16;
            sprite->m_drawFillArg = sel;
            break;
        }
    }

    {
        switch (mode) {
            case SERIAL_LOAD:
                ar->Read(&m_cooldownStamp, sizeof(m_cooldownStamp));
                ar->Read(&m_cooldownWindow, sizeof(m_cooldownWindow));
                break;
            case SERIAL_SAVE:
                ar->Write(&m_cooldownStamp, sizeof(m_cooldownStamp));
                ar->Write(&m_cooldownWindow, sizeof(m_cooldownWindow));
                break;
        }
        switch (mode) {
            case SERIAL_LOAD:
                ar->Read(&m_timer2Stamp, sizeof(m_timer2Stamp));
                ar->Read(&m_timer2Window, sizeof(m_timer2Window));
                break;
            case SERIAL_SAVE:
                ar->Write(&m_timer2Stamp, sizeof(m_timer2Stamp));
                ar->Write(&m_timer2Window, sizeof(m_timer2Window));
                break;
        }
    }
    return 1;
fail:
    return 0;
}

VTBL(CWarlord, 0x001e7404);

RVA(0x00044640, 0x102)
void CWarlord::FireActivation(i32 key) {

    if (*CActRegPool<CWarlord>::s_table.ResolveEntry(key) != 0) {
        CActHandler h = *CActRegPool<CWarlord>::s_table.ResolveEntry(key);
        (this->*h)();
    }
}

RVA(0x000447a0, 0x333)
void RegisterWarlordActions() {
    REGISTER_ACTION("A", &CWarlord::RearmMoving);
    REGISTER_ACTION("B", &CWarlord::LoadAttributes);
    REGISTER_ACTION("C", &CWarlord::BuildFortSplashParticles);
    REGISTER_ACTION("D", &CWarlord::LoadAttributes2);
    REGISTER_ACTION("E", &CWarlord::AdvanceMovingAnim);
    REGISTER_ACTION_TYPED("F", &CWarlord::RearmMoving2);
}

#undef REGISTER_ACTION
#undef REGISTER_ACTION_TYPED
#undef REGISTER_NAME

RVA(0x00044bb0, 0x38)
i32 CWarlord::RearmMoving() {
    m_wwdObject->m_animCursor.Advance(g_engineFrameDelta);
    CAniAdvanceCursor* sub = &m_wwdObject->m_animCursor;
    if (sub->m_finished != 0 && sub->m_frameTicksLeft == 0) {
        ResolveMovingAnimation();
    }
    return 0;
}

RVA(0x00044c00, 0xc6)
i32 CWarlord::LoadAttributes() {
    if (m_wwdObject->m_animCursor.Advance(g_engineFrameDelta) != 1) {
        return 0;
    }

    CGruntzMgr* reg = g_gameReg;
    if (reg->m_gameMode != GAMEMODE_SINGLE) {
        CWwdGameObjectA* o = m_object;
        i32 dist = reg->m_cmdGrid->NearestCellDist(o->m_smarts, o->m_screenX, o->m_screenY);
        if (dist < g_buteMgr.GetIntDef("Warlordz", "PanicRadius", 0x40)) {
            NotifyFortUnderAttack();
            return 0;
        }
    }

    if (static_cast<i64>(static_cast<u32>(g_frameTime)) - m_cooldownStamp >= m_cooldownWindow) {
        if (rand() % 10 < 5) {
            ResolveIdleAnimation();
            return 0;
        }
        ResolveBattlecryAnimation();
    }
    return 0;
}

RVA(0x00044d10, 0x106)
i32 CWarlord::LoadAttributes2() {
    if (m_wwdObject->m_animCursor.Advance(g_engineFrameDelta) != 1) {
        return 0;
    }

    if (g_gameReg->m_gameMode != GAMEMODE_SINGLE) {
        CWwdGameObjectA* o = m_object;
        i32 dist = g_gameReg->m_cmdGrid->NearestCellDist(o->m_smarts, o->m_screenX, o->m_screenY);
        if (dist >= g_buteMgr.GetIntDef("Warlordz", "PanicRadius", 0x40)) {
            RaiseBattleAlert();
            return 0;
        }
    } else {

        if ((static_cast<CPlay*>(g_gameReg->m_curState))->m_frameMarker->m_currentMs == 0) {
            ResolveMovingAnimation();
            return 0;
        }
        if (static_cast<i64>(static_cast<u32>(g_frameTime)) - m_cooldownStamp >= m_cooldownWindow) {
            g_gameReg->m_cueSink->SpawnVoiceDriver(m_object->m_objectId, 0x436, -1, -1, -1);
            m_cooldownWindow = 0x7530;
            m_cooldownStamp = static_cast<u32>(g_frameTime);
        }
    }
    return 0;
}

RVA(0x00044e70, 0x87)
i32 CWarlord::AdvanceMovingAnim() {
    m_wwdObject->m_animCursor.Advance(g_engineFrameDelta);
    CAniAdvanceCursor* sub = &m_wwdObject->m_animCursor;
    if (sub->m_finished != 0 && sub->m_frameTicksLeft == 0) {
        CTriggerMgr* h = g_gameReg->m_cmdGrid;
        if (h->m_phase != 0 && m_object->m_smarts == g_curPlayer) {
            h->m_pendingFx = NULL;
            CueTimer* tm = &g_gameReg->m_cmdGrid->m_cueTimer;
            tm->m_window = 0x3e8;
            tm->m_base = static_cast<u32>(g_frameTime);
        }
        ResolveMovingAnimation();
    }
    return 0;
}

RVA(0x00044f30, 0x38)
i32 CWarlord::RearmMoving2() {
    m_wwdObject->m_animCursor.Advance(g_engineFrameDelta);
    CAniAdvanceCursor* sub = &m_wwdObject->m_animCursor;
    if (sub->m_finished != 0 && sub->m_frameTicksLeft == 0) {
        ResolveMovingAnimation();
    }
    return 0;
}

RVA(0x00044f80, 0x127)
i32 CWarlord::BuildFortSplashParticles() {
    m_wwdObject->m_animCursor.Advance(g_engineFrameDelta);
    CAniAdvanceCursor* sub = &m_wwdObject->m_animCursor;
    if (sub->m_finished != 0 && sub->m_frameTicksLeft == 0) {
        CWwdGameObjectA* o = m_object;
        i32 y = o->m_screenY;
        i32 x = o->m_screenX;
        if (x < g_gameReg->m_viewBounds.right && x >= g_gameReg->m_viewBounds.left
            && y < g_gameReg->m_viewBounds.bottom && y >= g_gameReg->m_viewBounds.top) {
            CWwdGameObjectA* fx =
                g_gameReg->m_world->m_childGroup
                    ->CreateSprite(0, x - 30, y + 10, SORTKEY_ACTOR_BEHIND, "Particlez", 0x40003);
            if (fx != NULL) {
                fx->ApplyName("LEVEL_FORTSPLASH");
                fx->ApplyLookupGeometry("LEVEL_FORTSPLASH", 0);
            }
        }

        CTriggerMgr* h = g_gameReg->m_cmdGrid;
        if (h->m_phase != 0 && m_object->m_smarts == g_curPlayer) {
            h->m_pendingFx = NULL;
            CueTimer* tm = &g_gameReg->m_cmdGrid->m_cueTimer;
            tm->m_window = 0x3e8;
            tm->m_base = static_cast<u32>(g_frameTime);
        }

        GruntzPlayer* slot = &g_gameReg->m_options[m_object->m_smarts];
        if (slot != NULL) {
            slot->m_warlordObjectId = 0;
        }
        m_wwdObject->m_flags |= 0x10000;
    }
    return 0;
}

// @early-stop
RVA(0x00045100, 0x112)
i32 CWarlord::ResolveMovingAnimation() {
    if (m_deathStarted != 0) {
        return 0;
    }

    m_wwdObject->ApplyName(s_GRUNTZ_ + m_warlordName + s__MOVING);

    m_value = m_wwdObject->m_animCursor.m_animation;
    m_wwdObject->m_animCursor.Setup(m_animMoving);

    m_prevAnimSetNode = m_objAux->m_actKey;
    m_objAux->m_actKey = ActFindId(s_keyB);

    m_cooldownWindow = static_cast<u32>((rand() % 0x5dc1 + 0x1770) * 10);
    m_cooldownStamp = static_cast<u32>(g_frameTime);
    return 1;
}

// @early-stop
RVA(0x00045270, 0x2a8)
i32 CWarlord::NotifyFortUnderAttack() {

    if (m_deathStarted == 0) {
        bool alreadyPanicking =
            (strcmp(*g_typeColl.GetNameRecord(m_objAux->m_actKey), s_codeD) == 0);
        if (!alreadyPanicking) {
            if (g_gameReg->m_gameMode == GAMEMODE_SINGLE) {
                g_gameReg->m_cueSink->SpawnVoiceDriver(m_object->m_objectId, 0x436, -1, -1, -1);
                m_cooldownWindow = 0x7530;
                m_cooldownStamp = static_cast<u32>(g_frameTime);
            } else {
                if (static_cast<i64>(static_cast<u32>(g_frameTime)) - m_timer2Stamp
                        >= m_timer2Window
                    && g_gameReg->m_cmdGrid->m_pendingFx == this) {
                    g_gameReg->m_cueSink->SpawnVoiceDriver(m_object->m_objectId, 0x440, -1, -1, -1);
                    static CString s_alert("ALERT - Your Fort is under attack!");
                    g_gameReg->m_chatLog->AddItem(
                        static_cast<LPCTSTR>(
                            *g_buteMgr.GetStringDef("Warlordz", "NotifyString", &s_alert)
                        ),
                        0,
                        0x11
                    );
                    m_timer2Window =
                        static_cast<u32>(g_buteMgr.GetIntDef("Warlordz", "NotifyTimer", 0x1770));
                    m_timer2Stamp = static_cast<u32>(g_frameTime);
                }
                m_cooldownWindow = static_cast<u32>((rand() % 0x5dc1 + 0x1770) * 10);
                m_cooldownStamp = static_cast<u32>(g_frameTime);
            }

            m_value = m_wwdObject->m_animCursor.m_animation;
            m_wwdObject->m_animCursor.Setup(m_animPanic);

            m_wwdObject->ApplyName(s_GRUNTZ_ + m_warlordName + s__PANIC);

            m_prevAnimSetNode = m_objAux->m_actKey;
            m_objAux->m_actKey = ActFindId(s_codeD);
            return 1;
        }
    }
    return 0;
}

// @early-stop
RVA(0x000455f0, 0x15b)
i32 CWarlord::ResolveDeathAnimation() {
    if (m_deathStarted != 0) {
        return 0;
    }
    m_deathStarted = 1;

    CGruntzMgr* g = g_gameReg;
    if (g->m_gameMode == GAMEMODE_SINGLE) {
        CWwdGameObjectA* h = m_object;
        i32 x = h->m_screenX;
        i32 y = h->m_screenY;
        if (x < g->m_viewBounds.right && x >= g->m_viewBounds.left && y < g->m_viewBounds.bottom
            && y >= g->m_viewBounds.top) {
            g->m_cueSink->SpawnVoiceDriver(h->m_objectId, m_ownerTag, -1, -1, -1);
        }
    } else {
        g->m_cueSink->SpawnVoiceDriver(m_object->m_objectId, m_ownerTag, -1, -1, -1);
    }

    CAniElement* anim = m_animDeath;
    m_value = m_wwdObject->m_animCursor.m_animation;
    m_wwdObject->m_animCursor.Setup(anim);

    m_wwdObject->ApplyName(s_GRUNTZ_ + m_warlordName + s__DEATH);

    m_prevAnimSetNode = m_objAux->m_actKey;
    m_objAux->m_actKey = ActFindId(s_keyC);
    return 1;
}

RVA(0x000457b0, 0x14c)
i32 CWarlord::RaiseBattleAlert() {
    if (m_deathStarted != 0) {
        return 0;
    }

    CGruntzMgr* g = g_gameReg;
    if (g->m_gameMode == GAMEMODE_SINGLE) {
        CWwdGameObjectA* h = m_object;
        i32 x = h->m_screenX;
        i32 y = h->m_screenY;
        if (x < g->m_viewBounds.right && x >= g->m_viewBounds.left && y < g->m_viewBounds.bottom
            && y >= g->m_viewBounds.top) {
            g->m_cueSink->SpawnVoiceDriver(h->m_objectId, 0x435, -1, -1, -1);
        }
    } else {
        g->m_cueSink->SpawnVoiceDriver(m_object->m_objectId, 0x43f, -1, -1, -1);
    }

    CAniElement* anim = m_animJoy;
    m_value = m_wwdObject->m_animCursor.m_animation;
    m_wwdObject->m_animCursor.Setup(anim);

    m_wwdObject->ApplyName(s_GRUNTZ_ + m_warlordName + s__JOY);

    m_prevAnimSetNode = m_objAux->m_actKey;
    m_objAux->m_actKey = ActFindId(s_keyE);
    return 1;
}

RVA(0x00045960, 0x181)
i32 CWarlord::ResolveIdleAnimation() {
    if (m_deathStarted != 0) {
        return 0;
    }

    i32 idx = rand() % 3 + 1;

    CGruntzMgr* g = g_gameReg;
    if (g->m_gameMode == GAMEMODE_SINGLE) {
        CWwdGameObjectA* h = m_object;

        i32 cue = idx + 0x431;
        i32 x = h->m_screenX;
        i32 y = h->m_screenY;
        if (x < g->m_viewBounds.right && x >= g->m_viewBounds.left && y < g->m_viewBounds.bottom
            && y >= g->m_viewBounds.top) {
            g->m_cueSink->SpawnVoiceDriver(h->m_objectId, cue, -1, -1, -1);
        }
    } else {
        g->m_cueSink->SpawnVoiceDriver(m_object->m_objectId, idx + 0x43b, -1, -1, -1);
    }

    CAniElement* anim = m_idleAnims[idx];
    m_value = m_wwdObject->m_animCursor.m_animation;
    m_wwdObject->m_animCursor.Setup(anim);

    CAniElement* desc = m_wwdObject->m_animCursor.m_animation;
    CAniRecordView* elem =
        desc->m_records.GetSize() > 0 ? static_cast<CAniRecordView*>(desc->m_records.GetAt(0)) : 0;
    i32 frame = elem->m_param;

    m_wwdObject->ApplyLookupSprite(s_GRUNTZ_ + m_warlordName + s__IDLE, frame);

    m_prevAnimSetNode = m_objAux->m_actKey;
    m_objAux->m_actKey = ActFindId(s_keyA);
    return 1;
}

RVA(0x00045b60, 0x161)
i32 CWarlord::ResolveBattlecryAnimation() {
    if (m_deathStarted != 0) {
        return 0;
    }

    i32 idx = rand() % 3;

    CGruntzMgr* g = g_gameReg;
    if (g->m_gameMode == GAMEMODE_SINGLE) {
        CWwdGameObjectA* h = m_object;
        i32 cue = idx + 0x42e;
        i32 x = h->m_screenX;
        i32 y = h->m_screenY;
        if (x < g->m_viewBounds.right && x >= g->m_viewBounds.left && y < g->m_viewBounds.bottom
            && y >= g->m_viewBounds.top) {
            g->m_cueSink->SpawnVoiceDriver(h->m_objectId, cue, -1, -1, -1);
        }
    } else {
        g->m_cueSink->SpawnVoiceDriver(m_object->m_objectId, idx + 0x438, -1, -1, -1);
    }

    CAniElement* anim = m_battlecryAnims[idx];
    m_value = m_wwdObject->m_animCursor.m_animation;
    m_wwdObject->m_animCursor.Setup(anim);

    m_wwdObject->ApplyName(s_GRUNTZ_ + m_warlordName + s__BATTLECRY);

    m_prevAnimSetNode = m_objAux->m_actKey;
    m_objAux->m_actKey = ActFindId(s_keyF);
    return 1;
}
