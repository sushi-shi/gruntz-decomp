#include <StdAfx.h>

#include <Ints.h>

#include <Gruntz/DroppedObject.h>

#include <DDrawMgr/DDrawChildGroup.h>
#include <Enums.h>
#include <Globals.h>
#include <Gruntz/ActName.h>
#include <Gruntz/ActNameRegistry.h>
#include <Gruntz/ActReg.h>
#include <Gruntz/ActRegistry.h>
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
#include <Gruntz/LogicRecordHandler.h>
#include <Gruntz/LogicTypeId.h>
#include <Gruntz/MapCellFlags.h>
#include <Gruntz/ObjectDropper.h>
#include <Gruntz/Particlez.h>
#include <Gruntz/ResolveNodeInline.h>
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
#include <RectMacros.h>
#include <Rez/FrameClock.h>
#include <Wap32/TileGeometry.h>
#include <ZTools/BitVec.h>
#include <ZTools/ZDArray.h>

#include <string.h>

const double g_objDropDiv = 32.0;

const double g_dropFallBias = -0.5;

template<>
CActReg CActRegPool<CObjectDropper>::s_table(ACT_ID_FIRST, ACT_ID_LAST);

template<>
CActReg CActRegPool<CDroppedObject>::s_table(ACT_ID_FIRST, ACT_ID_LAST);

template<>
CActReg CActRegPool<CDroppedObjectShadow>::s_table(ACT_ID_FIRST, ACT_ID_LAST);


i32 DispatchObjectDropperLogic(CGameObject* owner) {
    LOGIC_RECORD_DISPATCH(CObjectDropper)
}

i32 DispatchDroppedObjectLogic(CGameObject* owner) {
    LOGIC_RECORD_DISPATCH(CDroppedObject)
}

i32 DispatchDroppedObjectShadowLogic(CGameObject* owner) {
    LOGIC_RECORD_DISPATCH(CDroppedObjectShadow)
}

CObjectDropper::CObjectDropper(CGameObject* obj)
    : CUserLogic(obj, CUserLogic::INLINE_BASE), CWapX(obj) {
    SwitchAnimationByName("LEVEL_OBJECTDROPPER", 0);
    SET_ANIMATION_ACT("A");
    SetObjectFlags(WWD_GAME_OBJECT_FLAGS_CULL_SOUND_KEEP_ACTIVE);

    SNAP_OBJECT_TO_TILE_CENTER_DOUBLE_POS(m_object, snapX, snapY, m_posX, m_posY)
    CWwdSpriteObject* o = m_object;
    SET_SORT_KEY_IF_CHANGED(o, SORTKEY_ACTOR_FRONT)

    CDDrawWorker* frameSet = m_wwdObject->m_imageSet;
    if (frameSet != NULL) {
        std::string name;
        name = frameSet->m_name;
        if ((name).compare("LEVEL_OBJECTDROPPER_NORTH") == 0) {
            m_object->m_direction = IDX(CARDINAL_NORTH);
            m_travelDx = 0;
            m_travelDy = -1;
        } else if ((name).compare("LEVEL_OBJECTDROPPER_EAST") == 0) {
            m_object->m_direction = IDX(CARDINAL_EAST);
            m_travelDx = 1;
            m_travelDy = 0;
        } else if ((name).compare("LEVEL_OBJECTDROPPER_SOUTH") == 0) {
            m_object->m_direction = IDX(CARDINAL_SOUTH);
            m_travelDx = 0;
            m_travelDy = 1;
        } else if ((name).compare("LEVEL_OBJECTDROPPER_WEST") == 0) {
            m_object->m_direction = IDX(CARDINAL_WEST);
            m_travelDx = -1;
            m_travelDy = 0;
        }
    }

    i32 time = g_buteMgr.GetDword("Hazardz", "ObjectDropperTimePerTile", 1000);
    m_scrollMode = OBJECT_DROP_ALL_PLAYERS;
    m_lastDropPlayerIndex = -1;
    m_lastDropUnitIndex = -1;
    m_speed = g_objDropDiv / static_cast<double>(static_cast<u32>(time));
    if (g_gameReg->GetGameMode() == GAMEMODE_QUESTZ) {
        m_scrollMode = OBJECT_DROP_PLAYER_ZERO_ONLY;
    }
    CShadeTable* sel = g_gameReg->m_lightFxMgr->m_tables[5];
    m_object->SetDrawFill(SHADE_DST_BY_SRC_16, sel);
    m_dropTiming.Clear();
    SET_OBJECT_AREA(1)
}

void CObjectDropper::FireActivation(i32 actId) {
    DispatchRegisteredAct(this, actId);
}

void CObjectDropper::RegisterActs() {
    ACT_NAME_ID(id, "A")
    (CActRegPool<CObjectDropper>::s_table[id]) =
        static_cast<i32 (CUserLogic::*)()>(&CObjectDropper::Update);
}

i32 CObjectDropper::Update() {
    if (m_dropTiming.Expired()) {
        if (g_gameReg->m_isEasyMode == false || g_gameReg->GetGameMode() != GAMEMODE_QUESTZ) {
            CWwdSpriteObject* o = m_object;
            RECT box;
            SET_RECT_XY_EXTENTS(
                box,
                o->m_screenX - o->m_frameImage->m_anchorX + 7,
                o->m_screenX + o->m_frameImage->m_anchorX - 7,
                o->m_screenY - o->m_frameImage->m_anchorY + 7,
                o->m_screenY + o->m_frameImage->m_anchorY - 7
            );
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
                            g_gameReg->World()->ChildGroup()->CreateSprite(
                                0,
                                fx,
                                fy,
                                0,
                                "DroppedObjectShadow",
                                WWD_GAME_OBJECT_FLAGS_WORLD_SPRITE
                            );
                            m_lastDropPlayerIndex = playerIndex;
                            m_lastDropUnitIndex = unitIndex;
                            m_dropTiming.Start(
                                g_buteMgr.GetDword("Hazardz", "ObjectDropperDelay", 1000)
                            );
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
            >= static_cast<double>(g_gameReg->World()->m_level->m_mainPlane->m_planePixelWidth)) {
            m_posX = 0.0;
            m_lastDropPlayerIndex = -1;
            m_lastDropUnitIndex = -1;
        }
    } else if (m_travelDx < 0) {
        m_posX -= drift;
        if (m_posX < 0.0) {
            m_posX = static_cast<double>(
                (g_gameReg->World()->m_level->m_mainPlane->m_planePixelWidth - 1)
            );
            m_lastDropPlayerIndex = -1;
            m_lastDropUnitIndex = -1;
        }
    }
    if (m_travelDy > 0) {
        m_posY += drift;
        if (m_posY
            > static_cast<double>(g_gameReg->World()->m_level->m_mainPlane->m_planePixelHeight)) {
            m_posY = 0.0;
            m_lastDropPlayerIndex = -1;
            m_lastDropUnitIndex = -1;
        }
    } else if (m_travelDy < 0) {
        m_posY -= drift;
        if (m_posY < 0.0) {
            m_posY = static_cast<double>(
                (g_gameReg->World()->m_level->m_mainPlane->m_planePixelHeight - 1)
            );
            m_lastDropPlayerIndex = -1;
            m_lastDropUnitIndex = -1;
        }
    }

    SET_SCREEN_POS(m_object, static_cast<i32>(m_posX), static_cast<i32>(m_posY));
    return 0;
}

i32 CObjectDropper::SerializeDispatch(
    CFileMemBase* ar,
    SerialMode mode,
    LogicTypeId typeId,
    CGameObject* object
) {
    SERIALIZE_USER_LOGIC_AND_ANIMATION_STATE_OR_RETURN(ar, mode, typeId, object)

    SerializeClockPair(ar, mode, &m_dropTiming);

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
            o->SetDrawFillReversed(SHADE_DST_BY_SRC_16, fill);
            break;
        }
    }
    return 1;
}

CDroppedObject::CDroppedObject(CGameObject* obj)
    : CUserLogic(obj, CUserLogic::INLINE_BASE), CWapX(obj) {
    SET_ANIMATION_ACT("A");
    SetImageSetByName("LEVEL_OBJECTDROPPER_OBJECT");
    SwitchAnimationByName("LEVEL_DROPPEDOBJECT", 0);
    SetObjectFlags(WWD_GAME_OBJECT_FLAGS_CULL_SOUND_KEEP_ACTIVE);
    i32 adjY = (m_object->m_screenY & ~TILE_MASK_PX) + TILE_HALF_PX;
    i32 adjX = (m_object->m_screenX & ~TILE_MASK_PX) + TILE_HALF_PX;
    m_landY = adjY;
    SET_SCREEN_POS(
        m_object,
        adjX,
        adjY - g_buteMgr.GetInt("Hazardz", "DroppedObjectYOffset", 0x140)
    );
    CWwdSpriteObject* o = m_object;
    m_fallY = static_cast<double>(o->m_screenY);
    SET_SORT_KEY_IF_CHANGED(o, SORTKEY_ACTOR_FRONT)
    m_timePerTile =
        g_objDropDiv
        / static_cast<double>(g_buteMgr.GetDword("Hazardz", "DroppedObjectTimePerTile", 0x3e8));
}

void CDroppedObject::FireActivation(i32 coord) {
    DispatchRegisteredAct(this, coord);
}

void CDroppedObject::RegisterActs() {
    ACT_NAME_ID(id, "A")
    CActRegPool<CDroppedObject>::s_table[id] =

        static_cast<i32 (CUserLogic::*)()>(&CDroppedObject::AdvanceFall);

    ACT_NAME_ID(id2, "B")
    CActRegPool<CDroppedObject>::s_table[id2] =
        static_cast<i32 (CUserLogic::*)()>(&CDroppedObject::AdvanceImpactAnimation);
}

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

                        case AREA_MINIATURE_MASTERZ:
                        default:
                            if (::PtInRect(&g_gameReg->m_viewBounds, x, m_landY)) {
                                CreateParticlez(
                                    g_gameReg->World()->ChildGroup(),
                                    x,
                                    m_landY,
                                    "LEVEL_DEATHSPLASH",
                                    "LEVEL_DEATHSPLASH"
                                );
                            }
                            break;
                        case AREA_HONEY_I_SHRUNK_THE_GRUNTZ:
                            break;
                    }
                }
            }
        } else {
            if (::PtInRect(&g_gameReg->m_viewBounds, x, m_landY)) {
                CreateParticlez(
                    g_gameReg->World()->ChildGroup(),
                    x,
                    m_landY,
                    "GAME_WATER",
                    "GAME_WATER"
                );
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

i32 CDroppedObject::AdvanceAnimation() {
    m_wwdObject->m_animationCursor.Advance(g_engineFrameDelta);
    MARK_OBJECT_COMPLETE_IF(m_wwdObject->m_animationCursor.IsComplete())
    return 0;
}

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
    SET_SORT_KEY_IF_CHANGED(o, SORTKEY_ACTOR_BEHIND)
}

void CDroppedObjectShadow::FireActivation(i32 coord) {
    DispatchRegisteredAct(this, coord);
}

void CDroppedObjectShadow::RegisterActs() {
    ACT_NAME_ID(id, "A")
    (CActRegPool<CDroppedObjectShadow>::s_table[id]) =
        static_cast<i32 (CUserLogic::*)()>(&CDroppedObjectShadow::Advance);
}

i32 CDroppedObjectShadow::Advance() {
    if (m_wwdObject->m_animationCursor.Advance(g_engineFrameDelta) == WWDDRAW_EFFECT_FRAME) {
        CWwdSpriteObject* o = m_object;
        g_gameReg->World()->ChildGroup()->CreateSprite(
            0,
            o->m_screenX,
            o->m_screenY,
            0,
            "DroppedObject",
            WWD_GAME_OBJECT_FLAGS_WORLD_SPRITE
        );
    }
    MARK_OBJECT_COMPLETE_IF(m_wwdObject->m_animationCursor.IsComplete())
    return 0;
}

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

i32 CDroppedObject::AdvanceImpactAnimation() {
    return AdvanceAnimation();
}
