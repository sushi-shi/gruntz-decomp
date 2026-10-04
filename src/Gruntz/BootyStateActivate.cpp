#include <StdAfx.h>

#include <rva.h>

#include <Gruntz/BootyStateActivate.h>

#include <Bute/ButeMgr.h>
#include <DDrawMgr/DDrawChildGroup.h>
#include <DDrawMgr/DDrawSubMgrPages.h>
#include <DDrawMgr/DDrawSubMgrPagesInline.h>
#include <DDrawMgr/DDrawSurfaceMgr.h>
#include <DDrawMgr/DDrawSurfacePair.h>
#include <DDrawMgr/DDrawWorkerRegistry.h>
#include <DDrawMgr/DDSurface.h>
#include <Dsndmgr/SoundBuffer.h>
#include <Dsndmgr/SoundStream.h>
#include <Enums.h>
#include <Gruntz/AniAdvanceCursorInline.h>
#include <Gruntz/Attract.h>
#include <Gruntz/BankMgr.h>
#include <Gruntz/BattleStatRow.h>
#include <Gruntz/BootyCheatState.h>
#include <Gruntz/BootyMessages.h>
#include <Gruntz/BootySeqPhase.h>
#include <Gruntz/BootyStateMacros.h>
#include <Gruntz/BootyStatRow.h>
#include <Gruntz/BootyStatsInline.h>
#include <Gruntz/BootyWalkAnim.h>
#include <Gruntz/BzState.h>
#include <Gruntz/ColorTint.h>
#include <Gruntz/CoordNode.h>
#include <Gruntz/CoordPool.h>
#include <Gruntz/DirectionRingIndex.h>
#include <Gruntz/ErrorStringId.h>
#include <Gruntz/GameMode.h>
#include <Gruntz/GameRand.h>
#include <Gruntz/GameRegistry.h>
#include <Gruntz/GameRegMfcPtr.h>
#include <Gruntz/GameStateId.h>
#include <Gruntz/GameStats.h>
#include <Gruntz/GameText.h>
#include <Gruntz/GlyphStringDraw.h>
#include <Gruntz/GruntDeathType.h>
#include <Gruntz/GruntDirection.h>
#include <Gruntz/GruntDirStatics.h>
#include <Gruntz/GruntPuddle.h>
#include <Gruntz/GruntzCommandId.h>
#include <Gruntz/GruntzMgr.h>
#include <Gruntz/ImageState.h>
#include <Gruntz/LightFxMgr.h>
#include <Gruntz/MgrAutoScroll.h>
#include <Gruntz/MovieEntryId.h>
#include <Gruntz/PickupType.h>
#include <Gruntz/Play.h>
#include <Gruntz/QuestLevel.h>
#include <Gruntz/ResolveNodeInline.h>
#include <Gruntz/SortKeyLayer.h>
#include <Gruntz/SoundCue.h>
#include <Gruntz/SoundCueInline.h>
#include <Gruntz/SoundCueRegistry.h>
#include <Gruntz/SoundCueRegistryInline.h>
#include <Gruntz/SoundState.h>
#include <Gruntz/Sprite.h>
#include <Gruntz/SpriteRefTable.h>
#include <Gruntz/SpriteStateFlags.h>
#include <Gruntz/String.h>
#include <Gruntz/TypeKeyColl.h>
#include <Gruntz/UserLogic.h>
#include <Gruntz/VoiceManager.h>
#include <Gruntz/WarlordOwner.h>
#include <Gruntz/WarpLetter.h>
#include <Gruntz/WwdGameReg.h>
#include <Image/CImage.h>
#include <Ints.h>
#include <RectMacros.h>
#include <Rez/FrameClock.h>
#include <Rez/RezArchive.h>
#include <Rez/RezArchiveDir.h>
#include <Rez/RezSync.h>
#include <Utils/MapTyped.h>
#include <Utils/MillisPer.h>
#include <Wap32/ScreenGeometry.h>

#include <ddraw.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// @identity-TODO: current consumers use the first four coordinates for WarpStone pieces;
// the role of the remaining entries is not established.
DATA(0x001e8fe8)
const Coord g_bootyLetterCoords[16] = {
    {472, 101},
    {525, 98},
    {474, 146},
    {525, 144},
    {127, 170},
    {215, 262},
    {301, 345},
    {386, 427},
    {127, 170},
    {215, 262},
    {301, 345},
    {386, 427},
    {127, 170},
    {215, 262},
    {301, 345},
    {386, 427},
};

DATA(0x001e9068)
const i32 g_idleSpriteIds[4] = {420, 475, 530, 585};
DATA(0x001e9078)
const Coord g_bootyCursePos[4] = {{190, 437}, {306, 437}, {422, 437}, {538, 437}};
DATA(0x001e9098)
const Coord g_bootyTimedPowerupPos[4] = {{190, 394}, {306, 394}, {422, 394}, {538, 394}};
DATA(0x001e90b8)
const Coord g_bootyToyPos[4] = {{190, 351}, {306, 351}, {422, 351}, {538, 351}};
DATA(0x001e90d8)
const Coord g_bootyToolPos[4] = {{190, 308}, {306, 308}, {422, 308}, {538, 308}};
DATA(0x001e90f8)
const Coord g_bootyGruntPos[4] = {{190, 265}, {306, 265}, {422, 265}, {538, 265}};
DATA(0x001e9118)
const Coord g_bootyPuddlePos[4] = {{190, 222}, {306, 222}, {422, 222}, {538, 222}};
DATA(0x001e9138)
const Coord g_bootyFlagPos[4] = {{218, 180}, {334, 180}, {450, 180}, {566, 180}};
DATA(0x001e9158)
const Coord g_bootyTabPos[4] = {{218, 138}, {334, 138}, {450, 138}, {566, 138}};
DATA(0x001e9178)
const RECT s_col1Rects[4] =
    {{200, 415, 284, 465}, {316, 415, 400, 465}, {432, 415, 516, 465}, {548, 415, 632, 465}};
DATA(0x001e91b8)
const RECT s_col2Rects[4] =
    {{200, 372, 284, 422}, {316, 372, 400, 422}, {432, 372, 516, 422}, {548, 372, 632, 422}};
DATA(0x001e91f8)
const RECT s_col3Rects[4] =
    {{200, 329, 284, 379}, {316, 329, 400, 379}, {432, 329, 516, 379}, {548, 329, 632, 379}};
DATA(0x001e9238)
const RECT s_col4Rects[4] =
    {{200, 286, 284, 336}, {316, 286, 400, 336}, {432, 286, 516, 336}, {548, 286, 632, 336}};
DATA(0x001e9278)
const RECT s_col5Rects[4] =
    {{200, 243, 284, 293}, {316, 243, 400, 293}, {432, 243, 516, 293}, {548, 243, 632, 293}};
DATA(0x001e92b8)
const RECT s_col6Rects[4] =
    {{200, 200, 284, 250}, {316, 200, 400, 250}, {432, 200, 516, 250}, {548, 200, 632, 250}};
DATA(0x001e92f8)
const RECT s_colorRects[4] =
    {{50, 87, 390, 115}, {166, 87, 506, 115}, {282, 87, 622, 115}, {398, 87, 738, 115}};
DATA(0x001e9338)
const RECT s_labelRects[7] = {
    {45, 155, 175, 215},
    {50, 198, 180, 258},
    {34, 241, 172, 301},
    {55, 284, 172, 344},
    {66, 327, 174, 387},
    {0, 370, 172, 430},
    {38, 413, 172, 473}
};

DATA(0x001e93a8)
const char g_secretChars[] = "WARP";
DATA(0x001e93b0)
const float g_secretRatioScale = 100.0f;
DATA(0x001e93b4)
static const float s_glitterPhaseBias = -225.0f;
DATA(0x001e93b8)
static const double s_degToRad = 0.017453292;
DATA(0x001e93c0)
static const double s_glitterShrinkRate = 0.002;
DATA(0x001e93c8)
static const double s_glitterStartRadius = 350.0;

DATA(0x0020b838)
RECT g_bootyStatLabelRects[8] = {
    {105, 106, 190, 155},
    {26, 149, 182, 199},
    {72, 192, 187, 240},
    {87, 238, 185, 288},
    {94, 281, 185, 332},
    {31, 324, 182, 374},
    {89, 360, 181, 411},
    {59, 400, 180, 449}
};

DATA(0x0020b8f8)
RECT g_bootyStatValueRects[8] = {
    {245, 92, 417, 162},
    {245, 135, 417, 205},
    {245, 180, 417, 250},
    {245, 227, 417, 297},
    {245, 266, 417, 340},
    {245, 310, 417, 380},
    {245, 351, 417, 421},
    {245, 392, 417, 462}
};

RVA_DYNINIT(0x00018720, 0xa, g_bootyStatLabels)
RVA_DYNINIT(0x00018740, 0x79, g_bootyStatLabels)
RVA_DYNINIT(0x000187e0, 0xe, g_bootyStatLabels)
RVA_DYNINIT(0x00018800, 0x14, g_bootyStatLabels)
DATA(0x00229ef8)
CString g_bootyStatLabels[8] = {
    "Time:",
    "Survivorz:",
    "Deathz:",
    "Toolz:",
    "Toyz:",
    "Powerupz:",
    "Coinz:",
    "Secretz:",
};

DATA(0x00229f30)
SecretMsgRow g_secretMsgRows[25];

DATA(0x0022af10)
b32 g_bootyCheatBuilt = false;

RVA(0x00018830, 0x380)
i32 CBootyState::LoadGameAssetNamespaces(CGruntzMgr* mgr, i32 areaArg, i32 prevStateId) {

    if (!CState::LoadGameAssetNamespaces(mgr, areaArg, prevStateId)) {
        return 0;
    }

    if (g_bootyCheatBuilt == false) {
        CString bootyCheatz("BootyCheatz");
        CString empty("");
        CString grp;
        CString text;
        CString desc;
        i32 i = 0;

        // byte-evidenced: retail compares the row cursor as a signed integer.
        i32 last = reinterpret_cast<i32>(g_secretMsgRows[24].m_strB + sizeof(SecretMsgRow));
        char* p = g_secretMsgRows[0].m_strB;
        do {
            grp.Format("A%dC%d", i / 3 + 1, i % 3 + 1);
            i32 id = g_buteMgr.GetInt(bootyCheatz, grp, 1);
            grp.Format("Cheat%i", id);
            text = *g_buteMgr.GetString(grp, "Text", &empty);
            desc = *g_buteMgr.GetString(grp, "Desc", &empty);
            strcpy(p - 0x20, text);
            strcpy(p, desc);
            i++;
            p += 0xa0;
        } while (reinterpret_cast<i32>(p) < last); // byte-evidenced: signed cursor compare
        g_bootyCheatBuilt = true;
    }

    m_mgr->EnsureStandardVideoMode(false);

    m_stateResources = m_resourceArchive->GetDirFromPath("STATEZ_BOOTY");
    if (!m_stateResources) {
        return 0;
    }
    m_gameResources = m_resourceArchive->GetDirFromPath("GAME");
    if (!m_gameResources) {
        return 0;
    }
    m_gruntResources = m_resourceArchive->GetDirFromPath("GRUNTZ");
    if (!m_gruntResources) {
        return 0;
    }

    m_world->ChildGroup()->ClearChildren();

    {
        CRezDir* soundz = StateResources()->GetDir("SOUNDZ");
        if (!soundz) {
            return 0;
        }
        m_world->SoundRegistry()->LoadFromTree(static_cast<CRezDir*>(soundz), "BOOTY", "_");

        CRezDir* wand = m_gruntResources->GetDirFromPath("SOUNDZ_WANDGRUNT");
        if (!wand) {
            return 0;
        }
        m_world->SoundRegistry()
            ->LoadFromTree(static_cast<CRezDir*>(wand), "GRUNTZ_WANDGRUNT", "_");

        CRezDir* imagez = StateResources()->GetDir("IMAGEZ");
        if (!imagez) {
            return 0;
        }
        m_world->GetImageRegistry()->InstallTree(imagez, "BOOTY", "_");
    }

    while (ShowCursor(false) >= 0) {
    }

    m_mgr->GetGameWindow()->DiscardMessages(WM_KEYDOWN, 0x40);

    m_secretHudHandled = false;

    if (!BuildWarpStoneGlitterAnimation()) {
        return 0;
    }
    if (!BuildGruntSprintAnimation()) {
        return 0;
    }
    if (!BuildStatRevealSprites()) {
        return 0;
    }
    if (!BuildBootyWalkingGruntz()) {
        return 0;
    }
    if (!BuildBootyPerfectAnimation()) {
        return 0;
    }

    m_frameTiming.Start(0x21);
    return 1;
}

// @early-stop
RVA(0x00018c90, 0x72)
void CBootyState::ReleaseResources() {
    SoundStream* r = m_world->SoundRegistry()->m_soundStream;
    if (r) {
        r->StopAllStreams();
    }
    m_world->SoundRegistry()->RemoveWithPrefix("BOOTY", "_");
    m_world->SoundRegistry()->RemoveWithPrefix("GRUNTZ_WANDGRUNT", "_");
    m_world->GetImageRegistry()->RemoveWithPrefix("BOOTY", "_");
    m_world->GetImageRegistry()->RemoveWithPrefix("GRUNTZ_GOKARTGRUNT", "_");
    CState::ReleaseResources();
}

RVA(0x00018d30, 0xcd)
i32 CBootyState::EnterState(GameStateId previousState) {
    while (ShowCursor(false) >= 0)
        ;
    if (!LoadTitlePage("bg", 0, 0, 0, 0, true)) {
        return 0;
    }
    m_world->GetDrawTarget()->TransExit();
    RetireScene(0x50, 0x3e8, 0, true);

    CGruntzMgr* reg = g_gameReg;
    CDDrawSurfaceMgr* world = reg->World();
    i32 token = reg->GetSoundVolume();
    SoundCueRegistry* set = world->SoundRegistry();
    if (set->IsSilent() == false) {
        SoundCue* found = set->FindCue("BOOTY_LOOP");
        if (found != NULL) {
            PlaySoundCueIfElapsed(found, token, 0, 0, true);
        }
    }
    return 1;
}

RVA(0x00018e40, 0x81)
i32 CBootyState::LeaveState(GameStateId nextState) {
    SoundCue* found = m_world->SoundRegistry()->FindCue("BOOTY_LOOP");
    if (found && found->IsPlaying()) {
        found->GetSound()->RampVolumeTo(0, 0x1f4, true);
        while (found->IsPlaying()) {
            m_world->SoundRegistry()->TickVolumeRamps();
        }
    }
    return 1;
}

RVA(0x00018f00, 0x4fb)
i32 CBootyState::ShowSecretBonusMessage() {
    if (m_secretBannerOnce != false && (g_gameReg->GetGameStats())->IsCampaignPerfect()) {
        CString s;
        if (!LoadTitlePage("multi", 0, 0, 0, 0, true)) {
            return 0;
        }
        RECT rA, rB, rTitle;
        SetRect(&rA, 0, -15, SCREEN_W_PX, 0x1d1);
        SetRect(&rB, 0, 0x19, SCREEN_W_PX, 0x1f9);
        SetRect(&rTitle, 0, 0x38, SCREEN_W_PX, 0x78);
        s.Format("The Secret of Secretz:");
        DrawTextToOverlaySurface(m_world, &s, &rTitle, 0x82, 1, 0xff, 0xff, 0, 1);

        CString s2(g_secretMsgRows[24].m_strA);
        CString s3(g_secretMsgRows[24].m_strB);
        for (i32 k = 0; k < s2.GetLength(); k++) {
            s2.SetAt(k, static_cast<char>(((static_cast<const char*>(s2))[k] - 0x3d)));
        }
        DrawTextToOverlaySurface(m_world, &s2, &rA, 0x78, 1, 0xff, 0xff, 0, 1);
        DrawTextToOverlaySurface(m_world, &s3, &rB, 0x6e, 1, 0xff, 0xff, 0, 1);
        return 1;
    } else {
        i32 count = static_cast<i32>(
            ((g_gameReg->GetGameStats())->CurrentAreaCoinRatio() * g_secretRatioScale)
        );
        i32 rowBase = (g_gameReg->GetGameStats()->GetLevelNumber() - 1) / 4;
        SecretBonusTier category =
            (count >= 0x64) ? SECRET_BONUS_TIER_THREE
                            : ((count >= 0x32) ? SECRET_BONUS_TIER_TWO : SECRET_BONUS_TIER_ONE);

        if (!LoadTitlePage("multi", 0, 0, 0, 0, true)) {
            return 0;
        }
        CString title;
        RECT rTitle;
        SetRect(&rTitle, 0, 0x38, SCREEN_W_PX, 0x78);
        if (category == SECRET_BONUS_TIER_ONE) {
            title.Format("Secret Bonus Acquired:");
        } else {
            title.Format("Secret Bonus Acquired:");
        }
        DrawTextToOverlaySurface(m_world, &title, &rTitle, 0x82, 1, 0xff, 0xff, 0, 1);

        for (i32 j = 0; j < IDX(category); j++) {
            RECT rA, rB;
            if (category == SECRET_BONUS_TIER_ONE) {
                SetRect(&rA, 0, -15, SCREEN_W_PX, 0x1d1);
                SetRect(&rB, 0, 0x19, SCREEN_W_PX, 0x1f9);
            } else if (category == SECRET_BONUS_TIER_TWO) {
                if (j == 0) {
                    SetRect(&rA, 0, -20, SCREEN_W_PX, 0x1cc);
                    SetRect(&rB, 0, 0x14, SCREEN_W_PX, 0x1f4);
                } else {
                    SetRect(&rA, 0, 0x46, SCREEN_W_PX, 0x226);
                    SetRect(&rB, 0, 0x6e, SCREEN_W_PX, 0x24e);
                }
            } else {
                if (j == 0) {
                    SetRect(&rA, 0, -60, SCREEN_W_PX, 0x1a4);
                    SetRect(&rB, 0, -20, SCREEN_W_PX, 0x1cc);
                } else if (j == 1) {
                    SetRect(&rA, 0, 0x1e, SCREEN_W_PX, 0x1fe);
                    SetRect(&rB, 0, 0x46, SCREEN_W_PX, 0x226);
                } else {
                    SetRect(&rA, 0, 0x78, SCREEN_W_PX, 0x24e);
                    SetRect(&rB, 0, 0xa0, SCREEN_W_PX, 0x276);
                }
            }
            i32 idx = rowBase * 3 + j;
            CString s5(g_secretMsgRows[idx].m_strA);
            CString s6(g_secretMsgRows[idx].m_strB);
            for (i32 k = 0; k < s5.GetLength(); k++) {
                s5.SetAt(k, static_cast<char>(((static_cast<const char*>(s5))[k] - 0x3d)));
            }
            DrawTextToOverlaySurface(m_world, &s5, &rA, 0x78, 1, 0xff, 0xff, 0, 1);
            DrawTextToOverlaySurface(m_world, &s6, &rB, 0x6e, 1, 0xff, 0xff, 0, 1);
        }
        return 1;
    }
}

RVA(0x00019540, 0x12a)
i32 CBootyState::BuildWarpStoneGlitterAnimation() {
    CWwdSpriteObject** pieces = m_warpStonePieceSprites;
    m_warpStonePieceIndex = (g_gameReg->GetGameStats()->GetLevelNumber() - 1) % 4;
    m_pieceOrbitRadius = 0xc8;
    m_pieceOrbitAngle = 0;
    m_pieceOrbitX = 0;
    m_pieceOrbitY = 0;
    for (i32 i = 0; i < 4; i++) {
        CWwdSpriteObject* piece = g_gameReg->World()->ChildGroup()->CreateSprite(
            0,
            0,
            0,
            (i != m_warpStonePieceIndex) ? 1 : 3,
            "DoNothing",
            WWD_GAME_OBJECT_FLAGS_SKIP_COLLISION_KEEP_ACTIVE
        );
        pieces[i] = piece;
        if (piece == NULL) {
            return 0;
        }
        piece->SetImageFrameByName("GAME_STATUSBAR_TABZ_GAMETAB_WARPSTONE", i + 2);
        pieces[i]->Hide();
    }
    for (i32 k = 0; k <= m_warpStonePieceIndex; k++) {
        pieces[k]->Show();
    }
    CWwdSpriteObject* glitter = CreateSimpleAnimationSprite(4);
    m_warpStoneGlitterSprite = glitter;
    if (glitter == NULL) {
        return 0;
    }
    glitter->SetImageSetByName("GAME_GLITTERGOLD");
    m_warpStoneGlitterSprite->SetAnimationByName("GAME_CYCLE100", 0);
    return 1;
}

// @early-stop
RVA(0x000196c0, 0x1d3)
i32 CBootyState::UpdateWarpStoneGlitterAnimation() {
    if (m_skipAnimations) {
        for (i32 i = 0; i <= m_warpStonePieceIndex; i++) {
            CWwdSpriteObject* piece = m_warpStonePieceSprites[i];
            piece->m_screenX = g_bootyLetterCoords[i].m_x;
            piece = m_warpStonePieceSprites[i];
            piece->m_screenY = g_bootyLetterCoords[i].m_y;
            piece = m_warpStonePieceSprites[i];
            piece->SetSortKey(1);
        }
        SET_SCREEN_POS(
            m_warpStoneGlitterSprite,
            g_bootyLetterCoords[m_warpStonePieceIndex].m_x,
            g_bootyLetterCoords[m_warpStonePieceIndex].m_y
        );
        return 1;
    }

    i32 angleDegrees = m_pieceOrbitAngle;
    i32 pieceIndex = m_warpStonePieceIndex;
    double radius = static_cast<float>(m_pieceOrbitRadius);
    double angleRadians = (static_cast<float>(angleDegrees) - s_glitterPhaseBias) * s_degToRad;
    m_pieceOrbitX =
        static_cast<i32>((sin(angleRadians) * radius + g_bootyLetterCoords[pieceIndex].m_x));
    m_pieceOrbitY =
        static_cast<i32>((cos(angleRadians) * radius + g_bootyLetterCoords[pieceIndex].m_y));
    m_pieceOrbitAngle = angleDegrees + 5;
    double shrinkFraction = static_cast<float>(angleDegrees + 5) * s_glitterShrinkRate;
    m_pieceOrbitRadius =
        static_cast<i32>((s_glitterStartRadius - shrinkFraction * s_glitterStartRadius));

    i32 i = 0;
    if (pieceIndex > 0) {
        do {
            CWwdSpriteObject* piece = m_warpStonePieceSprites[i];
            piece->m_screenX = g_bootyLetterCoords[i].m_x;
            piece = m_warpStonePieceSprites[i];
            piece->m_screenY = g_bootyLetterCoords[i].m_y;
            i++;
        } while (i < m_warpStonePieceIndex);
    }

    SET_SCREEN_POS(m_warpStoneGlitterSprite, m_pieceOrbitX, m_pieceOrbitY);
    SET_SCREEN_POS(m_warpStonePieceSprites[i], m_pieceOrbitX, m_pieceOrbitY);

    UpdateGruntSprintAnimation();

    if (m_pieceOrbitRadius == 0) {
        CWwdSpriteObject* piece = m_warpStonePieceSprites[i];
        piece->SetSortKey(1);
        return 1;
    }
    return 0;
}

// @early-stop
RVA(0x00019920, 0x1f0)
i32 CBootyState::BuildGruntSprintAnimation() {
    CShadeTable* h = g_gameReg->GruntPalettes()->GetShadeTable(0, 0);
    if (!h) {
        return 0;
    }

    for (i32 i = 0; i < 8; i++) {
        m_sprintSprites[i] = CreateSimpleAnimationSprite(2);
        if (m_sprintSprites[i] == NULL) {
            return 0;
        }

        CString dir;
        switch (static_cast<GruntDirection>(i + 1)) {
            case DIR_NORTH:
                dir = "NORTH";
                break;
            case DIR_NORTHEAST:
                dir = "NORTHEAST";
                break;
            case DIR_EAST:
                dir = "EAST";
                break;
            case DIR_SOUTHEAST:
                dir = "SOUTHEAST";
                break;
            case DIR_SOUTH:
                dir = "SOUTH";
                break;
            case DIR_SOUTHWEST:
                dir = "SOUTHWEST";
                break;
            case DIR_WEST:
                dir = "WEST";
                break;
            case DIR_NORTHWEST:
                dir = "NORTHWEST";
                break;
        }

        m_sprintSprites[i]->SetImageSetByName("GRUNTZ_NORMALGRUNT_" + dir + "_WALK");
        m_sprintSprites[i]->SetAnimationByName("GAME_GRUNTSPRINT", 0);
        {
            CWwdSpriteObject* o = m_sprintSprites[i];
            o->SetDrawFill(SHADE_PAL_16, h);
        }

        i32 outX, outY;
        PickGruntSprintStartPosition(static_cast<GruntDirection>(i + 1), &outX, &outY);
        SET_SCREEN_POS(m_sprintSprites[i], outX, outY);
    }
    return 1;
}

// @early-stop
RVA(0x00019b90, 0xf8)
void CBootyState::UpdateGruntSprintAnimation() {
    if (m_skipAnimations) {
        CWwdSpriteObject** q = m_sprintSprites;
        i32 n = 8;
        do {
            CGameObject* e = *q;
            q++;
            e->Hide();
        } while (--n);
        return;
    }
    i32 i = 0;
    CWwdSpriteObject** p = m_sprintSprites;
    for (; i < 8; i++, p++) {
        CGameObject* e = *p;
        i32 x = e->m_screenX;
        i32 y = e->m_screenY;
        if (x < 0 || x > SCREEN_W_PX || y < 0 || y > SCREEN_H_PX) {
            e->Hide();
        } else {
            switch (static_cast<DirectionRingIndex>(i)) {
                case DIRECTION_RING_NORTH:
                    y -= 4;
                    break;
                case DIRECTION_RING_NORTHEAST:
                    y -= 4;
                    x += 4;
                    break;
                case DIRECTION_RING_EAST:
                    x += 4;
                    break;
                case DIRECTION_RING_SOUTHEAST:
                    y += 4;
                    x += 4;
                    break;
                case DIRECTION_RING_SOUTH:
                    y += 4;
                    break;
                case DIRECTION_RING_SOUTHWEST:
                    y += 4;
                    x -= 4;
                    break;
                case DIRECTION_RING_WEST:
                    x -= 4;
                    break;
                case DIRECTION_RING_NORTHWEST:
                    y -= 4;
                    x -= 4;
                    break;
            }
            SET_SCREEN_POS((*p), x, y);
        }
    }
}

DATA(0x0020b8b8)
Coord g_bootyStatIconPositions[8] = {
    {0xea, 0x80},
    {0xec, 0xae},
    {0xeb, 0xe3},
    {0xe9, 0x10b},
    {0xe9, 0x12f},
    {0xe7, 0x159},
    {0xe8, 0x17c},
    {0xe9, 0x1a8},
};

// @early-stop
RVA(0x00019cd0, 0x200)
void CBootyState::PickGruntSprintStartPosition(GruntDirection direction, i32* outX, i32* outY) {
    if (!outX || !outY) {
        return;
    }
    i32 flip;
    switch (direction) {
        case DIR_NORTH:
            *outX = g_gameReg->Rand() % 0x281;
            *outY = SCREEN_H_PX;
            return;
        case DIR_SOUTH:
            *outX = g_gameReg->Rand() % 0x281;
            *outY = 0;
            return;
        case DIR_EAST:
            *outX = 0;
            *outY = g_gameReg->Rand() % 0x1e1;
            return;
        case DIR_WEST:
            *outX = SCREEN_W_PX;
            *outY = g_gameReg->Rand() % 0x1e1;
            return;
        case DIR_NORTHEAST:
            flip = g_gameReg->Rand() % 2;
            if (flip) {
                *outX = 0;
                *outY = g_gameReg->Rand() % 0xf1 + SCREEN_HALF_H_PX;
                return;
            }
            *outX = g_gameReg->Rand() % 0x141;
            *outY = SCREEN_H_PX;
            return;
        case DIR_NORTHWEST:
            flip = g_gameReg->Rand() % 2;
            if (flip) {
                *outX = SCREEN_W_PX;
                *outY = g_gameReg->Rand() % 0xf1 + SCREEN_HALF_H_PX;
                return;
            }
            *outX = g_gameReg->Rand() % 0x141 + SCREEN_HALF_W_PX;
            *outY = SCREEN_H_PX;
            return;
        case DIR_SOUTHEAST:
            flip = g_gameReg->Rand() % 2;
            if (flip) {
                *outX = g_gameReg->RandRange(0, SCREEN_HALF_W_PX);
                *outY = 0;
                return;
            }
            *outX = 0;
            *outY = g_gameReg->RandRange(0, SCREEN_HALF_H_PX);
            return;
        case DIR_SOUTHWEST:
            if (g_gameReg->RandRange(0, 1)) {
                *outX = g_gameReg->RandRange(0, SCREEN_HALF_W_PX) + SCREEN_HALF_W_PX;
                *outY = 0;
                return;
            }
            *outX = SCREEN_W_PX;
            *outY = g_gameReg->RandRange(0, SCREEN_HALF_H_PX);
            return;
    }
}

RVA(0x00019f50, 0xb2)
i32 CGruntzMgr::RandRange(i32 lo, i32 hi) {
    if ((hi - lo + 1) == 0) {
        if (GetRandomNumber() & 1) {
            return lo;
        } else {
            return hi;
        }
    }
    return (GetRandomNumber() % (hi - lo + 1)) + lo;
}

// @early-stop
RVA(0x0001a040, 0x55e)
i32 CBootyState::BuildStatRevealSprites() {
    CShadeTable* handleA = g_gameReg->GruntPalettes()->GetShadeTable(0, 0);
    if (handleA == NULL) {
        return 0;
    }
    CShadeTable* handleB = g_gameReg->GruntPalettes()->GetShadeTable(0, 1);

    CRezDir* img = m_gruntResources->GetDirFromPath("IMAGEZ_GOKARTGRUNT");
    if (img == NULL) {
        return 0;
    }
    m_world->GetImageRegistry()->InstallTree(img, "GRUNTZ_GOKARTGRUNT", "_");

    CDDrawChildGroup* f = g_gameReg->World()->ChildGroup();

    CWwdSpriteObject* sw = f->CreateSprite(
        0,
        0,
        0,
        0,
        "SimpleAnimation",
        WWD_GAME_OBJECT_FLAGS_SKIP_COLLISION_KEEP_ACTIVE
    );
    m_statIcons[0] = sw;
    if (sw == NULL) {
        return 0;
    }
    sw->SetImageSetByName("GAME_INGAMEICONZ_POWERUPZ_STOPWATCH");
    m_statIcons[0]->SetAnimationByName("GAME_CYCLE100", 0);
    m_statIcons[0]->Hide();

    CWwdSpriteObject* wh = CreateSimpleAnimationSprite(0);
    m_statIcons[7] = wh;
    if (wh == NULL) {
        return 0;
    }
    CLightFxMgr* lightFxMgr = g_gameReg->GetLightFxMgr();
    CShadeTable* tint = lightFxMgr->GetShadeTable(g_buteMgr.GetInt("Wormhole", "SecretColor", 1));
    m_statIcons[7]->SetImageSetByName("GAME_WORMHOLE");
    m_statIcons[7]->SetAnimationByName("GAME_TELEPORTER", 0);
    m_statIcons[7]->Hide();
    CWwdSpriteObject* icon7 = m_statIcons[7];
    icon7->SetDrawFill(SHADE_DST_BY_SRC_16, tint);

    CWwdSpriteObject* ex = CreateSimpleAnimationSprite(0);
    m_statIcons[1] = ex;
    if (ex == NULL) {
        return 0;
    }
    ex->SetImageSetByName("GRUNTZ_EXITZ");
    m_statIcons[1]->SetAnimationByName("GAME_GRUNTFLEX", 0);
    CWwdSpriteObject* icon1 = m_statIcons[1];
    icon1->SetDrawFill(SHADE_PAL_16, handleA);
    m_statIcons[1]->Hide();

    CWwdSpriteObject* dt = CreateSimpleAnimationSprite(0);
    m_statIcons[2] = dt;
    if (dt == NULL) {
        return 0;
    }
    dt->SetImageSetByName("GRUNTZ_NORMALGRUNT_DEATH");
    m_statIcons[2]->SetAnimationByName("GAME_GRUNTTWITCH", 0);
    CWwdSpriteObject* icon2 = m_statIcons[2];
    icon2->SetDrawFill(SHADE_PAL_16, handleA);
    m_statIcons[2]->Hide();

    CWwdSpriteObject* gl = CreateSimpleAnimationSprite(0);
    m_statIcons[3] = gl;
    if (gl == NULL) {
        return 0;
    }
    gl->SetImageSetByName("GAME_INGAMEICONZ_TOOLZ_GAUNTLETZ");
    m_statIcons[3]->SetAnimationByName("GAME_CYCLE100", 0);
    CWwdSpriteObject* icon3 = m_statIcons[3];
    icon3->SetDrawFill(SHADE_PAL_16, handleA);
    m_statIcons[3]->Hide();

    CWwdSpriteObject* bb = CreateSimpleAnimationSprite(0);
    m_statIcons[4] = bb;
    if (bb == NULL) {
        return 0;
    }
    bb->SetImageSetByName("GAME_INGAMEICONZ_TOYZ_BEACHBALLZ");
    m_statIcons[4]->SetAnimationByName("GAME_CYCLE100", 0);
    CWwdSpriteObject* beachBallIcon = m_statIcons[4];
    beachBallIcon->SetDrawFill(SHADE_PAL_16, handleA);
    m_statIcons[4]->Hide();

    CWwdSpriteObject* rz = CreateSimpleAnimationSprite(0);
    m_statIcons[5] = rz;
    if (rz == NULL) {
        return 0;
    }
    rz->SetImageSetByName("GAME_INGAMEICONZ_POWERUPZ_ROIDZ");
    m_statIcons[5]->SetAnimationByName("GAME_CYCLE100", 0);
    CWwdSpriteObject* icon5 = m_statIcons[5];
    icon5->SetDrawFill(SHADE_PAL_16, handleA);
    m_statIcons[5]->Hide();

    CWwdSpriteObject* cn = CreateSimpleAnimationSprite(0);
    m_statIcons[6] = cn;
    if (cn == NULL) {
        return 0;
    }
    cn->SetImageSetByName("GAME_INGAMEICONZ_POWERUPZ_COIN");
    m_statIcons[6]->SetAnimationByName("GAME_CYCLE100", 0);
    CWwdSpriteObject* icon6 = m_statIcons[6];
    icon6->SetDrawFill(SHADE_PAL_16, handleA);
    m_statIcons[6]->Hide();

    for (i32 i = 0; i < 8; i++) {
        CWwdSpriteObject* b = CreateSimpleAnimationSprite(2);
        m_bombSprites[i] = b;
        if (b == NULL) {
            return 0;
        }
        b->SetImageSetByName("GRUNTZ_BOMBGRUNT_WEST_ITEM");
        m_bombSprites[i]->SetAnimationByName("GAME_GRUNTBOMBSPRINT", 0);
        CWwdSpriteObject* bp = m_bombSprites[i];
        bp->SetDrawFill(SHADE_PAL_16, handleA);
        SET_SCREEN_POS(
            m_bombSprites[i],
            0x2c6,
            (g_bootyStatValueRects[i].top + g_bootyStatValueRects[i].bottom) / 2
        );
        m_bombSprites[i]->Hide();

        CWwdSpriteObject* e = CreateSimpleAnimationSprite(2);
        m_explosionSprites[i] = e;
        if (e == NULL) {
            return 0;
        }
        e->SetImageSetByName("GAME_EXPLOSION");
        m_explosionSprites[i]->Hide();

        CWwdSpriteObject* g = CreateSimpleAnimationSprite(2);
        m_goKartSprites[i] = g;
        if (g == NULL) {
            return 0;
        }
        g->SetImageSetByName("GRUNTZ_GOKARTGRUNT_EAST");
        m_goKartSprites[i]->SetAnimationByName("GAME_CYCLE100", 0);
        CWwdSpriteObject* gp = m_goKartSprites[i];
        gp->SetDrawFill(SHADE_PAL_16, handleB);
        SET_SCREEN_POS(
            m_goKartSprites[i],
            -70,
            (g_bootyStatValueRects[i].top + g_bootyStatValueRects[i].bottom) / 2
        );
        m_goKartSprites[i]->Hide();
    }
    return 1;
}

RVA(0x0001a700, 0x6b6)
i32 CBootyState::UpdateStatRevealAnimation() {
    if (m_skipAnimations != false) {

        if (m_statRowIndex == BOOTY_EXPLOSION_COUNT) {

            for (i32 i = 0; i < 8; i++) {
                CWwdSpriteObject* e = m_explosionSprites[i];
                if (e->m_animationCursor.IsComplete()) {
                    e->Hide();
                }
            }
            return 1;
        }

        i32 shown = 0;
        for (i32 i = 0; i < 8; i++) {
            m_bombSprites[i]->Hide();
            m_goKartSprites[i]->Hide();
            m_statIcons[i]->Show();
            SET_SCREEN_POS(
                m_statIcons[i],
                g_bootyStatIconPositions[i].m_x,
                g_bootyStatIconPositions[i].m_y
            );
            CRect box(g_bootyStatLabelRects[i]);
            CString text = g_bootyStatLabels[i];
            m_statLabelVisible[i] = 1;
            DrawTextToOverlaySurface(m_world, &text, &box, 0x78, 1, 0xff, 0xff, 0, 1);
            box.CopyRect(&g_bootyStatValueRects[i]);
            this->FormatStatValue(&text, static_cast<BootyStatRow>(i));
            m_readyFlags[i] = 1;
            DrawTextToOverlaySurface(m_world, &text, &box, 0x78, 1, 0xff, 0xff, 0, 1);
            if (i >= m_statRowIndex
                && (i != m_statRowIndex
                    || m_explosionSprites[i]->m_animationCursor.GetAnimation() == NULL)) {
                m_explosionSprites[i]->Show();
                m_explosionSprites[i]->SetAnimationByName("GAME_EXPLOSION1", 0);
                m_explosionSprites[i]->m_screenX =
                    (g_bootyStatValueRects[i].right + g_bootyStatValueRects[i].left) / 2;
                m_explosionSprites[i]->m_screenY =
                    (g_bootyStatValueRects[i].bottom + g_bootyStatValueRects[i].top) / 2 - 0x10;
                if (shown == 0) {

                    SoundCueRegistry* registry = g_gameReg->World()->SoundRegistry();
                    registry->PlayCue("GAME_EXPLOSION1");
                    shown = 1;
                }
            }
        }
        m_statRowIndex = 8;
        return 1;
    }

    if (m_statRowIndex < 8) {
        if (m_statRowIndex == 0
            && (m_bombSprites[0]->IsHidden() || m_goKartSprites[0]->IsHidden())) {
            m_bombSprites[0]->Show();
            m_goKartSprites[0]->Show();
        }
        m_bombSprites[m_statRowIndex]->m_screenX -= 10;
        i32 gx = m_goKartSprites[m_statRowIndex]->m_screenX + 10;
        m_goKartSprites[m_statRowIndex]->m_screenX = gx;
        i32 s = m_statRowIndex;

        if (m_statLabelVisible[s] == 0
            && gx >= (g_bootyStatLabelRects[s].right + g_bootyStatLabelRects[s].left) / 2) {
            m_statLabelVisible[s] = 1;
            CRect box(g_bootyStatLabelRects[m_statRowIndex]);
            CString text = g_bootyStatLabels[m_statRowIndex];
            m_statLabelVisible[m_statRowIndex] = 1;
            DrawTextToOverlaySurface(m_world, &text, &box, 0x78, 1, 0xff, 0xff, 0, 1);
        }
        s = m_statRowIndex;
        if (m_readyFlags[s] == 0 && gx >= g_bootyStatIconPositions[s].m_x) {
            m_readyFlags[s] = 1;
            m_statIcons[m_statRowIndex]->Show();
            SET_SCREEN_POS(
                m_statIcons[m_statRowIndex],
                g_bootyStatIconPositions[m_statRowIndex].m_x,
                g_bootyStatIconPositions[m_statRowIndex].m_y
            );
        }
    }

    for (i32 j = 0; j < m_statRowIndex; j++) {
        CWwdSpriteObject* e = m_explosionSprites[j];
        if (e->m_animationCursor.IsComplete()) {
            e->Hide();
        }
    }

    for (i32 i = m_statRowIndex; i < 8; i++) {
        if (m_goKartSprites[i]->m_screenX >= m_bombSprites[i]->m_screenX) {
            CString text;
            CRect box(g_bootyStatValueRects[i]);
            this->FormatStatValue(&text, static_cast<BootyStatRow>(i));
            m_readyFlags[i] = 1;
            DrawTextToOverlaySurface(m_world, &text, &box, 0x78, 1, 0xff, 0xff, 0, 1);
            m_explosionSprites[i]->Show();
            m_explosionSprites[i]->SetAnimationByName("GAME_EXPLOSION1", 0);
            m_explosionSprites[i]->m_screenX =
                (g_bootyStatValueRects[i].left + g_bootyStatValueRects[i].right) / 2;
            m_explosionSprites[i]->m_screenY =
                (g_bootyStatValueRects[i].top + g_bootyStatValueRects[i].bottom) / 2 - 0x10;
            m_bombSprites[i]->Hide();
            m_goKartSprites[i]->Hide();
            m_statRowIndex++;
            PlayRegistryCueIfElapsed(g_gameReg->World()->SoundRegistry(), "GAME_EXPLOSION1");
            if (m_statRowIndex >= 8) {
                return 1;
            }
            m_bombSprites[m_statRowIndex]->Show();
            m_goKartSprites[m_statRowIndex]->Show();
        }
    }
    return 0;
}

RVA(0x0001af70, 0x3e0)
void CBootyState::FormatStatValue(CString* buf, BootyStatRow sel) {
    switch (sel) {
        case BOOTYSTAT_TIME: {
            u32 secs = static_cast<u32>(
                STAT(SumElapsedTimeForCurrentArea, m_elapsedTimeMs) / MILLIS_PER_SECOND
            );
            buf->Format("%d:%2.2d", secs / 60, secs % 60);
            return;
        }
        case BOOTYSTAT_GRUNTZ_EXITED:
            buf->Format("%d", STAT(SumGruntzExitedForCurrentArea, m_gruntzExited));
            return;
        case BOOTYSTAT_GRUNTZ_LOST:
            buf->Format("%d", STAT(SumGruntzLostForCurrentArea, m_gruntzLost));
            return;
        case BOOTYSTAT_TOOLZ: {
            i32 total = STAT(SumToolzAvailableForCurrentArea, m_toolzAvailable);
            i32 cap = STAT(SumToolzAvailableForCurrentArea, m_toolzAvailable);
            i32 cur = STAT(SumToolzCollectedForCurrentArea, m_toolzCollected);
            cur = min(cur, cap);
            buf->Format("%d of %d", cur, total);
            return;
        }
        case BOOTYSTAT_TOYZ: {
            i32 total = STAT(SumToyzAvailableForCurrentArea, m_toyzAvailable);
            i32 cap = STAT(SumToyzAvailableForCurrentArea, m_toyzAvailable);
            i32 cur = STAT(SumToyzCollectedForCurrentArea, m_toyzCollected);
            cur = min(cur, cap);
            buf->Format("%d of %d", cur, total);
            return;
        }
        case BOOTYSTAT_POWERUPZ: {
            i32 total = STAT(SumPowerupzAvailableForCurrentArea, m_powerupzAvailable);
            i32 cap = STAT(SumPowerupzAvailableForCurrentArea, m_powerupzAvailable);
            i32 cur = STAT(SumPowerupzCollectedForCurrentArea, m_powerupzCollected);
            cur = min(cur, cap);
            buf->Format("%d of %d", cur, total);
            return;
        }
        case BOOTYSTAT_COINZ: {
            i32 total = STAT(SumCoinsAvailableForCurrentArea, m_coinsAvailable);
            i32 cap = STAT(SumCoinsAvailableForCurrentArea, m_coinsAvailable);
            i32 cur = STAT(SumCoinsCollectedForCurrentArea, m_coinsCollected);
            cur = min(cur, cap);
            buf->Format("%d of %d", cur, total);
            return;
        }
        case BOOTYSTAT_SECRETZ: {
            i32 total = STAT(SumSecretsAvailableForCurrentArea, m_secretsAvailable);
            i32 cap = STAT(SumSecretsAvailableForCurrentArea, m_secretsAvailable);
            i32 cur = STAT(SumSecretsFoundForCurrentArea, m_secretsFound);
            cur = min(cur, cap);
            buf->Format("%d of %d", cur, total);
            return;
        }
        default:
            *buf = "???";
            return;
    }
}

// @early-stop
RVA(0x0001b450, 0x1ac)
i32 CBootyState::BuildBootyWalkingGruntz() {
    if (g_gameReg->GetGameStats()->IsCustomLevel() != false) {
        return 1;
    }
    if (g_gameReg->GetGameStats()->GetLevelNumber() > IDX(QUESTLEVEL_LAST)) {
        return 1;
    }
    CShadeTable* sel = g_gameReg->GruntPalettes()->GetShadeTable(0, 0);
    if (sel == NULL) {
        return 0;
    }
    for (i32 i = 0; i < WARPLETTER_COUNT; i++) {
        m_animSprites[i] = CreateSimpleAnimationSprite(1);
        if (m_animSprites[i] == NULL) {
            return 0;
        }
        m_animSprites[i]->SetImageSetByName("GRUNTZ_NORMALGRUNT_NORTH_WALK");
        m_animSprites[i]->SetAnimationByName("GRUNTZ_NORMALGRUNT_WALK", 0);
        m_animSprites[i]->Hide();
        CWwdSpriteObject* anim = m_animSprites[i];
        anim->SetDrawFill(SHADE_PAL_16, sel);
        m_visSprites[i] = CreateSimpleAnimationSprite(1);
        if (m_visSprites[i] == NULL) {
            return 0;
        }
        RVA_DYNINIT(0x0001b670, 0xa, s_buf)
        DATA(0x0022af0c)
        static CString s_buf;
        const char* prefix = (i < (g_gameReg->GetGameStats()->GetLevelNumber() - 1) % 4 + 1)
                                 ? "GAME_INGAMEICONZ_"
                                 : "BOOTY_DIM";
        s_buf.Format("%sSECRET%c", prefix, g_secretChars[i]);
        m_visSprites[i]->SetImageSetByName(s_buf);
        m_visSprites[i]->SetAnimationByName("GAME_CYCLE100", 0);
        SET_SCREEN_POS(m_visSprites[i], g_idleSpriteIds[i] + 0xfa, 0xdc);
    }
    return 1;
}

// @early-stop
RVA(0x0001b690, 0x7e0)
i32 CBootyState::UpdateBootyWalkingGruntz() {
    CGameStats* gameStats = g_gameReg->GetGameStats();
    if (gameStats->IsCustomLevel() != false) {
        return 1;
    }
    i32 levelNumber = gameStats->GetLevelNumber();
    if (levelNumber > 0x24) {
        return 1;
    }
    if (m_stepIndex >= WARPLETTER_COUNT) {
        return 1;
    }

    if (m_skipAnimations != false) {

        if (levelNumber < 0x24) {
            for (i32 i = 0; i < WARPLETTER_COUNT; i++) {
                if (i <= (g_gameReg->GetGameStats()->GetLevelNumber() - 1) % 4) {
                    m_visSprites[i]->Hide();
                    SET_SCREEN_POS(m_animSprites[i], g_idleSpriteIds[i], 0xdc);
                    m_animSprites[i]->Show();
                    if ((g_gameReg->GetGameStats())->CurrentAreaHasWarpLetter(i) == 0) {
                        m_animSprites[i]->SetImageSetByName("GRUNTZ_NORMALGRUNT_SOUTH_IDLE");
                        m_animSprites[i]->SetAnimationByName("GRUNTZ_NORMALGRUNT_IDLE4", 0);
                    } else {
                        CString letter;
                        switch (static_cast<WarpLetter>(i)) {
                            case WARPLETTER_W:
                                letter = "W";
                                break;
                            case WARPLETTER_A:
                                letter = "A";
                                break;
                            case WARPLETTER_R:
                                letter = "R";
                                break;
                            case WARPLETTER_P:
                                letter = "P";
                                break;
                        }
                        m_animSprites[i]->SetImageSetByName("GRUNTZ_PICKUPS");
                        m_animSprites[i]->SetAnimationByName("GRUNTZ_PICKUPS_" + letter, 0);
                    }
                } else {
                    SET_SCREEN_POS(m_visSprites[i], g_idleSpriteIds[i], 0xdc);
                    m_visSprites[i]->Show();
                    m_animSprites[i]->Hide();
                }
            }
        }
        m_stepIndex = 4;
        return 1;
    }

    if (m_visSprites[0]->m_screenX != g_idleSpriteIds[0]) {
        for (i32 k = 0; k < 4; k++) {
            m_visSprites[k]->m_screenX -= 10;
        }
    }
    if (m_stepIndex == 0 && m_animSprites[0]->IsHidden()) {
        m_animSprites[0]->Show();
        SET_SCREEN_POS(m_animSprites[0], g_idleSpriteIds[0], 0x1f4);
    }

    if (m_soundStarted == false && m_animSprites[m_stepIndex]->m_screenY <= 0x195) {
        if ((g_gameReg->GetGameStats())->CurrentAreaHasWarpLetter(m_stepIndex) == 0) {
            m_soundStarted = true;
            SoundCueRegistry* ss = g_gameReg->World()->SoundRegistry();
            ss->PlayCue("GRUNTZ_WANDGRUNT_WANDZGRUNTUI1D");
        }
    }

    if (m_soundStarted != false) {
        SoundCueRegistry* ss = g_gameReg->World()->SoundRegistry();
        SoundCue* res = ss->FindCue("GRUNTZ_WANDGRUNT_WANDZGRUNTUI1D");
        if (res == NULL) {
            return 1;
        }
        if (res->IsPlaying() != 0) {
            m_visSprites[m_stepIndex]->m_stateFlags ^= SPRITE_STATE_HIDDEN;
        } else {
            m_visSprites[m_stepIndex]->Hide();
        }
    }

    if (m_walkStarted == false && m_animSprites[m_stepIndex]->m_screenY <= 0xdc) {
        {
            CString letter;
            switch (static_cast<WarpLetter>(m_stepIndex)) {
                case WARPLETTER_W:
                    letter = "W";
                    break;
                case WARPLETTER_A:
                    letter = "A";
                    break;
                case WARPLETTER_R:
                    letter = "R";
                    break;
                case WARPLETTER_P:
                    letter = "P";
                    break;
            }
            CShadeTable* sel = g_gameReg->GruntPalettes()->GetShadeTable(0, 0);
            if (sel != NULL) {
                if ((g_gameReg->GetGameStats())->CurrentAreaHasWarpLetter(m_stepIndex) != 0) {
                    PlayRegistryCueIfElapsed(g_gameReg->World()->SoundRegistry(), "GAME_FLAGRISE");
                    m_animSprites[m_stepIndex]->SetImageSetByName("GRUNTZ_PICKUPS");
                    m_animSprites[m_stepIndex]->SetAnimationByName("GRUNTZ_PICKUPS_" + letter, 0);
                    CWwdSpriteObject* g = m_animSprites[m_stepIndex];
                    g->SetDrawFill(SHADE_PAL_16, sel);
                    m_visSprites[m_stepIndex]->Hide();
                    g_gameReg->VoiceMgr()
                        ->PlayVoice(NULL, 0x3bf, GetRandomNumber() % 0x11, 1, -1, -1);
                    m_walkStarted = true;
                } else {
                    m_animSprites[m_stepIndex]->SetImageSetByName("GRUNTZ_NORMALGRUNT_SOUTH_IDLE");
                    m_animSprites[m_stepIndex]->SetAnimationByName("GRUNTZ_NORMALGRUNT_IDLE4", 0);
                    CWwdSpriteObject* g = m_animSprites[m_stepIndex];
                    g->SetDrawFill(SHADE_PAL_16, sel);
                    m_visSprites[m_stepIndex]->Hide();
                    m_stepIndex++;
                    g_gameReg->VoiceMgr()->PlayVoice(NULL, 0x441, 0, 1, -1, -1);
                    if (m_stepIndex == g_gameReg->GetGameStats()->GetLevelNumber() % 4) {
                        m_stepIndex = 4;
                        return 1;
                    }
                    if (m_stepIndex < 4) {
                        m_animSprites[m_stepIndex]->Show();
                        SET_SCREEN_POS(
                            m_animSprites[m_stepIndex],
                            g_idleSpriteIds[m_stepIndex],
                            0x1f4
                        );
                        m_soundStarted = false;
                        m_walkStarted = false;
                    }
                }
            }
        }
    } else if (m_walkStarted != false) {

        CAniAdvanceCursor* cursor = &m_animSprites[m_stepIndex]->m_animationCursor;
        if (cursor->IsComplete()) {
            m_stepIndex++;
            if (m_stepIndex == g_gameReg->GetGameStats()->GetLevelNumber() % 4) {
                m_stepIndex = 4;
                return 1;
            }
            if (m_stepIndex < 4) {
                m_animSprites[m_stepIndex]->Show();
                SET_SCREEN_POS(m_animSprites[m_stepIndex], g_idleSpriteIds[m_stepIndex], 0x1f4);
                m_walkStarted = false;
                m_soundStarted = false;
            }
        }
    } else {
        i32 nextY = m_animSprites[m_stepIndex]->m_screenY;
        nextY -= 3;
        m_animSprites[m_stepIndex]->m_screenY = nextY;
    }
    return 0;
}

RVA(0x0001c070, 0x59)
i32 CBootyState::BuildBootyPerfectAnimation() {
    CWwdSpriteObject* spr = g_gameReg->World()->ChildGroup()->CreateSprite(
        0,
        static_cast<i32>(0xffffff7e),
        0xf0,
        0x64,
        "SimpleAnimation",
        WWD_GAME_OBJECT_FLAGS_SKIP_COLLISION_KEEP_ACTIVE
    );
    m_bootyPerfectSprite = spr;
    if (!spr) {
        return 0;
    }
    spr->SetImageSetByName("BOOTY_PERFECT");
    m_bootyPerfectSprite->SetAnimationByName("GAME_CYCLE100", 0);
    return 1;
}

RVA(0x0001c0f0, 0xd5)
i32 CBootyState::CheckPerfectBonus() {
    if (!g_gameReg->GetGameStats()->IsCurrentLevelPerfect(-1)) {
        return 1;
    }
    CWwdSpriteObject* st = m_bootyPerfectSprite;
    i32 phase = st->m_screenX;
    if (phase == static_cast<i32>(0xffffff7e)) {
        CDDrawSurfaceMgr* host = g_gameReg->World();
        i32 item = g_gameReg->GetSoundVolume();
        SoundCueRegistry* cueRegistry = host->SoundRegistry();
        if (cueRegistry->IsSilent() == false) {
            SoundCue* found = cueRegistry->FindCue("BOOTY_PERFECT");
            if (found) {
                PlaySoundCueIfElapsed(found, item, 0, 0, false);
            }
        }
    }
    if (phase >= 0x302) {
        m_bootyPerfectSprite->AddFlags(IDX(WWD_GAME_OBJECT_FLAG_PENDING_DELETE));
        return 1;
    }
    m_bootyPerfectSprite->m_screenX = phase + 0xa;
    return 1;
}

RVA(0x0001c210, 0x540)
i32 CBootyState::Render() {
    IDirectDrawSurface* frameSurf =
        m_world->GetDrawTarget()->GetFrontSurface()->GetSurface()->GetDirectDrawSurface();
    if (frameSurf == NULL || frameSurf->IsLost() != 0) {
        if (RestoreGraphics() == 0) {
            m_mgr->ReportError(IDX(IDS_RESTORE_GAME), 0x459);
            return 0;
        }
    }
    SoundStream* snd = m_world->GetSoundStream();
    if (snd != NULL) {
        i32 now = static_cast<i32>(timeGetTime());
        snd->TickVolumeRamps(now);
        snd->TickStreams(now);
    }

    i64 elapsed = static_cast<i64>(g_frameTime) - m_frameTiming.GetStartTime();
    if (elapsed < m_frameTiming.GetInterval()) {
        return 0;
    }
    m_frameTiming.Start(0x21);

    switch (m_activation) {
        case BOOTYSEQ_WARP_CUE: {
            m_activation = BOOTYSEQ_GLITTER;
            SoundCueRegistry* set = g_gameReg->World()->SoundRegistry();
            set->PlayCue("BOOTY_WARP");
        }
            // FALL THROUGH

        case BOOTYSEQ_GLITTER: {
            if (UpdateWarpStoneGlitterAnimation() == 0) {
                break;
            }
            m_activation = BOOTYSEQ_LETTERS;
            SoundCueRegistry* set = g_gameReg->World()->SoundRegistry();
            set->PlayCue("BOOTY_BOOM");
            if (m_initOnce != false && g_gameReg->GetGameStats()->IsCurrentAreaComplete() != false
                && g_levelBias100 == false) {
                RECT rc;
                SET_RECT_COMPONENTS(rc, 0, 0x24, 0x1ea, 0x64);
                CString s("World Completed!");
                m_levelCompleteGate = true;
                DrawTextToOverlaySurface(m_world, &s, &rc, 0x82, 1, 0xff, 0xff, 0, 1);
            } else {
                RECT rc;
                SET_RECT_COMPONENTS(rc, 0, 0x24, 0x1ea, 0x64);
                CString s("Level Completed!");
                m_levelCompleteGate = true;
                DrawTextToOverlaySurface(m_world, &s, &rc, 0x82, 1, 0xff, 0xff, 0, 1);
            }
        }
        // FALL THROUGH
        case BOOTYSEQ_LETTERS:
            UpdateGruntSprintAnimation();
            if (UpdateStatRevealAnimation() == 0) {
                break;
            }
            m_activation = BOOTYSEQ_WALK;
        // FALL THROUGH
        case BOOTYSEQ_WALK:
            UpdateStatRevealAnimation();
            if (UpdateBootyWalkingGruntz() == 0) {
                break;
            }
            m_activation = BOOTYSEQ_PERFECT_BONUS;
            break;
        case BOOTYSEQ_PERFECT_BONUS: {
            UpdateStatRevealAnimation();
            UpdateBootyWalkingGruntz();
            CheckPerfectBonus();
            if (m_secretHudHandled == false
                && g_gameReg->GetGameStats()->IsCustomLevel() == false) {
                CString s;
                RECT rc;
                CGameStats* gameStats = g_gameReg->GetGameStats();
                if (gameStats->GetLevelNumber() > IDX(QUESTLEVEL_LAST)) {

                    if (gameStats->IsCurrentAreaComplete() != false) {
                        s = "You have completed training! Now, grab the pebble from my hand.";
                    } else {
                        s = "You are closer to achieving mastery! Keep training!";
                    }
                    SetRect(&rc, 0x194, 0xaa, 0x263, SCREEN_H_PX);
                } else {
                    if (gameStats->IsCurrentAreaComplete() != false) {
                        if (gameStats->CurrentAreaHasAllWarpLetters()) {
                            s.Format(
                                "WARP letterz recovered! Prepare to receive your cheat codez!"
                            );
                        } else {
                            s = "WARP letterz not recovered! No cheatz for you.";
                        }
                    } else if (gameStats->m_warpLetterFound != false) {
                        s = "Keep finding those WARP letterz!";
                    } else {
                        s = "Collect all four WARP letterz to receive secret bonus!";
                    }
                    SetRect(&rc, 0x194, 0xe6, 0x263, SCREEN_H_PX);
                }
                m_secretGate = true;
                DrawTextToOverlaySurface(m_world, &s, &rc, 0x6e, 1, 0xff, 0xff, 0, 1);
                m_secretHudHandled = true;
            } else if (g_gameReg->GetGameStats()->IsCustomLevel() != false) {
                m_secretHudHandled = true;
            }
            break;
        }
        case BOOTYSEQ_DONE:
            return 1;
    }

    m_world->ChildGroup()->TickKillCues(1);
    m_world->ChildGroup()->RenderChildren(m_world->GetDrawTarget()->GetBackPair());
    CDDrawSubMgrPages* dt = m_world->GetDrawTarget();
    FlipFrontAndRestoreOverlay(dt);
    m_world->SoundRegistry()->TickVolumeRamps();
    return 1;
}

RVA(0x0001c8a0, 0xec)
i32 CBootyState::RestoreGraphics() {
    if (CState::RestoreGraphics() == 0) {
        return 0;
    }
    while (ShowCursor(false) >= 0) {
    }
    CRezDir* booty = StateResources()->GetDirFromPath("IMAGEZ");
    if (booty == NULL) {
        return 0;
    }
    if (m_world->GetImageRegistry()->LoadNamespace(booty, "BOOTY", "_") == -1) {
        return 0;
    }
    CRezDir* gruntz = m_gruntResources->GetDirFromPath("IMAGEZ");
    if (gruntz == NULL) {
        return 0;
    }
    if (m_world->GetImageRegistry()->LoadNamespace(gruntz, "GRUNTZ", "_") == -1) {
        return 0;
    }
    if (m_activation != BOOTYSEQ_DONE) {
        if (LoadTitlePage("bg", 0, 0, 0, 0, true) == 0) {
            return 0;
        }
        ShowLevelCompleteMessage();
    } else {
        ShowSecretBonusMessage();
    }
    m_world->GetDrawTarget()->TransExit();
    RetireScene(0x50, 0x3e8, 0, true);
    return 1;
}

RVA(0x0001c9d0, 0x351)
void CBootyState::ShowLevelCompleteMessage() {
    for (i32 i = 0; i < 8; i++) {
        if (m_statLabelVisible[i]) {
            CRect r1(g_bootyStatLabelRects[i]);
            CString t(g_bootyStatLabels[i]);
            DrawTextToOverlaySurface(m_world, &t, &r1, 0x78, 1, 0xff, 0xff, 0, 1);
        }
        if (m_readyFlags[i]) {
            CRect r2(g_bootyStatValueRects[i]);
            CString t2;
            FormatStatValue(&t2, static_cast<BootyStatRow>(i));
            DrawTextToOverlaySurface(m_world, &t2, &r2, 0x78, 1, 0xff, 0xff, 0, 1);
        }
    }

    if (m_levelCompleteGate) {
        if (g_gameReg->GetGameStats()->IsCurrentAreaComplete() != false) {
            RECT r = {0, 0x24, 0x1ea, 0x64};
            CString s("World Completed!");
            DrawTextToOverlaySurface(m_world, &s, &r, 0x82, 1, 0xff, 0xff, 0, 1);
        } else {
            RECT r = {0, 0x24, 0x1ea, 0x64};
            CString s("Level Completed!");
            DrawTextToOverlaySurface(m_world, &s, &r, 0x82, 1, 0xff, 0xff, 0, 1);
        }
    }

    if (g_gameReg->GetGameStats()->IsCustomLevel() == false && m_secretGate != false) {
        CString s;
        RECT r;
        CGameStats* gameStats = g_gameReg->GetGameStats();
        if (gameStats->GetLevelNumber() > IDX(QUESTLEVEL_LAST)) {
            if (gameStats->IsCurrentAreaComplete() != false) {
                s = "You have completed training! Now, grab the pebble from my hand.";
            } else {
                s = "You are closer to achieving mastery! Keep training!";
            }
            SetRect(&r, 0x194, 0xaa, 0x263, SCREEN_H_PX);
        } else {
            if (gameStats->IsCurrentAreaComplete() != false) {
                if ((gameStats)->CurrentAreaHasAllWarpLetters()) {
                    s.Format("WARP letterz recovered! Prepare to receive your cheat codez!");
                } else {
                    s = "WARP letterz not recovered! No cheatz for you.";
                }
            } else {
                if (gameStats->m_warpLetterFound != false) {
                    s = "Keep finding those WARP letterz!";
                } else {
                    s = "Collect all four WARP letterz to receive secret bonus!";
                }
            }
            SetRect(&r, 0x194, 0xe6, 0x263, SCREEN_H_PX);
        }
        DrawTextToOverlaySurface(m_world, &s, &r, 0x6e, 1, 0xff, 0xff, 0, 1);
    }
}

RVA(0x0001ce10, 0xc)
i32 CBootyState::RestoreDisplay() {
    return IsActive() != 0;
}

RVA(0x0001ce30, 0x1d)
i32 CBootyState::OnPaint() {
    if (IsActive() == 0) {
        return 0;
    }
    return CState::OnPaint() != 0;
}

// @early-stop
RVA(0x0001ce60, 0x460)
i32 CBootyState::HandleContinueInput() {
    BootySeqPhase state = m_activation;
    if (state != BOOTYSEQ_PERFECT_BONUS && state != BOOTYSEQ_DONE) {
        m_skipAnimations = true;
        return 1;
    }
    CGameStats* gameStats = g_gameReg->GetGameStats();
    if (gameStats->IsCustomLevel() != false) {
        PostMessageA(g_gameReg->GetGameWindow()->GetHwnd(), WM_COMMAND, IDX(CMD_MAIN_MENU), 0);
    } else {
        if (m_initOnce == false) {
            if (gameStats->IsCurrentAreaComplete() != false) {
                m_initOnce = true;
                SoundCueRegistry* ss = g_gameReg->World()->SoundRegistry();
                ss->PlayCue("GRUNTZ_WANDGRUNT_WANDZGRUNTI3A");
                if (g_gameReg->GetGameStats()->GetLevelNumber() < 0x24) {
                    for (i32 p = 0; p < 4; p++) {
                        m_visSprites[p]->Hide();
                        SET_SCREEN_POS(m_animSprites[p], g_idleSpriteIds[p], 0xdc);
                        m_animSprites[p]->Show();
                        if ((g_gameReg->GetGameStats())->CurrentAreaHasWarpLetter(p) == 0) {
                            m_animSprites[p]->SetImageSetByName("GRUNTZ_NORMALGRUNT_SOUTH_IDLE");
                            m_animSprites[p]->SetAnimationByName("GRUNTZ_NORMALGRUNT_IDLE4", 0);
                        } else {
                            CString letter;
                            switch (static_cast<WarpLetter>(p)) {
                                case WARPLETTER_W:
                                    letter = "W";
                                    break;
                                case WARPLETTER_A:
                                    letter = "A";
                                    break;
                                case WARPLETTER_R:
                                    letter = "R";
                                    break;
                                case WARPLETTER_P:
                                    letter = "P";
                                    break;
                            }
                            m_animSprites[p]->SetImageSetByName("GRUNTZ_PICKUPS");
                            m_animSprites[p]->SetAnimationByName("GRUNTZ_PICKUPS_" + letter, 0);
                        }
                    }
                }
                CWwdSpriteObject** ap = m_warpStonePieceSprites;
                for (i32 k = 0; k < 4; k++) {
                    SET_SCREEN_POS((*ap), g_bootyLetterCoords[k].m_x, g_bootyLetterCoords[k].m_y);
                    (*ap)->Show();
                    ap++;
                }
                if (!LoadTitlePage("bg", 0, 0, 0, 0, true)) {
                    return 0;
                }
                ShowLevelCompleteMessage();
                m_world->GetDrawTarget()->TransExit();
                m_world->ChildGroup()->RenderChildren(m_world->GetDrawTarget()->GetBackPair());
                m_world->GetDrawTarget()->TransTitle();
                RetireScene(0x50, 0x3e8, 0, true);
                if (!LoadTitlePage("bg", 0, 0, 0, 0, true)) {
                    return 0;
                }
                ShowLevelCompleteMessage();
                return 1;
            }
        }
        if (m_initOnce != false && gameStats->IsCurrentAreaComplete() != false
            && gameStats->GetLevelNumber() < IDX(QUESTLEVEL_LAST)
            && state == BOOTYSEQ_PERFECT_BONUS) {
            if ((gameStats)->CurrentAreaHasAllWarpLetters()) {
                if (!ShowSecretBonusMessage()) {
                    return 0;
                }
                m_world->GetDrawTarget()->TransExit();
                RetireScene(0x50, 0x3e8, 0, true);
                m_activation = BOOTYSEQ_SECRET_PENDING;
                return 1;
            }
        }

        if (m_activation == BOOTYSEQ_SECRET_PENDING
            && (g_gameReg->GetGameStats())->IsCampaignPerfect() && m_secretBannerOnce == false) {
            m_secretBannerOnce = true;
            if (!ShowSecretBonusMessage()) {
                return 0;
            }
            m_world->GetDrawTarget()->TransExit();
            RetireScene(0x50, 0x3e8, 0, true);
            return 1;
        }

        CGameStats* nextLevelStats = g_gameReg->GetGameStats();
        if (nextLevelStats->GetLevelNumber() == IDX(QUESTLEVEL_CAMPAIGN_LAST)) {
            SoundStream* sub = m_world->SoundRegistry()->m_soundStream;
            if (sub != NULL) {
                sub->StopAllStreams();
            }
            g_gameReg->PlayMovieEntry(IDX(MOVIE_ENTRY_ENDING));
            PostMessageA(g_gameReg->GetGameWindow()->GetHwnd(), WM_COMMAND, IDX(CMD_SHOW_HELP), 0);
        } else {

            g_gameReg->LoadLevel((nextLevelStats->GetLevelNumber() % 0x28) + 1, false, 1);
        }
    }
    return 1;
}

RVA(0x0001d3e0, 0x8)
i32 CBootyState::OnLButtonDown(i32, i32, i32) {
    return HandleContinueInput();
}

RVA(0x0001d400, 0x8)
i32 CBootyState::OnRButtonDown(i32, i32, i32) {
    return HandleContinueInput();
}

RVA(0x0001d420, 0x8)
i32 CBootyState::OnKeyDown(i32, i32) {
    return HandleContinueInput();
}

RVA(0x0001d440, 0xd7d)
i32 CMultiBootyState::LoadGameAssetNamespaces(CGruntzMgr* mgr, i32 areaArg, i32 prevStateId) {
    if (!CState::LoadGameAssetNamespaces(mgr, areaArg, prevStateId)) {
        return 0;
    }
    m_mgr->EnsureStandardVideoMode(false);
    m_stateResources = m_resourceArchive->GetDirFromPath("STATEZ_BOOTY");
    if (!m_stateResources) {
        return 0;
    }
    m_gameResources = m_resourceArchive->GetDirFromPath("GAME");
    if (!m_gameResources) {
        return 0;
    }
    m_gruntResources = m_resourceArchive->GetDirFromPath("GRUNTZ");
    if (!m_gruntResources) {
        return 0;
    }
    {
        char area[128];
        sprintf(
            area,
            "AREA%i",
            IDX(LevelAreaForLevel(g_gameReg->GetGameStats()->GetLevelNumber()))
        );
        m_levelResources = m_resourceArchive->GetDirFromPath(area);
    }
    if (!m_levelResources) {
        return 0;
    }
    m_world->ChildGroup()->ClearChildren();
    {
        CRezDir* soundz = m_stateResources->GetDir("SOUNDZ");
        if (!soundz) {
            return 0;
        }
        m_world->SoundRegistry()->LoadFromTree(static_cast<CRezDir*>(soundz), "BOOTY", "_");
    }
    while (ShowCursor(false) >= 0) {
    }
    m_mgr->GetGameWindow()->DiscardMessages(WM_KEYDOWN, 0x40);

    m_reserved1b4 = 0;
    for (i32 i = 0; i < 4; i++) {
        if (g_gameReg->GetPlayer(i).HasJoinedRound() == false) {
            continue;
        }
        CShadeTable* tint =
            g_gameReg->m_gruntPalettes->GetShadeTable(IDX(g_gameReg->GetPlayer(i).GetColor()), 0);
        if (tint == NULL) {
            return 0;
        }
        CString key;

        m_puddleSprites[i] = CreateSimpleAnimationSprite(0);
        if (m_puddleSprites[i] == NULL) {
            return 0;
        }
        m_puddleSprites[i]->SetImageSetByName("GRUNTZ_GRUNTPUDDLE");
        m_puddleSprites[i]->SetAnimationByName(g_puddleSpriteKey, 0);
        (m_puddleSprites[i])->SetDrawFill(SHADE_PAL_16, tint);
        m_puddleSprites[i]->Hide();

        if (i == GetWinningPlayerIndex()) {
            m_gruntSprites[i] = CreateSimpleAnimationSprite(0);
            if (m_gruntSprites[i] == NULL) {
                return 0;
            }
            m_gruntSprites[i]->SetImageSetByName("GRUNTZ_EXITZ");
            m_gruntSprites[i]->SetAnimationByName("GAME_GRUNTFLEX", 0);
            (m_gruntSprites[i])->SetDrawFillReversed(SHADE_PAL_16, tint);
            m_gruntSprites[i]->Hide();
        } else {
            key.Format("GRUNTZ_NORMALGRUNT_IDLE%d", (g_gameReg->Rand() % 2 != 0) ? 1 : 4);
            m_gruntSprites[i] = CreateSimpleAnimationSprite(0);
            if (m_gruntSprites[i] == NULL) {
                return 0;
            }
            m_gruntSprites[i]->SetImageSetByName("GRUNTZ_NORMALGRUNT_SOUTH_IDLE");
            m_gruntSprites[i]->SetAnimationByName(key, 0);
            (m_gruntSprites[i])->SetDrawFill(SHADE_PAL_16, tint);
            m_gruntSprites[i]->Hide();
        }

        BuildPickupIconKey(
            &key,
            maxRunIndex(g_gameReg->GetGameStats()->GetToolPickupCounts(i), 22) + 1
        );
        m_toolIcons[i] = CreateSimpleAnimationSprite(0);
        if (m_toolIcons[i] == NULL) {
            return 0;
        }
        m_toolIcons[i]->SetImageSetByName(key);
        m_toolIcons[i]->SetAnimationByName("GAME_CYCLE100", 0);
        (m_toolIcons[i])->SetDrawFill(SHADE_PAL_16, tint);
        m_toolIcons[i]->Hide();

        {
            CShadeTable* iconTint = g_gameReg->m_gruntPalettes->GetShadeTable(0x10, 0);
            if (iconTint == NULL) {
                return 0;
            }
            BuildPickupIconKey(
                &key,
                maxRunIndex(g_gameReg->GetGameStats()->GetToyPickupCounts(i), 10) + 0x17
            );
            m_toyIcons[i] = CreateSimpleAnimationSprite(0);
            if (m_toyIcons[i] == NULL) {
                return 0;
            }
            m_toyIcons[i]->SetImageSetByName(key);
            m_toyIcons[i]->SetAnimationByName("GAME_CYCLE100", 0);
            (m_toyIcons[i])->SetDrawFill(SHADE_PAL_16, iconTint);
            m_toyIcons[i]->Hide();

            BuildPickupIconKey(
                &key,
                maxRunIndex(g_gameReg->GetGameStats()->GetTimedPowerupPickupCounts(i), 7) + 0x36
            );
            m_timedPowerupIcons[i] = CreateSimpleAnimationSprite(0);
            if (m_timedPowerupIcons[i] == NULL) {
                return 0;
            }
            m_timedPowerupIcons[i]->SetImageSetByName(key);
            m_timedPowerupIcons[i]->SetAnimationByName("GAME_CYCLE100", 0);
            (m_timedPowerupIcons[i])->SetDrawFill(SHADE_PAL_16, iconTint);
            m_timedPowerupIcons[i]->Hide();

            BuildPickupIconKey(
                &key,
                maxRunIndex(g_gameReg->GetGameStats()->GetCursePickupCounts(i), 4) + 0x3d
            );
            m_curseIcons[i] = CreateSimpleAnimationSprite(0);
            if (m_curseIcons[i] == NULL) {
                return 0;
            }
            m_curseIcons[i]->SetImageSetByName(key);
            m_curseIcons[i]->SetAnimationByName("GAME_CYCLE100", 0);
            (m_curseIcons[i])->SetDrawFill(SHADE_PAL_16, iconTint);
            m_curseIcons[i]->Hide();
        }

        SET_SCREEN_POS(m_puddleSprites[i], g_bootyPuddlePos[i].m_x, g_bootyPuddlePos[i].m_y);
        m_puddleSprites[i]->Show();
        SET_SCREEN_POS(m_gruntSprites[i], g_bootyGruntPos[i].m_x, g_bootyGruntPos[i].m_y);
        m_gruntSprites[i]->Show();
        SET_SCREEN_POS(m_toolIcons[i], g_bootyToolPos[i].m_x, g_bootyToolPos[i].m_y);
        m_toolIcons[i]->Show();
        SET_SCREEN_POS(m_toyIcons[i], g_bootyToyPos[i].m_x, g_bootyToyPos[i].m_y);
        m_toyIcons[i]->Show();
        SET_SCREEN_POS(
            m_timedPowerupIcons[i],
            g_bootyTimedPowerupPos[i].m_x,
            g_bootyTimedPowerupPos[i].m_y
        );
        m_timedPowerupIcons[i]->Show();
        SET_SCREEN_POS(m_curseIcons[i], g_bootyCursePos[i].m_x, g_bootyCursePos[i].m_y);
        m_curseIcons[i]->Show();
    }

    for (i32 t = 0; t < 4; t++) {
        CString tabKey;
        CString flagKey;
        GruntzPlayer* pl = &g_gameReg->GetPlayer(t);
        CShadeTable* tint = g_gameReg->m_gruntPalettes->GetShadeTable(IDX(pl->GetColor()), 0);
        if (tint == NULL) {
            return 0;
        }
        tabKey.Format("GAME_STATUSBAR_TABZ_MULTIPLAYERTAB_HEAD%d", t + 1);
        flagKey.Format("GAME_FORTRESSFLAGZ_%s", static_cast<const char*>(GetWarlordName(t)));

        m_tabSprites[t] = g_gameReg->World()->ChildGroup()->CreateSprite(
            0,
            0,
            0,
            0,
            "DoNothing",
            WWD_GAME_OBJECT_FLAGS_SKIP_COLLISION_KEEP_ACTIVE
        );
        if (m_tabSprites[t] == NULL) {
            return 0;
        }
        m_tabSprites[t]->SetImageSetByName(tabKey);
        m_tabSprites[t]->SetAnimationByName("GAME_CYCLE100", 0);
        (m_tabSprites[t])->SetDrawFill(SHADE_PAL_16, tint);
        m_tabSprites[t]->Hide();

        m_flagSprites[t] = g_gameReg->World()->ChildGroup()->CreateSprite(
            0,
            0,
            0,
            0,
            "DoNothing",
            WWD_GAME_OBJECT_FLAGS_SKIP_COLLISION_KEEP_ACTIVE
        );
        if (m_flagSprites[t] == NULL) {
            return 0;
        }
        m_flagSprites[t]->SetImageSetByName(flagKey);
        m_flagSprites[t]->SetAnimationByName("GAME_CYCLE100", 0);
        (m_flagSprites[t])->SetDrawFill(SHADE_PAL_16, tint);
        m_flagSprites[t]->Hide();

        SET_SCREEN_POS(m_tabSprites[t], g_bootyTabPos[t].m_x, g_bootyTabPos[t].m_y);
        m_tabSprites[t]->SetImageFrame((pl->HasJoinedRound() != false) ? 1 : 2);
        m_tabSprites[t]->Show();
    }

    CShadeTable* tint = g_gameReg->m_gruntPalettes->GetShadeTable(
        IDX(g_gameReg->GetPlayer(GetWinningPlayerIndex()).GetColor()),
        0
    );
    if (tint == NULL) {
        return 0;
    }
    m_fortSprite = CreateSimpleAnimationSprite(0);
    if (m_fortSprite == NULL) {
        return 0;
    }
    m_fortSprite->SetImageSetByName("LEVEL_FORT");
    m_fortSprite->SetAnimationByName("GAME_CYCLE100", 0);
    m_fortSprite->SetDrawFill(SHADE_PAL_16, tint);
    m_fortSprite->Hide();
    SET_SCREEN_POS(m_fortSprite, 0x64, 0x64);
    m_fortSprite->Show();

    CString joyKey;
    CString bootyKey;
    joyKey.Format(
        "GRUNTZ_WARLORDZ_%s_JOY",
        static_cast<const char*>(GetWarlordName(GetWinningPlayerIndex()))
    );
    bootyKey.Format(
        "GRUNTZ_WARLORDZ_%s_BOOTY",
        static_cast<const char*>(GetWarlordName(GetWinningPlayerIndex()))
    );
    m_warlordBooty = CreateSimpleAnimationSprite(0);
    if (m_warlordBooty == NULL) {
        return 0;
    }
    m_warlordBooty->SetImageSetByName(joyKey);
    m_warlordBooty->SetAnimationByName(bootyKey, 0);
    m_warlordBooty->SetDrawFill(SHADE_PAL_16, tint);
    m_warlordBooty->Hide();
    SET_SCREEN_POS(m_warlordBooty, 0x64, 0x64);
    CWwdSpriteObject* sorted = m_warlordBooty;
    sorted->SetSortKey(SORTKEY_BOOTY_WARLORD);
    m_warlordBooty->Show();

    const Coord* flagPos = g_bootyFlagPos;
    i32 w = 0;
    do {
        i32 held = g_gameReg->GetGameStats()->CountAllFlagCaptures(w);
        i32 placed = 0;
        for (i32 c = 0; c < 4; c++) {
            if (g_gameReg->GetGameStats()->GetFlagCapture(w, c) != 0) {
                i32 spread[3][3];
                spread[0][0] = 0;
                spread[0][1] = 0;
                spread[0][2] = 0;
                spread[1][0] = -1;
                spread[1][1] = 1;
                spread[1][2] = 0;
                spread[2][0] = -2;
                spread[2][1] = 0;
                spread[2][2] = 2;
                SET_SCREEN_POS(
                    m_flagSprites[c],
                    (spread[held - 1][placed] << 4) + flagPos->m_x,
                    flagPos->m_y
                );
                m_flagSprites[c]->Show();
                placed++;
            }
        }
        w++;
        flagPos++;
        // byte-evidenced: retail compares the table cursor as a signed integer.
    } while (reinterpret_cast<i32>(flagPos) < reinterpret_cast<i32>(g_bootyTabPos));
    return 1;
}

RVA(0x0001e520, 0x3e)
void CMultiBootyState::ReleaseResources() {

    SoundCueRegistry* reg = m_world->SoundRegistry();
    if (reg->m_soundStream) {
        reg->m_soundStream->StopAllStreams();
    }
    m_world->SoundRegistry()->RemoveWithPrefix("BOOTY", "_");

    m_mgr->VoiceMgr()->PauseAllVoices();
    CState::ReleaseResources();
}

RVA(0x0001e570, 0xb4)
i32 CMultiBootyState::EnterState(GameStateId previousState) {
    i32 ok = LoadTitlePage("multi", 0, 0, 0, 0, true);
    if (!ok) {
        return ok;
    }
    m_world->GetDrawTarget()->TransExit();
    RetireScene(0x50, 0x3e8, 0, true);

    CDDrawSurfaceMgr* host = g_gameReg->World();
    i32 item = g_gameReg->GetSoundVolume();
    SoundCueRegistry* cueRegistry = host->SoundRegistry();
    if (cueRegistry->IsSilent() == false) {
        SoundCue* found = cueRegistry->FindCue("BOOTY_LOOP");
        if (found) {
            PlaySoundCueIfElapsed(found, item, 0, 0, true);
        }
    }
    return 1;
}

RVA(0x0001e660, 0x81)
i32 CMultiBootyState::LeaveState(GameStateId nextState) {
    SoundCue* found = m_world->SoundRegistry()->FindCue("BOOTY_LOOP");
    if (found && found->IsPlaying()) {
        found->GetSound()->RampVolumeTo(0, 0x1f4, true);
        while (found->IsPlaying()) {
            m_world->SoundRegistry()->TickVolumeRamps();
        }
    }
    return 1;
}

RVA(0x0001e720, 0x400)
void CMultiBootyState::BuildPickupIconKey(CString* imageSetName, i32 pickupType) {
    *imageSetName = "GAME_INGAMEICONZ_";
    switch (static_cast<PickupType>(pickupType)) {
        case PICKUP_BOMB:
            *imageSetName += "TOOLZ_BOMBZ";
            return;
        case PICKUP_BOOMERANG:
            *imageSetName += "TOOLZ_BOOMERANGZ";
            return;
        case PICKUP_BRICK:
            *imageSetName += "TOOLZ_BRICKZ";
            return;
        case PICKUP_CLUB:
            *imageSetName += "TOOLZ_CLUBZ";
            return;
        case PICKUP_GAUNTLETZ:
            *imageSetName += "TOOLZ_GAUNTLETZ";
            return;
        case PICKUP_GLOVEZ:
            *imageSetName += "TOOLZ_GLOVEZ";
            return;
        case PICKUP_GOOBER:
            *imageSetName += "TOOLZ_GOOBERZ";
            return;
        case PICKUP_GRAVITYBOOTZ:
            *imageSetName += "TOOLZ_GRAVITYBOOTZ";
            return;
        case PICKUP_GUNHAT:
            *imageSetName += "TOOLZ_GUNHATZ";
            return;
        case PICKUP_NERFGUN:
            *imageSetName += "TOOLZ_NERFGUNZ";
            return;
        case PICKUP_ROCK:
            *imageSetName += "TOOLZ_ROCKZ";
            return;
        case PICKUP_SHIELD:
            *imageSetName += "TOOLZ_SHIELDZ";
            return;
        case PICKUP_SHOVEL:
            *imageSetName += "TOOLZ_SHOVELZ";
            return;
        case PICKUP_SPRING:
            *imageSetName += "TOOLZ_SPRINGZ";
            return;
        case PICKUP_SPY:
            *imageSetName += "TOOLZ_SPYZ";
            return;
        case PICKUP_SWORD:
            *imageSetName += "TOOLZ_SWORDZ";
            return;
        case PICKUP_TIMEBOMB:
            *imageSetName += "TOOLZ_TIMEBOMBZ";
            return;
        case PICKUP_TOOB:
            *imageSetName += "TOOLZ_TOOBZ";
            return;
        case PICKUP_WAND:
            *imageSetName += "TOOLZ_WANDZ";
            return;
        case PICKUP_WARPSTONE:
            *imageSetName += "TOOLZ_WARPSTONEZ1";
            return;
        case PICKUP_WELDER:
            *imageSetName += "TOOLZ_WELDERZ";
            return;
        case PICKUP_WINGZ:
            *imageSetName += "TOOLZ_WINGZ";
            return;
        case PICKUP_BABYWALKER:
            *imageSetName += "TOYZ_BABYWALKERZ";
            return;
        case PICKUP_BEACHBALL:
            *imageSetName += "TOYZ_BEACHBALLZ";
            return;
        case PICKUP_BIGWHEEL:
            *imageSetName += "TOYZ_BIGWHEELZ";
            return;
        case PICKUP_GOKART:
            *imageSetName += "TOYZ_GOKARTZ";
            return;
        case PICKUP_JACKINTHEBOX:
            *imageSetName += "TOYZ_JACKINTHEBOXZ";
            return;
        case PICKUP_JUMPROPE:
            *imageSetName += "TOYZ_JUMPROPEZ";
            return;
        case PICKUP_POGOSTICK:
            *imageSetName += "TOYZ_POGOSTICKZ";
            return;
        case PICKUP_SCROLL:
            *imageSetName += "TOYZ_SCROLLZ";
            return;
        case PICKUP_SQUEAKTOY:
            *imageSetName += "TOYZ_SQUEAKTOYZ";
            return;
        case PICKUP_YOYO:
            *imageSetName += "TOYZ_YOYOZ";
            return;
        case PICKUP_MEGAPHONE:
            *imageSetName += "POWERUPZ_MEGAPHONEZ";
            return;
        case PICKUP_GHOST:
            *imageSetName += "POWERUPZ_GHOST";
            return;
        case PICKUP_SUPERSPEED:
            *imageSetName += "POWERUPZ_SUPERSPEED";
            return;
        case PICKUP_INVULNERABILITY:
            *imageSetName += "POWERUPZ_INVULNERABILITY";
            return;
        case PICKUP_CONVERSION:
            *imageSetName += "POWERUPZ_CONVERSION";
            return;
        case PICKUP_DEATHTOUCH:
            *imageSetName += "POWERUPZ_DEATHTOUCH";
            return;
        case PICKUP_ROIDZ:
            *imageSetName += "POWERUPZ_ROIDZ";
            return;
        case PICKUP_REACTIVEARMOR:
            *imageSetName += "POWERUPZ_REACTIVEARMOR";
            return;
        case PICKUP_RANDOMCOLORZ:
            *imageSetName += "POWERUPZ_RANDOMCOLORZ";
            return;
        case PICKUP_SCREENSHAKE:
            *imageSetName += "POWERUPZ_SCREENSHAKE";
            return;
        case PICKUP_BLACKSCREEN:
            *imageSetName += "POWERUPZ_BLACKSCREEN";
            return;
        case PICKUP_MINICAM:
            *imageSetName += "POWERUPZ_MINICAM";
            return;
        default:
            *imageSetName += "POWERUPZ_COIN";
            return;
    }
}

RVA_DYNINIT(0x00082970, 0xa, g_areaNames)
RVA_DYNINIT(0x00082990, 0x79, g_areaNames)
RVA_DYNINIT(0x00082a30, 0xe, g_areaNames)
RVA_DYNINIT(0x00082a50, 0x14, g_areaNames)
DATA(0x002454e8)
CString g_areaNames[8] = {
    "Rocky Roadz",
    "Gruntziclez",
    "Trouble in the Tropicz",
    "High on Sweetz",
    "High Rollerz",
    "Honey, I Shrunk the Gruntz!",
    "The Miniature Masterz",
    "Gruntz in Space",
};

RVA_DYNINIT(0x00082a80, 0xa, g_gruntzWinApp)
RVA_DYNINIT(0x00082aa0, 0x10, g_gruntzWinApp)
RVA_DYNINIT(0x00082ac0, 0xe, g_gruntzWinApp)
RVA_DYNINIT(0x00082ae0, 0xa, g_gruntzWinApp)
DATA(0x002451a8)
CWinApp g_gruntzWinApp("Gruntz");

DATA(0x00245270)
GruntDeathType g_areaPitDeath;

RVA_DYNINIT(0x00082b00, 0xa, g_buteMgr)
RVA_DYNINIT(0x00082b20, 0xa, g_buteMgr)
RVA_DYNINIT(0x00082b40, 0xe, g_buteMgr)
RVA_DYNINIT(0x00082b60, 0xa, g_buteMgr)
DATA(0x002453d8)
CButeMgr g_buteMgr;

DATA(0x00245508)
i32 g_screenShakeMinDelayMs;
DATA(0x0024550c)
i32 g_screenShakeMaxDelayMs;

RVA_DYNINIT(0x00082b80, 0xa, g_profileTextLine1)
RVA_DYNINIT(0x00082ba0, 0xa, g_profileTextLine1)
RVA_DYNINIT(0x00082bc0, 0xe, g_profileTextLine1)
RVA_DYNINIT(0x00082be0, 0xa, g_profileTextLine1)
DATA(0x00245524)
CString g_profileTextLine1;

RVA_DYNINIT(0x00082c00, 0xa, g_profileTextLine2)
RVA_DYNINIT(0x00082c20, 0xa, g_profileTextLine2)
RVA_DYNINIT(0x00082c40, 0xe, g_profileTextLine2)
RVA_DYNINIT(0x00082c60, 0xa, g_profileTextLine2)
DATA(0x00245528)
CString g_profileTextLine2;

RVA_DYNINIT(0x00082c80, 0xa, g_profileTextLine3)
RVA_DYNINIT(0x00082ca0, 0xa, g_profileTextLine3)
RVA_DYNINIT(0x00082cc0, 0xe, g_profileTextLine3)
RVA_DYNINIT(0x00082ce0, 0xa, g_profileTextLine3)
DATA(0x0024552c)
CString g_profileTextLine3;

RVA_DYNINIT(0x00082d00, 0xa, g_profileTextLine4)
RVA_DYNINIT(0x00082d20, 0xa, g_profileTextLine4)
RVA_DYNINIT(0x00082d40, 0xe, g_profileTextLine4)
RVA_DYNINIT(0x00082d60, 0xa, g_profileTextLine4)
DATA(0x00245530)
CString g_profileTextLine4;

RVA_DYNINIT(0x00082d80, 0xa, g_profileTextLine5)
RVA_DYNINIT(0x00082da0, 0xa, g_profileTextLine5)
RVA_DYNINIT(0x00082dc0, 0xe, g_profileTextLine5)
RVA_DYNINIT(0x00082de0, 0xa, g_profileTextLine5)
DATA(0x00245514)
CString g_profileTextLine5;

RVA_DYNINIT(0x00082e00, 0xa, g_profileTextLine6)
RVA_DYNINIT(0x00082e20, 0xa, g_profileTextLine6)
RVA_DYNINIT(0x00082e40, 0xe, g_profileTextLine6)
RVA_DYNINIT(0x00082e60, 0xa, g_profileTextLine6)
DATA(0x00245518)
CString g_profileTextLine6;

RVA_DYNINIT(0x00082e80, 0xa, g_profileTextLine7)
RVA_DYNINIT(0x00082ea0, 0xa, g_profileTextLine7)
RVA_DYNINIT(0x00082ec0, 0xe, g_profileTextLine7)
RVA_DYNINIT(0x00082ee0, 0xa, g_profileTextLine7)
DATA(0x0024551c)
CString g_profileTextLine7;

RVA_DYNINIT(0x00082f00, 0xa, g_profileTextLine8)
RVA_DYNINIT(0x00082f20, 0xa, g_profileTextLine8)
RVA_DYNINIT(0x00082f40, 0xe, g_profileTextLine8)
RVA_DYNINIT(0x00082f60, 0xa, g_profileTextLine8)
DATA(0x00245520)
CString g_profileTextLine8;

DATA(0x00245534)
i32 g_attractStateCount = 0;
DATA(0x0024553c)
GruntDeathType g_areaHazardDeath = DEATH_DROP;

RVA_DYNINIT(0x00082f80, 0xa, g_coordPool)
RVA_DYNINIT(0x00082fa0, 0x17, g_coordPool)
RVA_DYNINIT(0x00082fd0, 0xe, g_coordPool)
RVA_DYNINIT(0x00082ff0, 0x2f, g_coordPool)
DATA(0x00245540)
FreeNodePool<Coord> g_coordPool;

RVA(0x0001ec20, 0xa0)
CString CMultiBootyState::GetWarlordName(i32 id) {
    switch (static_cast<WarlordOwner>(id)) {
        case WARLORDZ_KING:
            return CString("KING");
        case WARLORDZ_NAPOLEAN:
            return CString("NAPOLEAN");
        case WARLORDZ_PATTON:
            return CString("PATTON");
        case WARLORDZ_VIKING:
            return CString("VIKING");
        default:
            return CString("");
    }
}

RVA(0x0001ecf0, 0x2a)
i32 CMultiBootyState::GetWinningPlayerIndex() {
    i32 playerIndex = 0;
    while (playerIndex < 4) {
        GruntzPlayer* player = &g_gameReg->GetPlayer(playerIndex);
        if (player->HasJoinedRound() != false && player->IsEliminated() == false) {
            return player->GetPlayerIndex();
        }
        playerIndex++;
    }
    return 0;
}

// @early-stop
RVA(0x0001ed30, 0x5ac)
void CMultiBootyState::DrawBattleStats() {
    CString s;
    CRect rc;
    i32 i;
    i32 c;

    for (i = 0; i < 4; i++) {
        if (g_gameReg->GetPlayer(i).HasJoinedRound() != false) {
            s.Format("%d", sumRun(g_gameReg->GetGameStats()->GetCursePickupCounts(i), 4));
            rc.CopyRect(&s_col1Rects[i]);
            DrawTextToOverlaySurface(m_world, &s, &rc, 0x78, 1, 0xff, 0xff, 0, 1);

            s.Format("%d", sumRun(g_gameReg->GetGameStats()->GetTimedPowerupPickupCounts(i), 7));
            rc.CopyRect(&s_col2Rects[i]);
            DrawTextToOverlaySurface(m_world, &s, &rc, 0x78, 1, 0xff, 0xff, 0, 1);

            s.Format("%d", sumRun(g_gameReg->GetGameStats()->GetToyPickupCounts(i), 10));
            rc.CopyRect(&s_col3Rects[i]);
            DrawTextToOverlaySurface(m_world, &s, &rc, 0x78, 1, 0xff, 0xff, 0, 1);

            s.Format("%d", sumRun(g_gameReg->GetGameStats()->GetToolPickupCounts(i), 22));
            rc.CopyRect(&s_col4Rects[i]);
            DrawTextToOverlaySurface(m_world, &s, &rc, 0x78, 1, 0xff, 0xff, 0, 1);

            s.Format("%d", g_gameReg->GetGameStats()->m_gruntzSpawnedByPlayer[i]);
            rc.CopyRect(&s_col5Rects[i]);
            DrawTextToOverlaySurface(m_world, &s, &rc, 0x78, 1, 0xff, 0xff, 0, 1);

            s.Format("%d", (g_gameReg->GetGameStats())->CountKillsForPlayer(i));
            rc.CopyRect(&s_col6Rects[i]);
            DrawTextToOverlaySurface(m_world, &s, &rc, 0x78, 1, 0xff, 0xff, 0, 1);
        }
    }

    for (c = 0; c < IDX(BATTLEROW_COUNT); c++) {
        BattleStatRow row = static_cast<BattleStatRow>(c);
        switch (row) {
            case BATTLEROW_FORTZ:
                s = "Fortz:";
                break;
            case BATTLEROW_KILLZ:
                s = "Killz:";
                break;
            case BATTLEROW_GRUNTZ:
                s = "Gruntz:";
                break;
            case BATTLEROW_TOOLZ:
                s = "Toolz:";
                break;
            case BATTLEROW_TOYZ:
                s = "Toyz:";
                break;
            case BATTLEROW_POWERUPZ:
                s = "Powerupz:";
                break;
            case BATTLEROW_CURSEZ:
                s = "Cursez:";
                break;
        }
        rc.CopyRect(&s_labelRects[c]);
        DrawTextToOverlaySurface(m_world, &s, &rc, 0x78, 1, 0xff, 0xff, 0, 1);
    }

    for (i = 0; i < 4; i++) {
        GruntzPlayer* player = &g_gameReg->GetPlayer(i);
        if (player->HasJoinedRound() != false) {
            i32 color;
            switch (player->GetColor()) {
                case TINT_DKBLUE:
                    color = RGB(0, 0, 128);
                    break;
                case TINT_DKGREEN:
                    color = RGB(0, 128, 0);
                    break;
                case TINT_TURQ:
                    color = RGB(0, 128, 128);
                    break;
                case TINT_DKRED:
                    color = RGB(128, 0, 0);
                    break;
                case TINT_PURPLE:
                    color = RGB(128, 0, 128);
                    break;
                case TINT_DKYELLOW:
                    color = RGB(128, 128, 0);
                    break;
                case TINT_GREY:
                    color = RGB(128, 128, 128);
                    break;
                case TINT_BLUE:
                    color = RGB(0, 0, 255);
                    break;
                case TINT_GREEN:
                    color = RGB(0, 255, 0);
                    break;
                case TINT_CYAN:
                    color = RGB(0, 255, 255);
                    break;
                case TINT_RED:
                    color = RGB(255, 0, 0);
                    break;
                case TINT_PINK:
                    color = RGB(255, 0, 255);
                    break;
                case TINT_YELLOW:
                    color = RGB(255, 255, 0);
                    break;
                case TINT_WHITE:
                    color = RGB(255, 255, 255);
                    break;
                case TINT_ORANGE:
                    color = RGB(255, 128, 0);
                    break;
                case TINT_HOTPINK:
                    color = RGB(255, 0, 128);
                    break;
                default:
                    color = RGB(0, 0, 0);
                    break;
            }
            s.Format("%s", static_cast<const char*>(player->GetName()));
            rc.CopyRect(&s_colorRects[i]);
            DrawTextToOverlaySurface(
                m_world,
                &s,
                &rc,
                0x64,
                0,
                color & 0xff,
                (color >> 8) & 0xff,
                (color >> 0x10) & 0xff,
                1
            );
        }
    }

    s.Format("BATTLE STATZ");
    rc.top = 0xf;
    rc.bottom = 0x73;
    rc.left = 0x96;
    rc.right = SCREEN_W_PX;
    DrawTextToOverlaySurface(m_world, &s, &rc, 0x82, 1, 0xff, 0xff, 0, 1);
}

RVA(0x0001f480, 0x1e9)
i32 CMultiBootyState::Render() {
    IDirectDrawSurface* frameSurf =
        m_world->GetDrawTarget()->GetFrontSurface()->GetSurface()->GetDirectDrawSurface();
    if (frameSurf == NULL || frameSurf->IsLost() != 0) {
        if (RestoreGraphics() == 0) {
            m_mgr->ReportError(IDX(IDS_RESTORE_GAME), 0x459);
            return 0;
        }
    }
    if (m_sequenceState == BOOTYSEQ_WARP_CUE) {
        DrawBattleStats();
        m_sequenceState = BOOTYSEQ_PERFECT_BONUS;
    }
    m_world->ChildGroup()->TickKillCues(1);
    m_world->ChildGroup()->RenderChildren(m_world->GetDrawTarget()->GetBackPair());

    u32 secs = g_gameReg->GetGameStats()->m_elapsedTimeMs / MILLIS_PER_SECOND;
    CString s;
    RECT rc;
    SetRect(&rc, 8, 0x41, 0xcb, 0xae);
    if (secs / 3600 != 0) {
        s.Format("%d:%2.2d:%2.2d", secs / 3600, (secs / 60) % 60, secs % 60);
    } else {
        s.Format("%d:%2.2d", secs / 60, secs % 60);
    }
    DrawTextToBackSurface(m_world, &s, &rc, 0x6e, 1, 0xff, 0xff, 0, 1);

    CDDrawSubMgrPages* dt = m_world->GetDrawTarget();
    FlipFrontAndRestoreOverlay(dt);
    m_world->SoundRegistry()->TickVolumeRamps();
    return 1;
}

RVA(0x0001f6f0, 0x10b)
i32 CMultiBootyState::RestoreGraphics() {
    if (!CState::RestoreGraphics()) {
        return 0;
    }

    while (ShowCursor(false) >= 0)
        ;

    CRezDir* tree = StateResources()->GetDirFromPath("IMAGEZ");
    if (!tree) {
        return 0;
    }
    CDDrawWorkerRegistry* reg = m_world->GetImageRegistry();
    if (reg->LoadNamespace(tree, "BOOTY", "_") == -1) {
        return 0;
    }

    tree = m_gruntResources->GetDirFromPath("IMAGEZ");
    if (!tree) {
        return 0;
    }
    reg = m_world->GetImageRegistry();
    if (reg->LoadNamespace(tree, "GRUNTZ", "_") == -1) {
        return 0;
    }

    tree = m_levelResources->GetDirFromPath("IMAGEZ");
    if (!tree) {
        return 0;
    }
    reg = m_world->GetImageRegistry();
    if (reg->LoadNamespace(tree, "LEVEL", "_") == -1) {
        return 0;
    }

    if (!LoadTitlePage("multi", 0, 0, 0, 0, true)) {
        return 0;
    }

    DrawBattleStats();
    m_world->GetDrawTarget()->TransExit();
    RetireScene(0x50, 0x3e8, 0, true);
    return 1;
}

RVA(0x0001f850, 0xc)
i32 CMultiBootyState::RestoreDisplay() {
    return IsActive() != 0;
}

RVA(0x0001f870, 0x1d)
i32 CMultiBootyState::OnPaint() {
    if (IsActive() == 0) {
        return 0;
    }
    return CState::OnPaint() != 0;
}

RVA(0x0001f8a0, 0x30)
i32 CMultiBootyState::PostCommandIfKey() {
    if (m_sequenceState == BOOTYSEQ_PERFECT_BONUS) {
        PostMessageA(g_gameReg->GetGameWindow()->GetHwnd(), WM_COMMAND, IDX(CMD_MAIN_MENU), 0);
    }
    return 1;
}

RVA(0x0001f8e0, 0x8)
i32 CMultiBootyState::OnLButtonDown(i32, i32, i32) {
    return PostCommandIfKey();
}

RVA(0x0001f900, 0x8)
i32 CMultiBootyState::OnRButtonDown(i32, i32, i32) {
    return PostCommandIfKey();
}

RVA(0x0001f920, 0x8)
i32 CMultiBootyState::OnKeyDown(i32, i32) {
    return PostCommandIfKey();
}

RVA(0x0001f940, 0x4c)
i32 SoundCue::PlayIfElapsed(
    i32 volumePercent,
    i32 panPercent,
    i32 frequencyOffsetPercent,
    b32 looping
) {
    return PlaySoundCueIfElapsed(this, volumePercent, panPercent, frequencyOffsetPercent, looping);
}

RVA_COMPGEN(0x0008d410, 0x1e, ??_GCBootyState@@UAEPAXI@Z)
RVA(0x0008d440, 0x55)
CBootyState::~CBootyState() {
    ReleaseResources();
}

RVA_COMPGEN(0x0008d4e0, 0x1e, ??_GCMultiBootyState@@UAEPAXI@Z)
RVA(0x0008d510, 0x55)
CMultiBootyState::~CMultiBootyState() {
    ReleaseResources();
}
