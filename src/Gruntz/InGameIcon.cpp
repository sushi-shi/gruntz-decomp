#include <StdAfx.h>

#include <rva.h>

#include <Gruntz/InGameIcon.h>

#include <Bute/ButeMgr.h>
#include <DDrawMgr/DDrawChildGroup.h>
#include <Enums.h>
#include <Globals.h>
#include <Gruntz/ActNameRegistry.h>
#include <Gruntz/ActReg.h>
#include <Gruntz/ActRegistry.h>
#include <Gruntz/AniAdvanceCursor.h>
#include <Gruntz/AniElement.h>
#include <Gruntz/AnimationRegistry.h>
#include <Gruntz/Brickz.h>
#include <Gruntz/ColorTint.h>
#include <Gruntz/GameLevel.h>
#include <Gruntz/GameModeId.h>
#include <Gruntz/GameRand.h>
#include <Gruntz/GameRegistry.h>
#include <Gruntz/GameRegMfcPtr.h>
#include <Gruntz/Grunt.h>
#include <Gruntz/GruntzMgr.h>
#include <Gruntz/InGameText.h>
#include <Gruntz/LogicFnTable.h>
#include <Gruntz/LogicTypeId.h>
#include <Gruntz/MapCellFlags.h>
#include <Gruntz/MapCellInline.h>
#include <Gruntz/PickupType.h>
#include <Gruntz/Play.h>
#include <Gruntz/ResolveNodeInline.h>
#include <Gruntz/SerialArchive.h>
#include <Gruntz/SerialRecords.h>
#include <Gruntz/SerialRefLookup.h>
#include <Gruntz/SortKeyLayer.h>
#include <Gruntz/SoundCue.h>
#include <Gruntz/SoundCueInline.h>
#include <Gruntz/SoundCueRegistry.h>
#include <Gruntz/SoundState.h>
#include <Gruntz/SpellId.h>
#include <Gruntz/SpriteRefTable.h>
#include <Gruntz/SpriteStateFlags.h>
#include <Gruntz/TileSnapMacros.h>
#include <Gruntz/ToyPeek.h>
#include <Gruntz/TriggerMgr.h>
#include <Gruntz/TypeKeyColl.h>
#include <Gruntz/WapSerializationInline.h>
#include <Gruntz/WarpStoneFragment.h>
#include <Io/FileMem.h>
#include <Rez/FrameClock.h>
#include <Utils/MapTyped.h>
#include <Wap32/TileGeometry.h>
#include <ZTools/BitVec.h>
#include <ZTools/ZDArray.h>

#include <string.h>

RVA_DYNINIT(0x000977e0, 0xa, CActRegPool<CInGameIcon>::s_table)
RVA_DYNINIT(0x00097800, 0x15, CActRegPool<CInGameIcon>::s_table)
RVA_DYNINIT(0x00097830, 0xe, CActRegPool<CInGameIcon>::s_table)
RVA_DYNINIT(0x00097850, 0x1f, CActRegPool<CInGameIcon>::s_table)
template<> DATA(0x002458b0)
CActReg CActRegPool<CInGameIcon>::s_table(ACT_ID_FIRST, ACT_ID_LAST);
RVA_DYNINIT(0x00097d40, 0xa, CActRegPool<CToyPeek>::s_table)
RVA_DYNINIT(0x00097d60, 0x15, CActRegPool<CToyPeek>::s_table)
RVA_DYNINIT(0x00097d90, 0xe, CActRegPool<CToyPeek>::s_table)
RVA_DYNINIT(0x00097db0, 0x1f, CActRegPool<CToyPeek>::s_table)
template<> DATA(0x00245928)
CActReg CActRegPool<CToyPeek>::s_table(ACT_ID_FIRST, ACT_ID_LAST);
RVA_DYNINIT(0x000993c0, 0xa, CActRegPool<CInGameText>::s_table)
RVA_DYNINIT(0x000993e0, 0x15, CActRegPool<CInGameText>::s_table)
RVA_DYNINIT(0x00099410, 0xe, CActRegPool<CInGameText>::s_table)
RVA_DYNINIT(0x00099430, 0x1f, CActRegPool<CInGameText>::s_table)
template<> DATA(0x00245950)
CActReg CActRegPool<CInGameText>::s_table(ACT_ID_FIRST, ACT_ID_LAST);

RVA_COMPGEN(0x00011c10, 0x1e, ??_GCToyPeek@@UAEPAXI@Z)

RVA_COMPGEN(0x00011c40, 0x44, ??1CToyPeek@@UAE@XZ)

RVA_COMPGEN(0x00011cd0, 0x1e, ??_GCInGameIcon@@UAEPAXI@Z)
RVA_COMPGEN(0x00011d00, 0x44, ??1CInGameIcon@@UAE@XZ)

RVA_COMPGEN(0x00011d90, 0x1e, ??_GCInGameText@@UAEPAXI@Z)
RVA_COMPGEN(0x00011dc0, 0x44, ??1CInGameText@@UAE@XZ)

// @early-stop
RVA(0x00095b10, 0x15f0)
CInGameIcon::CInGameIcon(CGameObject* obj) : CUserLogic(obj, CUserLogic::INLINE_BASE), CWapX(obj) {
    SNAP_OBJECT_TO_TILE_CENTER_COPY(m_object, snapX, snapY)

    CWwdSpriteObject* snapped = m_object;
    snapped->SetSortKey(SORTKEY_INGAME_INFO);

    SET_ANIMATION_ACT("A");
    SwitchAnimationByName("GAME_CYCLE100", 0);

    SetObjectFlags(IDX(WWD_GAME_OBJECT_FLAG_KEEP_ACTIVE));
    SetPickupSoundCue(NULL);

    m_glitterSprite = NULL;
    m_colorCycleTimer.Clear();

    InGameIconGlitter glitter = ICON_GLITTER_NONE;
    CImageSet* frameSet = m_wwdObject->GetImageSet();
    if (frameSet != NULL) {
        CString name;
        name = frameSet->GetName();

        if (name.Compare("GAME_INGAMEICONZ_TOOLZ_BOMBZ") == 0) {
            m_object->SetSmarts(IDX(PICKUP_BOMB));
            SetPickupSoundCue("GAME_TREASURE");
        } else if (name.Compare("GAME_INGAMEICONZ_TOOLZ_BOOMERANGZ") == 0) {
            m_object->SetSmarts(IDX(PICKUP_BOOMERANG));
            SetPickupSoundCue("GAME_TREASURE");
        } else if (name.Compare("GAME_INGAMEICONZ_TOOLZ_BRICKZ") == 0) {
            m_object->SetSmarts(IDX(PICKUP_BRICK));
            SetPickupSoundCue("GAME_TREASURE");
        } else if (name.Compare("GAME_INGAMEICONZ_TOOLZ_CLUBZ") == 0) {
            m_object->SetSmarts(IDX(PICKUP_CLUB));
            SetPickupSoundCue("GAME_TREASURE");
        } else if (name.Compare("GAME_INGAMEICONZ_TOOLZ_GAUNTLETZ") == 0) {
            m_object->SetSmarts(IDX(PICKUP_GAUNTLETZ));
            SetPickupSoundCue("GAME_TREASURE");
        } else if (name.Compare("GAME_INGAMEICONZ_TOOLZ_GLOVEZ") == 0) {
            m_object->SetSmarts(IDX(PICKUP_GLOVEZ));
            SetPickupSoundCue("GAME_TREASURE");
        } else if (name.Compare("GAME_INGAMEICONZ_TOOLZ_GOOBERZ") == 0) {
            m_object->SetSmarts(IDX(PICKUP_GOOBER));
            SetPickupSoundCue("GAME_TREASURE");
        } else if (name.Compare("GAME_INGAMEICONZ_TOOLZ_GRAVITYBOOTZ") == 0) {
            m_object->SetSmarts(IDX(PICKUP_GRAVITYBOOTZ));
            SetPickupSoundCue("GAME_TREASURE");
        } else if (name.Compare("GAME_INGAMEICONZ_TOOLZ_GUNHATZ") == 0) {
            m_object->SetSmarts(IDX(PICKUP_GUNHAT));
            SetPickupSoundCue("GAME_TREASURE");
        } else if (name.Compare("GAME_INGAMEICONZ_TOOLZ_NERFGUNZ") == 0) {
            m_object->SetSmarts(IDX(PICKUP_NERFGUN));
            SetPickupSoundCue("GAME_TREASURE");
        } else if (name.Compare("GAME_INGAMEICONZ_TOOLZ_ROCKZ") == 0) {
            m_object->SetSmarts(IDX(PICKUP_ROCK));
            SetPickupSoundCue("GAME_TREASURE");
        } else if (name.Compare("GAME_INGAMEICONZ_TOOLZ_SHIELDZ") == 0) {
            m_object->SetSmarts(IDX(PICKUP_SHIELD));
            SetPickupSoundCue("GAME_TREASURE");
        } else if (name.Compare("GAME_INGAMEICONZ_TOOLZ_SHOVELZ") == 0) {
            m_object->SetSmarts(IDX(PICKUP_SHOVEL));
            SetPickupSoundCue("GAME_TREASURE");
        } else if (name.Compare("GAME_INGAMEICONZ_TOOLZ_SPRINGZ") == 0) {
            m_object->SetSmarts(IDX(PICKUP_SPRING));
            SetPickupSoundCue("GAME_TREASURE");
        } else if (name.Compare("GAME_INGAMEICONZ_TOOLZ_SPYZ") == 0) {
            m_object->SetSmarts(IDX(PICKUP_SPY));
            SetPickupSoundCue("GAME_TREASURE");
        } else if (name.Compare("GAME_INGAMEICONZ_TOOLZ_SWORDZ") == 0) {
            m_object->SetSmarts(IDX(PICKUP_SWORD));
            SetPickupSoundCue("GAME_TREASURE");
        } else if (name.Compare("GAME_INGAMEICONZ_TOOLZ_TIMEBOMBZ") == 0) {
            m_object->SetSmarts(IDX(PICKUP_TIMEBOMB));
            SetPickupSoundCue("GAME_TREASURE");
        } else if (name.Compare("GAME_INGAMEICONZ_TOOLZ_TOOBZ") == 0) {
            m_object->SetSmarts(IDX(PICKUP_TOOB));
            SetPickupSoundCue("GAME_TREASURE");
        } else if (name.Compare("GAME_INGAMEICONZ_TOOLZ_WANDZ") == 0) {
            m_object->SetSmarts(IDX(PICKUP_WAND));
            SetPickupSoundCue("GAME_TREASURE");
        } else if (name.Compare("GAME_INGAMEICONZ_TOOLZ_WARPSTONEZ1") == 0) {
            m_object->SetSmarts(IDX(PICKUP_WARPSTONE));
            m_object->SetHealth(IDX(WARPSTONE_FRAGMENT_FIRST));
            CPlay* lvl = static_cast<CPlay*>(g_gameReg->GetCurrentState());
            i32 anchorX = m_object->m_screenX;
            i32 anchorY = m_object->m_screenY;
            lvl->m_anchors[0].m_x = anchorX;
            lvl->m_anchors[0].m_y = anchorY;
            SetPickupSoundCue("GAME_TREASURE");
        } else if (name.Compare("GAME_INGAMEICONZ_TOOLZ_WARPSTONEZ2") == 0) {
            m_object->SetSmarts(IDX(PICKUP_WARPSTONE));
            m_object->SetHealth(IDX(WARPSTONE_FRAGMENT_SECOND));
            CPlay* lvl = static_cast<CPlay*>(g_gameReg->GetCurrentState());
            i32 anchorX = m_object->m_screenX;
            i32 anchorY = m_object->m_screenY;
            lvl->m_anchors[1].m_x = anchorX;
            lvl->m_anchors[1].m_y = anchorY;
            SetPickupSoundCue("GAME_TREASURE");
        } else if (name.Compare("GAME_INGAMEICONZ_TOOLZ_WARPSTONEZ3") == 0) {
            m_object->SetSmarts(IDX(PICKUP_WARPSTONE));
            m_object->SetHealth(IDX(WARPSTONE_FRAGMENT_THIRD));
            CPlay* lvl = static_cast<CPlay*>(g_gameReg->GetCurrentState());
            i32 anchorX = m_object->m_screenX;
            i32 anchorY = m_object->m_screenY;
            lvl->m_anchors[2].m_x = anchorX;
            lvl->m_anchors[2].m_y = anchorY;
            SetPickupSoundCue("GAME_TREASURE");
        } else if (name.Compare("GAME_INGAMEICONZ_TOOLZ_WARPSTONEZ4") == 0) {
            m_object->SetSmarts(IDX(PICKUP_WARPSTONE));
            m_object->SetHealth(IDX(WARPSTONE_FRAGMENT_FOURTH));
            CPlay* lvl = static_cast<CPlay*>(g_gameReg->GetCurrentState());
            i32 anchorX = m_object->m_screenX;
            i32 anchorY = m_object->m_screenY;
            lvl->m_anchors[3].m_x = anchorX;
            lvl->m_anchors[3].m_y = anchorY;
            SetPickupSoundCue("GAME_TREASURE");
        } else if (name.Compare("GAME_INGAMEICONZ_TOOLZ_WELDERZ") == 0) {
            m_object->SetSmarts(IDX(PICKUP_WELDER));
            SetPickupSoundCue("GAME_TREASURE");
        } else if (name.Compare("GAME_INGAMEICONZ_TOOLZ_WINGZ") == 0) {
            m_object->SetSmarts(IDX(PICKUP_WINGZ));
            SetPickupSoundCue("GAME_TREASURE");
        } else if (name.Compare("GAME_INGAMEICONZ_TOYZ_BABYWALKERZ") == 0) {
            m_object->SetSmarts(IDX(PICKUP_BABYWALKER));
            SetPickupSoundCue("GAME_TREASURE");
        } else if (name.Compare("GAME_INGAMEICONZ_TOYZ_BEACHBALLZ") == 0) {
            m_object->SetSmarts(IDX(PICKUP_BEACHBALL));
            SetPickupSoundCue("GAME_TREASURE");
        } else if (name.Compare("GAME_INGAMEICONZ_TOYZ_BIGWHEELZ") == 0) {
            m_object->SetSmarts(IDX(PICKUP_BIGWHEEL));
            SetPickupSoundCue("GAME_TREASURE");
        } else if (name.Compare("GAME_INGAMEICONZ_TOYZ_GOKARTZ") == 0) {
            m_object->SetSmarts(IDX(PICKUP_GOKART));
            SetPickupSoundCue("GAME_TREASURE");
        } else if (name.Compare("GAME_INGAMEICONZ_TOYZ_JACKINTHEBOXZ") == 0) {
            m_object->SetSmarts(IDX(PICKUP_JACKINTHEBOX));
            SetPickupSoundCue("GAME_TREASURE");
        } else if (name.Compare("GAME_INGAMEICONZ_TOYZ_JUMPROPEZ") == 0) {
            m_object->SetSmarts(IDX(PICKUP_JUMPROPE));
            SetPickupSoundCue("GAME_TREASURE");
        } else if (name.Compare("GAME_INGAMEICONZ_TOYZ_POGOSTICKZ") == 0) {
            m_object->SetSmarts(IDX(PICKUP_POGOSTICK));
            SetPickupSoundCue("GAME_TREASURE");
        } else if (name.Compare("GAME_INGAMEICONZ_TOYZ_SCROLLZ") == 0) {
            m_object->SetSmarts(IDX(PICKUP_SCROLL));
            SetPickupSoundCue("GAME_TREASURE");
        } else if (name.Compare("GAME_INGAMEICONZ_TOYZ_SQUEAKTOYZ") == 0) {
            m_object->SetSmarts(IDX(PICKUP_SQUEAKTOY));
            SetPickupSoundCue("GAME_TREASURE");
        } else if (name.Compare("GAME_INGAMEICONZ_TOYZ_YOYOZ") == 0) {
            m_object->SetSmarts(IDX(PICKUP_YOYO));
            SetPickupSoundCue("GAME_TREASURE");
        } else if (name.Compare("GAME_INGAMEICONZ_POWERUPZ_MEGAPHONEZ") == 0) {
            m_object->SetSmarts(IDX(PICKUP_MEGAPHONE));
            SetPickupSoundCue("GAME_TREASURE");
        } else if (name.Compare("GAME_INGAMEICONZ_POWERUPZ_HEALTH1") == 0) {
            m_object->SetSmarts(IDX(PICKUP_HEALTH1));
            SetPickupSoundCue("GAME_TREASURE");
        } else if (name.Compare("GAME_INGAMEICONZ_POWERUPZ_HEALTH2") == 0) {
            m_object->SetSmarts(IDX(PICKUP_HEALTH2));
            SetPickupSoundCue("GAME_TREASURE");
        } else if (name.Compare("GAME_INGAMEICONZ_POWERUPZ_HEALTH3") == 0) {
            m_object->SetSmarts(IDX(PICKUP_HEALTH3));
            SetPickupSoundCue("GAME_TREASURE");
        } else if (name.Compare("GAME_INGAMEICONZ_POWERUPZ_CONVERSION") == 0) {
            m_object->SetSmarts(IDX(PICKUP_CONVERSION));
            SetPickupSoundCue("GAME_POWERUP");
            glitter = ICON_GLITTER_POWERUP_RED;
        } else if (name.Compare("GAME_INGAMEICONZ_POWERUPZ_DEATHTOUCH") == 0) {
            m_object->SetSmarts(IDX(PICKUP_DEATHTOUCH));
            SetPickupSoundCue("GAME_POWERUP");
            glitter = ICON_GLITTER_POWERUP_RED;
        } else if (name.Compare("GAME_INGAMEICONZ_POWERUPZ_GHOST") == 0) {
            m_object->SetSmarts(IDX(PICKUP_GHOST));
            SetPickupSoundCue("GAME_POWERUP");
            glitter = ICON_GLITTER_POWERUP_RED;
        } else if (name.Compare("GAME_INGAMEICONZ_POWERUPZ_INVULNERABILITY") == 0) {
            m_object->SetSmarts(IDX(PICKUP_INVULNERABILITY));
            SetPickupSoundCue("GAME_POWERUP");
            glitter = ICON_GLITTER_POWERUP_RED;
        } else if (name.Compare("GAME_INGAMEICONZ_POWERUPZ_REACTIVEARMOR") == 0) {
            m_object->SetSmarts(IDX(PICKUP_REACTIVEARMOR));
            SetPickupSoundCue("GAME_POWERUP");
            glitter = ICON_GLITTER_POWERUP_RED;
        } else if (name.Compare("GAME_INGAMEICONZ_POWERUPZ_ROIDZ") == 0) {
            m_object->SetSmarts(IDX(PICKUP_ROIDZ));
            SetPickupSoundCue("GAME_POWERUP");
            glitter = ICON_GLITTER_POWERUP_RED;
        } else if (name.Compare("GAME_INGAMEICONZ_POWERUPZ_SUPERSPEED") == 0) {
            m_object->SetSmarts(IDX(PICKUP_SUPERSPEED));
            SetPickupSoundCue("GAME_POWERUP");
            glitter = ICON_GLITTER_POWERUP_RED;
        } else if (name.Compare("GAME_INGAMEICONZ_SECRETW") == 0) {
            if (g_gameReg->GetEasyMode() != false && g_gameReg->GetGameMode() == GAMEMODE_QUESTZ) {
                SetObjectFlags(IDX(WWD_GAME_OBJECT_FLAG_PENDING_DELETE));
                return;
            }
            m_object->SetSmarts(IDX(PICKUP_W));
            SetPickupSoundCue("GAME_POWERUP");
        } else if (name.Compare("GAME_INGAMEICONZ_SECRETA") == 0) {
            if (g_gameReg->GetEasyMode() != false && g_gameReg->GetGameMode() == GAMEMODE_QUESTZ) {
                SetObjectFlags(IDX(WWD_GAME_OBJECT_FLAG_PENDING_DELETE));
                return;
            }
            m_object->SetSmarts(IDX(PICKUP_A));
            SetPickupSoundCue("GAME_POWERUP");
        } else if (name.Compare("GAME_INGAMEICONZ_SECRETR") == 0) {
            if (g_gameReg->GetEasyMode() != false && g_gameReg->GetGameMode() == GAMEMODE_QUESTZ) {
                SetObjectFlags(IDX(WWD_GAME_OBJECT_FLAG_PENDING_DELETE));
                return;
            }
            m_object->SetSmarts(IDX(PICKUP_R));
            SetPickupSoundCue("GAME_POWERUP");
        } else if (name.Compare("GAME_INGAMEICONZ_SECRETP") == 0) {
            if (g_gameReg->GetEasyMode() != false && g_gameReg->GetGameMode() == GAMEMODE_QUESTZ) {
                SetObjectFlags(IDX(WWD_GAME_OBJECT_FLAG_PENDING_DELETE));
                return;
            }
            m_object->SetSmarts(IDX(PICKUP_P));
            SetPickupSoundCue("GAME_POWERUP");
        } else if (name.Compare("GAME_INGAMEICONZ_POWERUPZ_STOPWATCH") == 0) {
            m_object->SetSmarts(IDX(PICKUP_STOPWATCH));
            SetPickupSoundCue("GAME_TREASURE");
        } else if (name.Compare("GAME_INGAMEICONZ_POWERUPZ_COIN") == 0) {
            m_object->SetSmarts(IDX(PICKUP_COIN));
            SetPickupSoundCue("GAME_TREASURE");
        } else if (name.Compare("GAME_TOYBOX") == 0) {
            m_object->SetSmarts(IDX(PICKUP_TOYBOX));
            SetPickupSoundCue("GAME_TREASURE");
        } else if (name.Compare("GAME_INGAMEICONZ_POWERUPZ_MINICAM") == 0) {
            m_object->SetSmarts(IDX(PICKUP_MINICAM));
            glitter = ICON_GLITTER_CURSE_GREEN;
            SetPickupSoundCue("GAME_CURSE");
        } else if (name.Compare("GAME_INGAMEICONZ_POWERUPZ_SCREENSHAKE") == 0) {
            m_object->SetSmarts(IDX(PICKUP_SCREENSHAKE));
            glitter = ICON_GLITTER_CURSE_GREEN;
            SetPickupSoundCue("GAME_CURSE");
        } else if (name.Compare("GAME_INGAMEICONZ_POWERUPZ_RANDOMCOLORZ") == 0) {
            m_object->SetSmarts(IDX(PICKUP_RANDOMCOLORZ));
            glitter = ICON_GLITTER_CURSE_GREEN;
            SetPickupSoundCue("GAME_CURSE");
        } else if (name.Compare("GAME_INGAMEICONZ_POWERUPZ_BLACKSCREEN") == 0) {
            m_object->SetSmarts(IDX(PICKUP_BLACKSCREEN));
            glitter = ICON_GLITTER_CURSE_GREEN;
            SetPickupSoundCue("GAME_CURSE");
        }
    }

    PickupType pickup = GetPickupType();
    if (pickup == PICKUP_WARPSTONE && g_gameReg->GetGameMode() == GAMEMODE_QUESTZ) {
        CPlay* lvl = static_cast<CPlay*>(g_gameReg->GetCurrentState());
        CString levelStr;
        levelStr.Format("Level%i", lvl->m_levelIndex);
        CString warpName;
        i32 target = g_buteMgr.GetInt("WarpStone", levelStr);
        warpName.Format("GAME_INGAMEICONZ_TOOLZ_WARPSTONEZ%i", target);
        m_object->SetImageSetByName(warpName);
        m_object->SetHealth(target);
    }

    if (glitter != ICON_GLITTER_NONE) {
        CWwdSpriteObject* fx = g_gameReg->World()->ChildGroup()->CreateSprite(
            0,
            m_object->m_screenX,
            m_object->m_screenY,
            SORTKEY_INGAME_INFO_FX,
            "SimpleAnimation",
            WWD_GAME_OBJECT_FLAGS_WORLD_SPRITE
        );
        m_glitterSprite = fx;
        if (glitter == ICON_GLITTER_POWERUP_RED) {
            fx->SetImageSetByName("GAME_GLITTERRED");
        }
        if (glitter == ICON_GLITTER_CURSE_GREEN) {
            m_glitterSprite->SetImageSetByName("GAME_GLITTERGREEN");
        }
        m_glitterSprite->SetAnimationByName("GAME_CYCLE100", 0);
    }

    if (ApplyPickupPalette() == 0) {
        SetObjectFlags(IDX(WWD_GAME_OBJECT_FLAG_PENDING_DELETE));
        return;
    }

    g_gameReg->m_tileGrid->SetObjectIdAt(
        m_object->m_screenX >> TILE_SHIFT_PX,
        m_object->m_screenY >> TILE_SHIFT_PX,
        m_object->GetObjectId()
    );
    m_object->Show();
}

RVA(0x00097680, 0x110)
i32 CInGameIcon::ApplyPickupPalette() {
    CWwdSpriteObject* sprite = m_object;
    PickupType pickupType = GetPickupType();
    CShadeTable* palette;
    if (pickupType == PICKUP_TOYBOX) {
        i32 playerIndex = GetPlayerIndex();
        PickupType toyType = GetToyType();
        if (toyType < PICKUP_TOYZ_FIRST || toyType > PICKUP_TOYZ_LAST) {
            return 0;
        }
        i32 colorIndex = IDX(g_gameReg->GetPlayer(playerIndex).GetColor());
        if (colorIndex < 0 || colorIndex >= TINT_COUNT) {
            colorIndex = IDX(TINT_ORANGE);
        }
        palette = g_gameReg->GruntPalettes()->GetShadeTable(colorIndex, 0);
        if (palette == NULL) {
            palette = g_gameReg->GruntPalettes()->GetShadeTable(IDX(TINT_GREEN), 0);
        }
    } else if (pickupType == PICKUP_SCROLL || pickupType == PICKUP_WAND) {
        i32 colorIndex;
        switch (static_cast<SpellId>(sprite->GetFaceDirection())) {
            case SPELL_FREEZE:
                colorIndex = IDX(TINT_WHITE);
                break;
            case SPELL_HEALTH:
                colorIndex = IDX(TINT_GREEN);
                break;
            case SPELL_RESURRECTION:
                colorIndex = IDX(TINT_ORANGE);
                break;
            case SPELL_RANDOM_TOYZ:
                colorIndex = IDX(TINT_PINK);
                break;
            case SPELL_TELEPORT:
                colorIndex = IDX(TINT_BLUE);
                break;
            case SPELL_ROLLING_BALLZ:
                colorIndex = IDX(TINT_RED);
                break;
            default:
                colorIndex = IDX(TINT_BLACK);
                break;
        }
        palette = g_gameReg->GruntPalettes()->GetShadeTable(colorIndex, 0);
        if (palette == NULL) {
            palette = g_gameReg->GruntPalettes()->GetShadeTable(IDX(TINT_GREEN), 0);
        }
    } else {
        return 1;
    }
    CWwdSpriteObject* renderSprite = m_object;
    renderSprite->SetDrawFill(SHADE_PAL_16, palette);
    return 1;
}

RVA(0x00097880, 0x102)
void CInGameIcon::FireActivation(i32 id) {
    DispatchRegisteredAct(this, id);
}

RVA(0x000979e0, 0x2ac)
void RegisterIconActions() {
    ACT_NAME_ID(idxA, "A")
    CActHandler* dslotA = &CActRegPool<CInGameIcon>::s_table[idxA];
    *dslotA = static_cast<CActHandler>(&CInGameIcon::UpdateAvailablePickup);

    ACT_NAME_ID(idxB, "B")
    CActHandler* dslotB = &CActRegPool<CInGameIcon>::s_table[idxB];
    *dslotB = static_cast<CActHandler>(&CInGameIcon::UpdateRespawn);
}

RVA(0x00097de0, 0x102)
void CToyPeek::FireActivation(i32 id) {
    DispatchRegisteredAct(this, id);
}

RVA(0x00097f40, 0x18d)
void RegisterIconState() {
    ACT_NAME_ID(idx, "A")
    CActHandler* dslot = &CActRegPool<CToyPeek>::s_table[idx];
    *dslot = static_cast<CActHandler>(&CInGameIcon::RefreshCell);
}

// @early-stop
RVA(0x00098140, 0x18e)
CToyPeek::CToyPeek(CGameObject* obj) : CUserLogic(obj, CUserLogic::INLINE_BASE), CWapX(obj) {
    m_object->m_screenY -= 0x18;
    CWwdSpriteObject* o = m_object;
    o->SetSortKey(SORTKEY_GRUNT_HUD);
    SetImageFrameByName("GAME_STATUSBAR_TABZ_STATZTAB_SMALLICONZ", m_object->GetSmarts());
    m_countdownTiming.Start(0x1388);
    SET_ANIMATION_ACT("A");
}

RVA(0x00098340, 0x71)
i32 CInGameIcon::RefreshCell() {
    CWwdSpriteObject* obj = m_object;
    i32 tileX = obj->m_screenX >> TILE_SHIFT_PX;
    i32 tileY = (obj->m_screenY + 0x18) >> TILE_SHIFT_PX;
    if (!m_driftTiming.Expired()) {
        CMapMgr* grid = g_gameReg->GetTileGrid();
        if (grid->ObjectIdAt(tileX, tileY) != 0) {
            return 0;
        }
    }
    CWwdSpriteObject* r = m_wwdObject;
    r->AddFlags(IDX(WWD_GAME_OBJECT_FLAG_PENDING_DELETE));
    return 0;
}

RVA(0x000983e0, 0x98)
i32 CToyPeek::SerializeDispatch(
    CFileMemBase* ar,
    SerialMode mode,
    LogicTypeId typeId,
    CGameObject* object
) {
    SERIALIZE_USER_LOGIC_AND_ANIMATION_STATE_OR_RETURN(ar, mode, typeId, object)

    m_countdownTiming.Serialize(ar, mode, typeId, object);
    return 1;
}

// @early-stop
RVA(0x000984b0, 0x186)
i32 CInGameIcon::UpdateAvailablePickup() {
    m_wwdObject->GetAnimationCursor().Advance(g_engineFrameDelta);
    CWwdSpriteObject* pickupObject = m_object;
    PickupType pickupType = GetPickupType();
    if (pickupType == PICKUP_TOYBOX) {
        i32 tileY = pickupObject->m_screenY >> TILE_SHIFT_PX;
        CMapMgr* tileGrid = g_gameReg->GetTileGrid();
        i32 tileX = pickupObject->m_screenX >> TILE_SHIFT_PX;
        i32 cellFlags = tileGrid->CellFlagsAt(tileX, tileY);
        if ((cellFlags & BRICKZ_BLOCKED_MASK) != 0 || (cellFlags & IDX(CELL_FLAG_SPECIAL)) != 0) {
            tileGrid->SetObjectIdAt(tileX, tileY, 0);
            SetObjectFlags(IDX(WWD_GAME_OBJECT_FLAG_PENDING_DELETE));
        }
        return 0;
    }
    if (pickupType != PICKUP_WAND && pickupType != PICKUP_SCROLL) {
        return 0;
    }
    if (pickupObject->GetFaceDirection() != 0) {
        return 0;
    }
    if (m_colorCycleTimer.Expired()) {
        CShadeTable* palette =
            g_gameReg->GruntPalettes()->GetShadeTable(GetRandomNumber() % 0x11, 0);
        CWwdSpriteObject* renderSprite = m_object;
        renderSprite->SetDrawFill(SHADE_PAL_16, palette);
        m_colorCycleTimer.Start(0xfa);
    }
    return 0;
}

RVA(0x000986b0, 0x30c)

i32 CInGameIcon::TryGivePickupToGrunt(i32 playerIndex, i32 unitIndex) {
    CWwdSpriteObject* pickupObject;
    CWwdSpriteObject* audibleObject;
    CWwdSpriteObject* expiredObject;
    CWwdSpriteObject* respawnObject;
    CWwdSpriteObject* glitterObject;
    CGrunt* recipient;
    CGrunt* warpstoneRecipient;
    PickupType pickupType;
    PickupType toyboxPickup;
    b32 ownedByRecipient;
    b32 fromOtherPlayer;
    i32 pickupParam;
    b32 pickupAccepted;
    PickupType initialPickupType;
    CGruntzMgr* gameMgr = g_gameReg;
    if (gameMgr->GetGameMode() == GAMEMODE_QUESTZ && playerIndex != g_curPlayer
        && GetPickupType() != PICKUP_TOYBOX) {
        goto fail;
    }
    initialPickupType = GetPickupType();
    pickupObject = m_object;
    if (initialPickupType == PICKUP_TOYBOX) {

        toyboxPickup = GetToyType();
        ownedByRecipient = false;
        fromOtherPlayer = true;
        if (GetPlayerIndex() == playerIndex) {
            ownedByRecipient = true;
            fromOtherPlayer = false;
        }
        pickupParam = pickupObject->m_faceDirection;
        recipient = gameMgr->GetTriggerMgr()->UnitAt(playerIndex, unitIndex);
        if (recipient == NULL || recipient->IsEntranceCommitted() == false) {
            pickupAccepted = false;
        } else if (ownedByRecipient) {
            pickupAccepted =
                recipient->BeginPickupAnimation(toyboxPickup, fromOtherPlayer, 0, pickupParam, 0);
        } else {
            pickupAccepted = recipient->ApplyPickup(toyboxPickup, fromOtherPlayer, pickupParam, 0);
        }
        gameMgr = g_gameReg;
        if (pickupAccepted == false) {
            goto fail;
        }
        if (m_pickupSoundCue != NULL) {
            audibleObject = m_object;
            if (::PtInRect(
                    &gameMgr->m_viewBounds,
                    audibleObject->m_screenX,
                    audibleObject->m_screenY
                )) {

                m_pickupSoundCue->PlayIfElapsed(g_soundVolumePercent, 0, 0, false);
                gameMgr = g_gameReg;
            }
        }
        ClearTileBit(gameMgr, m_object);
        expiredObject = m_wwdObject;
        expiredObject->m_flags |= IDX(WWD_GAME_OBJECT_FLAG_PENDING_DELETE);
        return 1;
    }

    pickupParam = pickupObject->m_faceDirection;
    pickupType = GetPickupType();
    recipient = gameMgr->GetTriggerMgr()->UnitAt(playerIndex, unitIndex);
    if (recipient == NULL || recipient->IsEntranceCommitted() == false) {
        pickupAccepted = false;
    } else {
        pickupAccepted = recipient->BeginPickupAnimation(pickupType, 0, 0, pickupParam, 1);
    }
    gameMgr = g_gameReg;
    if (pickupAccepted != false) {
        if (pickupType == PICKUP_WARPSTONE) {
            warpstoneRecipient = gameMgr->GetTriggerMgr()->UnitAt(playerIndex, unitIndex);
            if (warpstoneRecipient != NULL) {
                warpstoneRecipient->m_warpstoneAnchorIndex = m_object->m_health;
                gameMgr = g_gameReg;
            }
        }
        if (m_pickupSoundCue != NULL) {
            audibleObject = m_object;
            if (::PtInRect(
                    &gameMgr->m_viewBounds,
                    audibleObject->m_screenX,
                    audibleObject->m_screenY
                )) {

                m_pickupSoundCue->PlayIfElapsed(g_soundVolumePercent, 0, 0, false);
                gameMgr = g_gameReg;
            }
        }
        ClearTileBit(gameMgr, m_object);
        respawnObject = m_wwdObject;
        if (respawnObject->m_damage > 0) {
            respawnObject->Hide();
            SET_ANIMATION_ACT("B");
            respawnObject = m_wwdObject;
            m_driftTiming.Start(respawnObject->m_damage);
            return 1;
        }
        glitterObject = m_glitterSprite;
        if (glitterObject != NULL) {
            glitterObject->m_flags |= IDX(WWD_GAME_OBJECT_FLAG_PENDING_DELETE);
            m_glitterSprite = NULL;
        }
        expiredObject = m_wwdObject;
        expiredObject->m_flags |= IDX(WWD_GAME_OBJECT_FLAG_PENDING_DELETE);
        return 1;
    }
fail:
    return 0;
}

// @early-stop
RVA(0x00098a90, 0x18d)
i32 CInGameIcon::UpdateRespawn() {
    m_wwdObject->GetAnimationCursor().Advance(g_engineFrameDelta);
    if (m_driftTiming.Expired()) {
        CWwdSpriteObject* respawnObject = m_wwdObject;
        respawnObject->Show();
        SET_ANIMATION_ACT("A");

        CGruntzMgr* gameMgr = g_gameReg;
        CWwdSpriteObject* pickupObject = m_object;
        i32 tileX = pickupObject->m_screenX >> TILE_SHIFT_PX;
        i32 tileY = pickupObject->m_screenY >> TILE_SHIFT_PX;
        CMapMgr* tileGrid = gameMgr->GetTileGrid();
        i32 occupantObjectId = tileGrid->ObjectIdAt(tileX, tileY);
        if (occupantObjectId != 0) {

            CGameObject* occupantObject = NULL;
            if (gameMgr->World()->ChildGroup()->LookupRegisteredObject(
                    occupantObjectId,
                    occupantObject
                )
                && occupantObject != NULL) {
                occupantObject->AddFlags(IDX(WWD_GAME_OBJECT_FLAG_PENDING_DELETE));
            }
            gameMgr = g_gameReg;
            tileGrid = gameMgr->GetTileGrid();
            tileGrid->SetObjectIdAt(tileX, tileY, 0);
        }
        pickupObject = m_object;
        g_gameReg->GetTileGrid()->SetObjectIdAt(
            pickupObject->m_screenX >> TILE_SHIFT_PX,
            pickupObject->m_screenY >> TILE_SHIFT_PX,
            pickupObject->GetObjectId()
        );
    }
    return 0;
}

// @early-stop
RVA(0x00098c90, 0x382)
i32 CInGameIcon::SerializeDispatch(
    CFileMemBase* ar,
    SerialMode mode,
    LogicTypeId typeId,
    CGameObject* obj
) {

    char name[SERIAL_NAME_LEN];

    if (ar == NULL) {
        return 0;
    }
    SERIALIZE_USER_LOGIC_OR_RETURN(ar, mode, typeId, obj)

    if (!SerializeAnimationState(ar, mode, typeId, obj)) {
        return 0;
    }

    m_driftTiming.Serialize(ar, mode, typeId, obj);
    m_colorCycleTimer.Serialize(ar, mode, typeId, obj);

    switch (mode) {
        case SERIAL_SAVE: {
            memset(name, 0, sizeof(name));
            if (m_pickupSoundCue != NULL) {
                strcpy(
                    name,
                    static_cast<const char*>(m_ownerLogicRecord->GetWorld()
                                                 ->m_soundRegistry->FindCueKey(m_pickupSoundCue))
                );
            }
            ar->Write(name, SERIAL_NAME_LEN);
            g_serialCounter++;
            i32 id = 0;
            if (m_glitterSprite != NULL) {
                id = m_glitterSprite->GetObjectId();
            }
            ar->Write(&id, sizeof(id));
            break;
        }
        case SERIAL_LOAD: {
            ar->Read(name, SERIAL_NAME_LEN);

            if (strlen(name) != 0) {
                m_pickupSoundCue = m_ownerLogicRecord->GetWorld()->m_soundRegistry->FindCue(name);
            } else {
                m_pickupSoundCue = NULL;
            }
            g_serialCounter++;
            i32 id;
            ar->Read(&id, sizeof(id));
            CWwdSpriteObject* sprite = LookupSpriteObjectById(
                m_ownerLogicRecord->GetWorld()->ChildGroup()->m_registeredGameObjectsById,
                id
            );
            m_glitterSprite = sprite;
            if (sprite != NULL) {
                break;
            }

            if (id != 0) {
                return 0;
            }
            break;
        }
        case SERIAL_POSTLOAD:
            if (ApplyPickupPalette() == 0) {
                return 0;
            }
            break;
    }
    return 1;
}

// @early-stop
RVA(0x00099110, 0x215)
CInGameText::CInGameText(CGameObject* obj) : CUserLogic(obj, CUserLogic::INLINE_BASE), CWapX(obj) {
    if (g_gameReg->GetGameMode() == GAMEMODE_MULTIPLAYER) {
        SetObjectFlags(IDX(WWD_GAME_OBJECT_FLAG_PENDING_DELETE));
        return;
    }
    SET_ANIMATION_ACT("A");
    SwitchAnimationByName("GAME_CYCLE100", 0);
    SetImageSetByName("GAME_HELPBOX");
    SetObjectFlags(IDX(WWD_GAME_OBJECT_FLAG_KEEP_ACTIVE));

    InGameTextVisibility visibilityMode = static_cast<InGameTextVisibility>(m_object->GetHealth());
    if (visibilityMode == INGAME_TEXT_EASY_ONLY) {

        if (g_gameReg->GetEasyMode() == false || g_gameReg->GetGameMode() != GAMEMODE_QUESTZ) {
            SetObjectFlags(IDX(WWD_GAME_OBJECT_FLAG_PENDING_DELETE));
            return;
        }
    } else if (visibilityMode == INGAME_TEXT_NORMAL_ONLY) {
        if (g_gameReg->GetEasyMode() != false && g_gameReg->GetGameMode() == GAMEMODE_QUESTZ) {
            SetObjectFlags(IDX(WWD_GAME_OBJECT_FLAG_PENDING_DELETE));
            return;
        }
    }

    SNAP_OBJECT_TO_TILE_CENTER(m_object)
    CWwdSpriteObject* helpObject = m_object;
    helpObject->SetSortKey(SORTKEY_INGAME_INFO);
    m_lastReaderPlayerIndex = -1;
    m_lastReaderUnitIndex = -1;
}

RVA(0x00099460, 0x102)
void CInGameText::FireActivation(i32 idx) {
    DispatchRegisteredAct(this, idx);
}

RVA(0x000995c0, 0x18d)
void RegisterTextLogic() {
    ACT_NAME_ID(idx, "A")
    CActHandler* dslot = &CActRegPool<CInGameText>::s_table[idx];
    *dslot = static_cast<CActHandler>(&CInGameText::UpdateHelpBook);
}

RVA(0x000997c0, 0x1e7)
i32 CInGameText::UpdateHelpBook() {
    m_wwdObject->GetAnimationCursor().Advance(static_cast<i32>(g_engineFrameDelta));

    i32 playerIndex;
    i32 unitIndex;
    CGrunt* reader = g_gameReg->GetTriggerMgr()->FindGruntAtPoint(
        m_object->m_screenX,
        m_object->m_screenY,
        &playerIndex,
        &unitIndex,
        1
    );

    if (reader != NULL) {
        if (playerIndex != g_curPlayer) {
            return 0;
        }
        if (m_lastReaderUnitIndex != -1 && playerIndex == m_lastReaderPlayerIndex
            && unitIndex == m_lastReaderUnitIndex) {
            return 0;
        }

        if (reader->GetAnimationActName() == "K") {
            return 0;
        }

        if (!reader->BeginPickupAnimation(PICKUP_HELPBOX, 0, m_object->GetSmarts(), 0, 1)) {
            return 0;
        }

        CWwdSpriteObject* helpObject = m_object;
        i32 screenY = helpObject->m_screenY;
        i32 screenX = helpObject->m_screenX;
        CGruntzMgr* gameMgr = g_gameReg;
        if (::PtInRect(&gameMgr->m_viewBounds, screenX, screenY)) {
            PlayRegistryCueIfElapsed(gameMgr->World()->SoundRegistry(), "GAME_HELPBOOK");
        }

        m_lastReaderPlayerIndex = playerIndex;
        m_lastReaderUnitIndex = unitIndex;
        Hide();
        return 0;
    }
    m_lastReaderUnitIndex = -1;
    Show();
    return 0;
}

RVA(0x00099a30, 0xaa)
i32 CInGameText::SerializeDispatch(
    CFileMemBase* ar,
    SerialMode mode,
    LogicTypeId typeId,
    CGameObject* object
) {
    if (ar == NULL) {
        return 0;
    }
    SERIALIZE_USER_LOGIC_AND_ANIMATION_STATE_OR_RETURN(ar, mode, typeId, object)
    switch (mode) {
        case SERIAL_SAVE:
            ar->Write(&m_lastReaderPlayerIndex, sizeof(m_lastReaderPlayerIndex));
            ar->Write(&m_lastReaderUnitIndex, sizeof(m_lastReaderUnitIndex));
            break;
        case SERIAL_LOAD:
            ar->Read(&m_lastReaderPlayerIndex, sizeof(m_lastReaderPlayerIndex));
            ar->Read(&m_lastReaderUnitIndex, sizeof(m_lastReaderUnitIndex));
            break;
    }
    return 1;
}

RVA(0x00099b10, 0x36)
void CInGameIcon::SetPickupSoundCue(const char* soundKey) {
    SoundCue* found = NULL;
    if (soundKey != NULL) {
        found = NULL;
        MapLookup(g_gameReg->World()->SoundRegistry()->m_cues, soundKey, found);
    }
    m_pickupSoundCue = found;
}
