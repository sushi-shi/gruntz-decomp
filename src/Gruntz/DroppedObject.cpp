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

    Coord position = m_object->ScreenPos();
    SnapTileCenter(&position);
    m_object->SetScreenPos(position);
    m_position.Init(position);
    CWwdSpriteObject* o = m_object;
    o->SetSortKey(SORTKEY_ACTOR_FRONT);

    CDDrawWorker* frameSet = m_wwdObject->m_imageSet;
    if (frameSet != NULL) {
        CString name;
        name = frameSet->m_name;
        const char* s = name;
        if (strcmp(s, "LEVEL_OBJECTDROPPER_NORTH") == 0) {
            m_object->m_direction = IDX(CARDINAL_NORTH);
            m_travelDirection.Set(0, -1);
        } else if (strcmp(s, "LEVEL_OBJECTDROPPER_EAST") == 0) {
            m_object->m_direction = IDX(CARDINAL_EAST);
            m_travelDirection.Set(1, 0);
        } else if (strcmp(s, "LEVEL_OBJECTDROPPER_SOUTH") == 0) {
            m_object->m_direction = IDX(CARDINAL_SOUTH);
            m_travelDirection.Set(0, 1);
        } else if (strcmp(s, "LEVEL_OBJECTDROPPER_WEST") == 0) {
            m_object->m_direction = IDX(CARDINAL_WEST);
            m_travelDirection.Set(-1, 0);
        }
    }

    i32 time = g_buteMgr.GetDword("Hazardz", "ObjectDropperTimePerTile", 1000);
    m_scrollMode = OBJECT_DROP_ALL_PLAYERS;
    m_lastDropPlayerIndex = -1;
    m_lastDropUnitIndex = -1;
    m_speed = g_objDropDiv / static_cast<double>(static_cast<u32>(time));
    if (g_gameReg->m_gameMode == GAMEMODE_QUESTZ) {
        m_scrollMode = OBJECT_DROP_PLAYER_ZERO_ONLY;
    }
    CShadeTable* sel = g_gameReg->m_lightFxMgr->m_tables[5];
    m_object->SetDrawFill(SHADE_DST_BY_SRC_16, sel);
    m_lastDropTime = 0;
    m_dropInterval = 0;
    SetObjectArea(1);
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
            Coord dropperPosition = o->ScreenPos();
            CRect box(
                dropperPosition.m_x - o->m_frameImage->m_anchor.x + 7,
                dropperPosition.m_y - o->m_frameImage->m_anchor.y + 7,
                dropperPosition.m_x + o->m_frameImage->m_anchor.x - 7,
                dropperPosition.m_y + o->m_frameImage->m_anchor.y - 7
            );
            i32 playerIndex;
            i32 unitIndex;
            CGrunt* found = g_gameReg->m_triggerMgr->FindGruntAt(
                dropperPosition.m_x,
                dropperPosition.m_y,
                &o->m_area,
                &playerIndex,
                &unitIndex,
                &box
            );
            if (found != NULL) {
                if (m_lastDropPlayerIndex != playerIndex || m_lastDropUnitIndex != unitIndex) {
                    if (m_scrollMode == OBJECT_DROP_ALL_PLAYERS || playerIndex == 0) {
                        Coord foundPosition = found->m_object->ScreenPos();
                        CMapMgr* plane = g_gameReg->m_tileGrid;
                        Coord foundTile = foundPosition;
                        ScreenTile(&foundTile);
                        u32 flags = plane->CellFlagsAt(foundTile.m_x, foundTile.m_y);
                        if ((flags & IDX(CELL_FLAG_SPECIAL)) == 0) {
                            g_gameReg->m_world->m_childGroup->CreateSprite(
                                0,
                                foundPosition.m_x,
                                foundPosition.m_y,
                                0,
                                "DroppedObjectShadow",
                                WWD_GAME_OBJECT_FLAGS_WORLD_SPRITE
                            );
                            m_lastDropPlayerIndex = playerIndex;
                            m_lastDropUnitIndex = unitIndex;
                            m_dropInterval =
                                g_buteMgr.GetDword("Hazardz", "ObjectDropperDelay", 1000);
                            m_lastDropTime = g_frameTime;
                        }
                    }
                }
            }
        }
    }

    m_wwdObject->m_animationCursor.Advance(static_cast<i32>(g_engineFrameDelta));

    double drift = static_cast<double>(g_frameDelta) * m_speed;
    if (m_travelDirection.m_x > 0) {
        m_position.x += drift;
        if (m_position.x
            >= static_cast<double>(g_gameReg->m_world->m_level->m_mainPlane->m_planePixelSize.cx)) {
            m_position.x = 0.0;
            m_lastDropPlayerIndex = -1;
            m_lastDropUnitIndex = -1;
        }
    } else if (m_travelDirection.m_x < 0) {
        m_position.x -= drift;
        if (m_position.x < 0.0) {
            m_position.x = static_cast<double>(
                (g_gameReg->m_world->m_level->m_mainPlane->m_planePixelSize.cx - 1)
            );
            m_lastDropPlayerIndex = -1;
            m_lastDropUnitIndex = -1;
        }
    }
    if (m_travelDirection.m_y > 0) {
        m_position.y += drift;
        if (m_position.y
            > static_cast<double>(g_gameReg->m_world->m_level->m_mainPlane->m_planePixelSize.cy)) {
            m_position.y = 0.0;
            m_lastDropPlayerIndex = -1;
            m_lastDropUnitIndex = -1;
        }
    } else if (m_travelDirection.m_y < 0) {
        m_position.y -= drift;
        if (m_position.y < 0.0) {
            m_position.y = static_cast<double>(
                (g_gameReg->m_world->m_level->m_mainPlane->m_planePixelSize.cy - 1)
            );
            m_lastDropPlayerIndex = -1;
            m_lastDropUnitIndex = -1;
        }
    }

    Coord position = m_position.ToCoord();
    m_object->SetScreenPos(position);
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
            ar->Write(&m_position.x, sizeof(m_position.x));
            ar->Write(&m_position.y, sizeof(m_position.y));
            ar->Write(&m_travelDirection.m_x, sizeof(m_travelDirection.m_x));
            ar->Write(&m_travelDirection.m_y, sizeof(m_travelDirection.m_y));
            ar->Write(&m_lastDropPlayerIndex, sizeof(m_lastDropPlayerIndex));
            ar->Write(&m_lastDropUnitIndex, sizeof(m_lastDropUnitIndex));
            ar->Write(&m_scrollMode, sizeof(m_scrollMode));
            break;
        case SERIAL_LOAD:
            ar->Read(&m_speed, sizeof(m_speed));
            ar->Read(&m_position.x, sizeof(m_position.x));
            ar->Read(&m_position.y, sizeof(m_position.y));
            ar->Read(&m_travelDirection.m_x, sizeof(m_travelDirection.m_x));
            ar->Read(&m_travelDirection.m_y, sizeof(m_travelDirection.m_y));
            ar->Read(&m_lastDropPlayerIndex, sizeof(m_lastDropPlayerIndex));
            ar->Read(&m_lastDropUnitIndex, sizeof(m_lastDropUnitIndex));
            ar->Read(&m_scrollMode, sizeof(m_scrollMode));
            break;
        case SERIAL_POSTLOAD: {
            CShadeTable* fill = g_gameReg->m_lightFxMgr->m_tables[5];
            CWwdSpriteObject* o = m_object;
            o->SetDrawFill(SHADE_DST_BY_SRC_16, fill);
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
    Coord adjusted = m_object->m_screenPosition;
    SnapTileCenter(&adjusted);
    m_landY = adjusted.m_y;
    m_object->SetScreenPos(
        adjusted.m_x,
        adjusted.m_y - g_buteMgr.GetInt("Hazardz", "DroppedObjectYOffset", 0x140)
    );
    CWwdSpriteObject* o = m_object;
    m_fallY = static_cast<double>(o->m_screenPosition.m_y);
    o->SetSortKey(SORTKEY_ACTOR_FRONT);
    m_timePerTile =
        g_objDropDiv
        / static_cast<double>(g_buteMgr.GetDword("Hazardz", "DroppedObjectTimePerTile", 0x3e8));
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
        Coord landing(m_object->m_screenPosition.m_x, m_landY);
        CMapMgr* g = g_gameReg->m_tileGrid;
        i32 cell;
        {
            Coord tile = landing;
            ScreenTile(&tile);
            cell = g->CellFlagsAt(tile.m_x, tile.m_y);
        }
        if ((cell & IDX(CELL_FLAG_WATER | CELL_FLAG_SINK_HAZARD)) == 0) {
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
                            if (::PtInRect(&g_gameReg->m_viewBounds, landing.m_x, landing.m_y)) {
                                CWwdSpriteObject* s =
                                    g_gameReg->m_world->m_childGroup->CreateSprite(
                                        0,
                                        landing.m_x,
                                        landing.m_y,
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
            if (::PtInRect(&g_gameReg->m_viewBounds, landing.m_x, landing.m_y)) {
                CWwdSpriteObject* s = g_gameReg->m_world->m_childGroup->CreateSprite(
                    0,
                    landing.m_x,
                    landing.m_y,
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
            ->ApplyGruntAreaEffect(landing.m_x, landing.m_y, 1, GRUNT_AREA_EFFECT_SQUASH, -1);
        return 0;
    }
    m_object->m_screenPosition.m_y = landed;
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
    draw->SetDrawFill(SHADE_DST_BY_SRC_16, fill);
    CWwdSpriteObject* o = m_object;
    o->SetSortKey(SORTKEY_ACTOR_BEHIND);
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
            o->m_screenPosition.m_x,
            o->m_screenPosition.m_y,
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
        o->SetDrawFill(SHADE_DST_BY_SRC_16, fill);
    }
    return 1;
}

RVA(0x000c7be0, 0x5)
i32 CDroppedObject::AdvanceImpactAnimation() {
    return AdvanceAnimation();
}
