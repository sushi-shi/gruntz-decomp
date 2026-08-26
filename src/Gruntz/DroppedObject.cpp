#include <rva.h>

#include <Gruntz/DroppedObject.h>

#include <Bute/ButeTree.h>
#include <DDrawMgr/DDrawChildGroup.h>
#include <Enums.h>
#include <Gruntz/ActName.h>
#include <Gruntz/ActNameRegistry.h>
#include <Gruntz/ActReg.h>
#include <Gruntz/AniAdvanceCursor.h>
#include <Gruntz/AniAdvanceCursorInline.h>
#include <Gruntz/Brickz.h>
#include <Gruntz/CardinalDir.h>
#include <Gruntz/DroppedObjectShadow.h>
#include <Gruntz/GameLevel.h>
#include <Gruntz/GameModeId.h>
#include <Gruntz/GameObjectLogicTypes.h>
#include <Gruntz/GameRegistry.h>
#include <Gruntz/GameRegMfcPtr.h>
#include <Gruntz/Grunt.h>
#include <Gruntz/GruntAreaEffectKind.h>
#include <Gruntz/GruntDirStatics.h>
#include <Gruntz/GruntzMgr.h>
#include <Gruntz/LevelArea.h>
#include <Gruntz/LightFxMgr.h>
#include <Gruntz/LogicEventDispatch.h>
#include <Gruntz/LogicTypeId.h>
#include <Gruntz/MapCellFlags.h>
#include <Gruntz/ObjectDropper.h>
#include <Gruntz/SerialArchive.h>
#include <Gruntz/SortKeyLayer.h>
#include <Gruntz/SortKeyMacros.h>
#include <Gruntz/State.h>
#include <Gruntz/TileSnapMacros.h>
#include <Gruntz/TriggerMgr.h>
#include <Gruntz/TypeKeyColl.h>
#include <Gruntz/UserLogic.h>
#include <Image/CImage.h>
#include <Io/FileMem.h>
#include <Rez/FrameClock.h>
#include <Wap32/TileGeometry.h>
#include <Wap32/zBitVec.h>
#include <Wap32/ZVec.h>
#include <Wwd/LogicRecordEvent.h>

#include <string.h>

DATA(0x001ea9f0)
const double g_objDropDiv = 32.0;
DATA(0x001eaa00)
const double g_dropFallBias = -0.5;

RVA_DYNINIT(0x000c5ee0, 0xa, CActRegPool<CObjectDropper>::s_table)
RVA_DYNINIT(0x000c5f00, 0x15, CActRegPool<CObjectDropper>::s_table)
RVA_DYNINIT(0x000c5f30, 0xe, CActRegPool<CObjectDropper>::s_table)
RVA_DYNINIT(0x000c5f50, 0x1f, CActRegPool<CObjectDropper>::s_table)
template<> DATA(0x0024be90)
CActReg CActRegPool<CObjectDropper>::s_table(ACT_ID_FIRST, ACT_ID_LAST);
RVA_DYNINIT(0x000c6b30, 0xa, CActRegPool<CDroppedObject>::s_table)
RVA_DYNINIT(0x000c6b50, 0x15, CActRegPool<CDroppedObject>::s_table)
RVA_DYNINIT(0x000c6b80, 0xe, CActRegPool<CDroppedObject>::s_table)
RVA_DYNINIT(0x000c6ba0, 0x1f, CActRegPool<CDroppedObject>::s_table)
template<> DATA(0x0024bed8)
CActReg CActRegPool<CDroppedObject>::s_table(ACT_ID_FIRST, ACT_ID_LAST);
RVA_DYNINIT(0x000c76b0, 0xa, CActRegPool<CDroppedObjectShadow>::s_table)
RVA_DYNINIT(0x000c76d0, 0x15, CActRegPool<CDroppedObjectShadow>::s_table)
RVA_DYNINIT(0x000c7700, 0xe, CActRegPool<CDroppedObjectShadow>::s_table)
RVA_DYNINIT(0x000c7720, 0x1f, CActRegPool<CDroppedObjectShadow>::s_table)
template<> DATA(0x0024bf00)
CActReg CActRegPool<CDroppedObjectShadow>::s_table(ACT_ID_FIRST, ACT_ID_LAST);

struct CString;

static inline CActHandler* DropLookup(i32 coord) {
    return (CActRegPool<CDroppedObject>::s_table.ResolveEntry(coord));
}

RVA_COMPGEN(0x000124c0, 0x1e, ??_GCObjectDropper@@UAEPAXI@Z)
RVA_COMPGEN(0x000124f0, 0x44, ??1CObjectDropper@@UAE@XZ)

RVA_COMPGEN(0x00012580, 0x1e, ??_GCDroppedObject@@UAEPAXI@Z)
RVA_COMPGEN(0x000125b0, 0x44, ??1CDroppedObject@@UAE@XZ)

RVA_COMPGEN(0x00012640, 0x1e, ??_GCDroppedObjectShadow@@UAEPAXI@Z)
RVA_COMPGEN(0x00012670, 0x44, ??1CDroppedObjectShadow@@UAE@XZ)

RVA(0x000c5630, 0xf4)
i32 DispatchObjectDropperLogic(CGameObject* obj) {
    CLogicRecord* record = obj->m_logicRecord;
    switch (record->LogicEvent()) {
        case ACT_UNINITIALISED: {
            record->SetLogicEvent(ACT_LIVE);
            CObjectDropper* h = new CObjectDropper(obj);
            h->Activate();
            record->m_userLogic = h;
            break;
        }
        case ACT_OBJECT_REMOVED:
            record->m_userLogic->OnObjectRemoved();
            break;
        case ACT_LEAVE_ACTIVE_REGION:
            record->m_userLogic->OnLeaveActiveRegion();
            break;
        case ACT_PREPARE_SAVE:
            record->m_userLogic->PrepareSave();
            break;
        case ACT_AFTER_SAVE:
            record->m_userLogic->AfterSave();
            break;
        case ACT_AFTER_LOAD:
            record->m_userLogic->AfterLoad();
            break;
        case ACT_AFTER_LOAD_REFERENCES:
            record->m_userLogic->AfterLoadReferences();
            break;
        case ACT_LIVE:
            break;
        default:
            DispatchLogicEvent(record->m_userLogic);
            break;
    }
    return 1;
}

RVA(0x000c5770, 0xf1)
i32 DispatchDroppedObjectLogic(CGameObject* obj) {
    CLogicRecord* record = obj->m_logicRecord;
    switch (record->LogicEvent()) {
        case ACT_UNINITIALISED: {
            record->SetLogicEvent(ACT_LIVE);
            CDroppedObject* h = new CDroppedObject(obj);
            h->Activate();
            record->m_userLogic = h;
            break;
        }
        case ACT_OBJECT_REMOVED:
            record->m_userLogic->OnObjectRemoved();
            break;
        case ACT_LEAVE_ACTIVE_REGION:
            record->m_userLogic->OnLeaveActiveRegion();
            break;
        case ACT_PREPARE_SAVE:
            record->m_userLogic->PrepareSave();
            break;
        case ACT_AFTER_SAVE:
            record->m_userLogic->AfterSave();
            break;
        case ACT_AFTER_LOAD:
            record->m_userLogic->AfterLoad();
            break;
        case ACT_AFTER_LOAD_REFERENCES:
            record->m_userLogic->AfterLoadReferences();
            break;
        case ACT_LIVE:
            break;
        default:
            DispatchLogicEvent(record->m_userLogic);
            break;
    }
    return 1;
}

RVA(0x000c58b0, 0xf1)
i32 DispatchDroppedObjectShadowLogic(CGameObject* obj) {
    CLogicRecord* record = obj->m_logicRecord;
    switch (record->LogicEvent()) {
        case ACT_UNINITIALISED: {
            record->SetLogicEvent(ACT_LIVE);
            CDroppedObjectShadow* h = new CDroppedObjectShadow(obj);
            h->Activate();
            record->m_userLogic = h;
            break;
        }
        case ACT_OBJECT_REMOVED:
            record->m_userLogic->OnObjectRemoved();
            break;
        case ACT_LEAVE_ACTIVE_REGION:
            record->m_userLogic->OnLeaveActiveRegion();
            break;
        case ACT_PREPARE_SAVE:
            record->m_userLogic->PrepareSave();
            break;
        case ACT_AFTER_SAVE:
            record->m_userLogic->AfterSave();
            break;
        case ACT_AFTER_LOAD:
            record->m_userLogic->AfterLoad();
            break;
        case ACT_AFTER_LOAD_REFERENCES:
            record->m_userLogic->AfterLoadReferences();
            break;
        case ACT_LIVE:
            break;
        default:
            DispatchLogicEvent(record->m_userLogic);
            break;
    }
    return 1;
}

// @early-stop
RVA(0x000c59f0, 0x3e3)
CObjectDropper::CObjectDropper(CGameObject* obj)
    : CUserLogic(obj, CUserLogic::INLINE_BASE), CWapX(obj) {
    m_lastDropTime = 0;
    m_dropInterval = 0;
    SwitchAnimationByName("LEVEL_OBJECTDROPPER", 0);
    SET_ANIMATION_ACT("A");
    SetObjectFlags(WWD_GAME_OBJECT_FLAGS_CULL_SOUND_KEEP_ACTIVE);

    SNAP_OBJECT_TO_TILE_CENTER_DOUBLE_POS(m_object, snapX, snapY, m_posX, m_posY)
    CWwdSpriteObject* o = m_object;
    SET_SORT_KEY_IF_CHANGED(o, SORTKEY_ACTOR_FRONT)

    CDDrawWorker* frameSet = m_wwdObject->m_imageSet;
    if (frameSet != NULL) {
        CString name;
        name = frameSet->m_name;
        const char* s = name;
        if (strcmp(s, "LEVEL_OBJECTDROPPER_NORTH") == 0) {
            m_object->m_direction = IDX(CARDINAL_NORTH);
            m_travelDx = 0;
            m_travelDy = -1;
        } else if (strcmp(s, "LEVEL_OBJECTDROPPER_EAST") == 0) {
            m_object->m_direction = IDX(CARDINAL_EAST);
            m_travelDx = 1;
            m_travelDy = 0;
        } else if (strcmp(s, "LEVEL_OBJECTDROPPER_SOUTH") == 0) {
            m_object->m_direction = IDX(CARDINAL_SOUTH);
            m_travelDx = 0;
            m_travelDy = 1;
        } else if (strcmp(s, "LEVEL_OBJECTDROPPER_WEST") == 0) {
            m_object->m_direction = IDX(CARDINAL_WEST);
            m_travelDx = -1;
            m_travelDy = 0;
        }
    }

    i32 time = g_buteMgr.GetDwordDef("Hazardz", "ObjectDropperTimePerTile", 1000);
    m_scrollMode = OBJECT_DROP_ALL_PLAYERS;
    m_lastDropPlayerIndex = -1;
    m_lastDropUnitIndex = -1;
    m_speed = g_objDropDiv / static_cast<double>(static_cast<u32>(time));
    if (g_gameReg->m_gameMode == GAMEMODE_QUESTZ) {
        m_scrollMode = OBJECT_DROP_PLAYER_ZERO_ONLY;
    }
    CShadeTable* sel = g_gameReg->m_lightFxMgr->m_tables[5];
    SET_DRAW_FILL(m_object, SHADE_DST_BY_SRC_16, sel);
    m_lastDropTime = 0;
    m_dropInterval = 0;
    SET_OBJECT_AREA(1)
}

RVA(0x000c5f80, 0x102)
void CObjectDropper::FireActivation(i32 actId) {
    if ((*((CActRegPool<CObjectDropper>::s_table.ResolveEntry(actId)))) != NULL) {
        (this->*((*((CActRegPool<CObjectDropper>::s_table.ResolveEntry(actId))))))();
    }
}

RVA(0x000c60e0, 0x18d)
void CObjectDropper::RegisterActs() {
    ACT_NAME_ID(id, "A")
    (*((CActRegPool<CObjectDropper>::s_table.ResolveEntry(id)))) =
        static_cast<i32 (CUserLogic::*)()>(&CObjectDropper::Update);
}

RVA(0x000c62e0, 0x2dd)
i32 CObjectDropper::Update() {
    if (static_cast<i64>(g_frameTime) - m_lastDropTime >= m_dropInterval) {
        if (g_gameReg->m_isEasyMode == false || g_gameReg->m_gameMode != GAMEMODE_QUESTZ) {
            CWwdSpriteObject* o = m_object;
            RECT box;
            box.left = o->m_screenX - o->m_frameImage->m_anchorX + 7;
            box.right = o->m_screenX + o->m_frameImage->m_anchorX - 7;
            box.top = o->m_screenY - o->m_frameImage->m_anchorY + 7;
            box.bottom = o->m_screenY + o->m_frameImage->m_anchorY - 7;
            i32 playerIndex;
            i32 unitIndex;
            CGrunt* found = g_gameReg->m_triggerMgr->FindGruntAt(
                o->m_screenX,
                o->m_screenY,
                &o->m_area,
                &playerIndex,
                &unitIndex,
                &box
            );
            if (found != NULL) {
                if (m_lastDropPlayerIndex != playerIndex || m_lastDropUnitIndex != unitIndex) {
                    if (m_scrollMode == OBJECT_DROP_ALL_PLAYERS || playerIndex == 0) {
                        CGameObject* fo = found->m_object;
                        i32 fx = fo->m_screenX;
                        i32 fy = fo->m_screenY;
                        CMapMgr* plane = g_gameReg->m_tileGrid;
                        i32 cx = fx >> TILE_SHIFT_PX;
                        i32 cy = fy >> TILE_SHIFT_PX;
                        u32 flags = plane->CellFlagsAt(cx, cy);
                        if ((flags & IDX(CELL_FLAG_SPECIAL)) == 0) {
                            g_gameReg->m_world->m_childGroup->CreateSprite(
                                0,
                                fx,
                                fy,
                                0,
                                "DroppedObjectShadow",
                                WWD_GAME_OBJECT_FLAGS_WORLD_SPRITE
                            );
                            m_lastDropPlayerIndex = playerIndex;
                            m_lastDropUnitIndex = unitIndex;
                            m_dropInterval =
                                g_buteMgr.GetDwordDef("Hazardz", "ObjectDropperDelay", 1000);
                            m_lastDropTime = g_frameTime;
                        }
                    }
                }
            }
        }
    }

    m_wwdObject->m_animationCursor.Advance(static_cast<i32>(g_engineFrameDelta));

    double drift = static_cast<double>(g_frameDelta) * m_speed;
    if (m_travelDx > 0) {
        m_posX += drift;
        if (m_posX
            >= static_cast<double>(g_gameReg->m_world->m_level->m_mainPlane->m_planePixelWidth)) {
            m_posX = 0.0;
            m_lastDropPlayerIndex = -1;
            m_lastDropUnitIndex = -1;
        }
    } else if (m_travelDx < 0) {
        m_posX -= drift;
        if (m_posX < 0.0) {
            m_posX = static_cast<double>(
                (g_gameReg->m_world->m_level->m_mainPlane->m_planePixelWidth - 1)
            );
            m_lastDropPlayerIndex = -1;
            m_lastDropUnitIndex = -1;
        }
    }
    if (m_travelDy > 0) {
        m_posY += drift;
        if (m_posY
            > static_cast<double>(g_gameReg->m_world->m_level->m_mainPlane->m_planePixelHeight)) {
            m_posY = 0.0;
            m_lastDropPlayerIndex = -1;
            m_lastDropUnitIndex = -1;
        }
    } else if (m_travelDy < 0) {
        m_posY -= drift;
        if (m_posY < 0.0) {
            m_posY = static_cast<double>(
                (g_gameReg->m_world->m_level->m_mainPlane->m_planePixelHeight - 1)
            );
            m_lastDropPlayerIndex = -1;
            m_lastDropUnitIndex = -1;
        }
    }

    m_object->m_screenX = static_cast<i32>(m_posX);
    m_object->m_screenY = static_cast<i32>(m_posY);
    return 0;
}

RVA(0x000c6680, 0x1b4)
i32 CObjectDropper::SerializeDispatch(
    CFileMemBase* ar,
    SerialMode mode,
    LogicTypeId typeId,
    CGameObject* object
) {
    SERIALIZE_USER_LOGIC_AND_ANIMATION_STATE_OR_RETURN(ar, mode, typeId, object)

    SerBandPair(ar, mode, &m_dropTiming);

    switch (mode) {
        case SERIAL_SAVE:
            ar->Write(&m_speed, sizeof(m_speed));
            ar->Write(&m_posX, sizeof(m_posX));
            ar->Write(&m_posY, sizeof(m_posY));
            ar->Write(&m_travelDx, sizeof(m_travelDx));
            ar->Write(&m_travelDy, sizeof(m_travelDy));
            ar->Write(&m_lastDropPlayerIndex, sizeof(m_lastDropPlayerIndex));
            ar->Write(&m_lastDropUnitIndex, sizeof(m_lastDropUnitIndex));
            ar->Write(&m_scrollMode, sizeof(m_scrollMode));
            break;
        case SERIAL_LOAD:
            ar->Read(&m_speed, sizeof(m_speed));
            ar->Read(&m_posX, sizeof(m_posX));
            ar->Read(&m_posY, sizeof(m_posY));
            ar->Read(&m_travelDx, sizeof(m_travelDx));
            ar->Read(&m_travelDy, sizeof(m_travelDy));
            ar->Read(&m_lastDropPlayerIndex, sizeof(m_lastDropPlayerIndex));
            ar->Read(&m_lastDropUnitIndex, sizeof(m_lastDropUnitIndex));
            ar->Read(&m_scrollMode, sizeof(m_scrollMode));
            break;
        case SERIAL_POSTLOAD: {
            CShadeTable* fill = g_gameReg->m_lightFxMgr->m_tables[5];
            CWwdSpriteObject* o = m_object;
            SET_DRAW_FILL_REVERSED(o, SHADE_DST_BY_SRC_16, fill);
            break;
        }
    }
    return 1;
}

// @early-stop
RVA(0x000c68b0, 0x1f5)
CDroppedObject::CDroppedObject(CGameObject* obj)
    : CUserLogic(obj, CUserLogic::INLINE_BASE), CWapX(obj) {
    SET_ANIMATION_ACT("A");
    SetImageSetByName("LEVEL_OBJECTDROPPER_OBJECT");
    SwitchAnimationByName("LEVEL_DROPPEDOBJECT", 0);
    SetObjectFlags(WWD_GAME_OBJECT_FLAGS_CULL_SOUND_KEEP_ACTIVE);
    i32 adjY = (m_object->m_screenY & ~TILE_MASK_PX) + TILE_HALF_PX;
    i32 adjX = (m_object->m_screenX & ~TILE_MASK_PX) + TILE_HALF_PX;
    m_landY = adjY;
    m_object->m_screenX = adjX;
    m_object->m_screenY = adjY - g_buteMgr.GetIntDef("Hazardz", "DroppedObjectYOffset", 0x140);
    CWwdSpriteObject* o = m_object;
    m_fallY = static_cast<double>(o->m_screenY);
    SET_SORT_KEY_IF_CHANGED(o, SORTKEY_ACTOR_FRONT)
    m_timePerTile =
        g_objDropDiv
        / static_cast<double>(g_buteMgr.GetDwordDef("Hazardz", "DroppedObjectTimePerTile", 0x3e8));
}

RVA(0x000c6bd0, 0x102)
void CDroppedObject::FireActivation(i32 coord) {
    CActHandler* e = DropLookup(coord);
    if ((*e) != NULL) {
        CActHandler* e2 = DropLookup(coord);
        (this->*((*e2)))();
    }
}

RVA(0x000c6d30, 0x2ac)
void CDroppedObject::RegisterActs() {
    ACT_NAME_ID_CALL_REPORT(id, "A")
    *(CActRegPool<CDroppedObject>::s_table.ResolveEntryCallReport(id)) =

        static_cast<i32 (CUserLogic::*)()>(&CDroppedObject::AdvanceFall);

    ACT_NAME_ID(id2, "B")
    *(CActRegPool<CDroppedObject>::s_table.ResolveEntryCallReport(id2)) =
        static_cast<i32 (CUserLogic::*)()>(&CDroppedObject::AdvanceImpactAnimation);
}

RVA(0x000c7090, 0x230)
i32 CDroppedObject::AdvanceFall() {
    m_wwdObject->m_animationCursor.Advance(g_engineFrameDelta);
    m_fallY = static_cast<double>(g_frameDelta) * m_timePerTile + m_fallY;
    i32 landed = static_cast<i32>((m_fallY - g_dropFallBias));
    if (landed > m_landY) {
        i32 x = m_object->m_screenX;
        CMapMgr* g = g_gameReg->m_tileGrid;
        i32 cell;
        {
            i32 cx = x >> TILE_SHIFT_PX;
            i32 cy = m_landY >> TILE_SHIFT_PX;
            cell = g->CellFlagsAt(cx, cy);
        }
        if ((cell & 0x900) == 0) {
            if (cell & IDX(CELL_FLAG_SPECIAL)) {
                if (cell == IDX(CELL_FLAG_REVEALED_POWERUP)) {
                    SetObjectFlags(IDX(WWD_GAME_OBJECT_FLAG_PENDING_DELETE));
                } else {
                    switch (g_gameReg->m_curState->m_levelType) {
                        case AREA_HIGH_ON_SWEETZ:
                        case AREA_HIGH_ROLLERZ:
                        case AREA_GRUNTZ_IN_SPACE:
                            SetObjectFlags(IDX(WWD_GAME_OBJECT_FLAG_PENDING_DELETE));
                            // fall through
                        case AREA_MINIATURE_MASTERZ:
                        default:
                            if (CGameLevel::PointInRect(&g_gameReg->m_viewBounds, x, m_landY)) {
                                CWwdSpriteObject* s =
                                    g_gameReg->m_world->m_childGroup->CreateSprite(
                                        0,
                                        x,
                                        m_landY,
                                        SORTKEY_ACTOR_BEHIND,
                                        "Particlez",
                                        WWD_GAME_OBJECT_FLAGS_WORLD_SPRITE
                                    );
                                if (s != NULL) {
                                    s->SetImageSetByName("LEVEL_DEATHSPLASH");
                                    s->SetAnimationByName("LEVEL_DEATHSPLASH", 0);
                                }
                            }
                            break;
                        case AREA_HONEY_I_SHRUNK_THE_GRUNTZ:
                            break;
                    }
                }
            }
        } else {
            if (CGameLevel::PointInRect(&g_gameReg->m_viewBounds, x, m_landY)) {
                CWwdSpriteObject* s = g_gameReg->m_world->m_childGroup->CreateSprite(
                    0,
                    x,
                    m_landY,
                    SORTKEY_ACTOR_BEHIND,
                    "Particlez",
                    WWD_GAME_OBJECT_FLAGS_WORLD_SPRITE
                );
                if (s != NULL) {
                    s->SetImageSetByName("GAME_WATER");
                    s->SetAnimationByName("GAME_WATER", 0);
                }
            }
        }
        SwitchAnimationByName("LEVEL_DROPPEDOBJECTHIT", 0);
        SET_ANIMATION_ACT("B");
        g_gameReg->m_triggerMgr
            ->ApplyGruntAreaEffect(m_object->m_screenX, m_landY, 1, GRUNT_AREA_EFFECT_SQUASH, -1);
        return 0;
    }
    m_object->m_screenY = landed;
    return 0;
}

RVA(0x000c7350, 0x39)
i32 CDroppedObject::AdvanceAnimation() {
    m_wwdObject->m_animationCursor.Advance(g_engineFrameDelta);
    MARK_OBJECT_COMPLETE_IF(IsAniCursorComplete(&m_wwdObject->m_animationCursor))
    return 0;
}

RVA(0x000c73a0, 0xb5)
i32 CDroppedObject::SerializeDispatch(
    CFileMemBase* ar,
    SerialMode mode,
    LogicTypeId typeId,
    CGameObject* object
) {
    SERIALIZE_USER_LOGIC_AND_ANIMATION_STATE_OR_RETURN(ar, mode, typeId, object)
    switch (mode) {
        case SERIAL_SAVE:
            ar->Write(&m_timePerTile, sizeof(m_timePerTile));
            ar->Write(&m_fallY, sizeof(m_fallY));
            ar->Write(&m_landY, sizeof(m_landY));
            break;
        case SERIAL_LOAD:
            ar->Read(&m_timePerTile, sizeof(m_timePerTile));
            ar->Read(&m_fallY, sizeof(m_fallY));
            ar->Read(&m_landY, sizeof(m_landY));
            break;
    }
    return 1;
}

RVA(0x000c7490, 0x1a6)
CDroppedObjectShadow::CDroppedObjectShadow(CGameObject* obj)
    : CUserLogic(obj, CUserLogic::INLINE_BASE), CWapX(obj) {
    SET_ANIMATION_ACT("A");
    SetImageSetByName("LEVEL_OBJECTDROPPER_SHADOW");
    SwitchAnimationByName("LEVEL_DROPPEDOBJECTSHADOW", 0);
    SetObjectFlags(WWD_GAME_OBJECT_FLAGS_CULL_SOUND_KEEP_ACTIVE);
    CShadeTable* fill = g_gameReg->m_lightFxMgr->m_tables[5];
    CWwdSpriteObject* draw = m_object;
    SET_DRAW_FILL(draw, SHADE_DST_BY_SRC_16, fill);
    CWwdSpriteObject* o = m_object;
    SET_SORT_KEY_IF_CHANGED(o, SORTKEY_ACTOR_BEHIND)
}

RVA(0x000c7750, 0x102)
void CDroppedObjectShadow::FireActivation(i32 coord) {
    if ((*((CActRegPool<CDroppedObjectShadow>::s_table.ResolveEntry(coord)))) != NULL) {
        (this->*((*((CActRegPool<CDroppedObjectShadow>::s_table.ResolveEntry(coord))))))();
    }
}

RVA(0x000c78b0, 0x18d)
void CDroppedObjectShadow::RegisterActs() {
    ACT_NAME_ID(id, "A")
    (*((CActRegPool<CDroppedObjectShadow>::s_table.ResolveEntry(id)))) =
        static_cast<i32 (CUserLogic::*)()>(&CDroppedObjectShadow::Advance);
}

// @early-stop
RVA(0x000c7ab0, 0x67)
i32 CDroppedObjectShadow::Advance() {
    if (m_wwdObject->m_animationCursor.Advance(g_engineFrameDelta) == WWDDRAW_EFFECT_FRAME) {
        CWwdSpriteObject* o = m_object;
        g_gameReg->m_world->m_childGroup->CreateSprite(
            0,
            o->m_screenX,
            o->m_screenY,
            0,
            "DroppedObject",
            WWD_GAME_OBJECT_FLAGS_WORLD_SPRITE
        );
    }
    MARK_OBJECT_COMPLETE_IF(IsAniCursorComplete(&m_wwdObject->m_animationCursor))
    return 0;
}

RVA(0x000c7b40, 0x76)
i32 CDroppedObjectShadow::SerializeDispatch(
    CFileMemBase* ar,
    SerialMode mode,
    LogicTypeId typeId,
    CGameObject* object
) {
    SERIALIZE_USER_LOGIC_AND_ANIMATION_STATE_OR_RETURN(ar, mode, typeId, object)
    if (mode == SERIAL_POSTLOAD) {
        CShadeTable* fill = g_gameReg->m_lightFxMgr->m_tables[5];
        CWwdSpriteObject* o = m_object;
        SET_DRAW_FILL(o, SHADE_DST_BY_SRC_16, fill);
    }
    return 1;
}

RVA(0x000c7be0, 0x5)
i32 CDroppedObject::AdvanceImpactAnimation() {
    return AdvanceAnimation();
}
