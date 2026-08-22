#include <rva.h>

#include <Gruntz/BootyStateActivate.h>

#include <Mfc.h>
#include <MfcWin.h>

#include <AddrWord.h>
#include <Bute/ButeMgr.h>
#include <Bute/SymParser.h>
#include <Bute/SymTab.h>
#include <DDrawMgr/DDrawChildGroup.h>
#include <DDrawMgr/DDrawSubMgrLeafScan.h>
#include <DDrawMgr/DDrawSubMgrLeafScanInline.h>
#include <DDrawMgr/DDrawSubMgrPages.h>
#include <DDrawMgr/DDrawSubMgrPagesInline.h>
#include <DDrawMgr/DDrawSurfaceMgr.h>
#include <DDrawMgr/DDrawSurfacePair.h>
#include <DDrawMgr/DDrawWorkerRegistry.h>
#include <DDrawMgr/DDSurface.h>
#include <Dsndmgr/DirectSoundMgr.h>
#include <Dsndmgr/SoundStream.h>
#include <Enums.h>
#include <Gruntz/AniAdvanceCursorInline.h>
#include <Gruntz/Attract.h>
#include <Gruntz/BankMgr.h>
#include <Gruntz/BattleStatRow.h>
#include <Gruntz/BattlezData.h>
#include <Gruntz/BootyCheatState.h>
#include <Gruntz/BootyMessages.h>
#include <Gruntz/BootySeqPhase.h>
#include <Gruntz/BootyStatRow.h>
#include <Gruntz/BootyWalkAnim.h>
#include <Gruntz/BzState.h>
#include <Gruntz/ColorTint.h>
#include <Gruntz/CoordNode.h>
#include <Gruntz/DirectionRingIndex.h>
#include <Gruntz/ErrorStringId.h>
#include <Gruntz/FreeNodePool.h>
#include <Gruntz/GameMode.h>
#include <Gruntz/GameRand.h>
#include <Gruntz/GameRegistry.h>
#include <Gruntz/GameRegMfcPtr.h>
#include <Gruntz/GameStateId.h>
#include <Gruntz/GameText.h>
#include <Gruntz/GlyphStringDraw.h>
#include <Gruntz/GruntDeathType.h>
#include <Gruntz/GruntDirection.h>
#include <Gruntz/GruntDirStatics.h>
#include <Gruntz/GruntPuddle.h>
#include <Gruntz/GruntSpawnConfig.h>
#include <Gruntz/GruntzCommandId.h>
#include <Gruntz/GruntzMgr.h>
#include <Gruntz/ImageState.h>
#include <Gruntz/LeafCue.h>
#include <Gruntz/LeafCueInline.h>
#include <Gruntz/LightFxMgr.h>
#include <Gruntz/MgrAutoScroll.h>
#include <Gruntz/PickupType.h>
#include <Gruntz/Play.h>
#include <Gruntz/QuestLevel.h>
#include <Gruntz/SortKeyLayer.h>
#include <Gruntz/SortKeyMacros.h>
#include <Gruntz/SoundCue.h>
#include <Gruntz/SoundState.h>
#include <Gruntz/Sprite.h>
#include <Gruntz/SpriteRefTable.h>
#include <Gruntz/SpriteStateFlags.h>
#include <Gruntz/String.h>
#include <Gruntz/TypeKeyColl.h>
#include <Gruntz/UserLogic.h>
#include <Gruntz/WarlordOwner.h>
#include <Gruntz/WarpLetter.h>
#include <Gruntz/WwdGameReg.h>
#include <Image/CImage.h>
#include <Ints.h>
#include <Rez/FrameClock.h>
#include <Rez/RezSync.h>
#include <Utils/MapTyped.h>
#include <Wap32/ScreenGeometry.h>

#include <ddraw.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

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
// Eight SEPARATE arrays, not one 8x4 table: retail addresses every row with an
// absolute `[i*8 + &row]`, which is only forced when the rows are distinct symbols
// (the inter-row distance is then a link-time value, so cl cannot fold the twelve
// row accesses of LoadGameAssetNamespaces onto one cursor the way it does for a
// single 2-D object). One row per sprite kind, which is also what the code reads.
DATA(0x001e9078)
const Coord g_bootyMiscPos[4] = {{190, 437}, {306, 437}, {422, 437}, {538, 437}};
DATA(0x001e9098)
const Coord g_bootyPowerupPos[4] = {{190, 394}, {306, 394}, {422, 394}, {538, 394}};
DATA(0x001e90b8)
const Coord g_bootyToyPos[4] = {{190, 351}, {306, 351}, {422, 351}, {538, 351}};
DATA(0x001e90d8)
const Coord g_bootyWeaponPos[4] = {{190, 308}, {306, 308}, {422, 308}, {538, 308}};
DATA(0x001e90f8)
const Coord g_bootyGruntPos[4] = {{190, 265}, {306, 265}, {422, 265}, {538, 265}};
DATA(0x001e9118)
const Coord g_bootyPuddlePos[4] = {{190, 222}, {306, 222}, {422, 222}, {538, 222}};
DATA(0x001e9138)
const Coord g_bootyFlagPos[4] = {{218, 180}, {334, 180}, {450, 180}, {566, 180}};
DATA(0x001e9158)
const Coord g_bootyTabPos[4] = {{218, 138}, {334, 138}, {450, 138}, {566, 138}};
// Retail put these eight arrays at 0x1e9178..0x1e93a8, inside `.rdata` - whose
// section characteristics are 0x40000040, READ with no WRITE bit. Read-only
// storage is only reachable for a `const` object, so retail declared them const;
// spelled non-const they landed in our `.data`, the storage classes disagreed
// and all 560 bytes went unenrolled, hence uncompared (`audit.data_coverage`).
DATA(0x001e9178)
const RECT g_col1Rects[4] =
    {{200, 415, 284, 465}, {316, 415, 400, 465}, {432, 415, 516, 465}, {548, 415, 632, 465}};
DATA(0x001e91b8)
const RECT g_col2Rects[4] =
    {{200, 372, 284, 422}, {316, 372, 400, 422}, {432, 372, 516, 422}, {548, 372, 632, 422}};
DATA(0x001e91f8)
const RECT g_col3Rects[4] =
    {{200, 329, 284, 379}, {316, 329, 400, 379}, {432, 329, 516, 379}, {548, 329, 632, 379}};
DATA(0x001e9238)
const RECT g_col4Rects[4] =
    {{200, 286, 284, 336}, {316, 286, 400, 336}, {432, 286, 516, 336}, {548, 286, 632, 336}};
DATA(0x001e9278)
const RECT g_col5Rects[4] =
    {{200, 243, 284, 293}, {316, 243, 400, 293}, {432, 243, 516, 293}, {548, 243, 632, 293}};
DATA(0x001e92b8)
const RECT g_col6Rects[4] =
    {{200, 200, 284, 250}, {316, 200, 400, 250}, {432, 200, 516, 250}, {548, 200, 632, 250}};
DATA(0x001e92f8)
const RECT g_colorRects[4] =
    {{50, 87, 390, 115}, {166, 87, 506, 115}, {282, 87, 622, 115}, {398, 87, 738, 115}};
DATA(0x001e9338)
const RECT g_labelRects[7] = {
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
static const float kGlitterPhaseBias = -225.0f;
DATA(0x001e93b8)
static const double kDegToRad = 0.017453292;
DATA(0x001e93c0)
static const double kGlitterShrinkRate = 0.002;
DATA(0x001e93c8)
static const double kGlitterStartRadius = 350.0;

DATA(0x0020b838)
RECT g_levelMsgRectsA[8] = {
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
RECT g_levelMsgRectsB[8] = {
    {245, 92, 417, 162},
    {245, 135, 417, 205},
    {245, 180, 417, 250},
    {245, 227, 417, 297},
    {245, 266, 417, 340},
    {245, 310, 417, 380},
    {245, 351, 417, 421},
    {245, 392, 417, 462}
};

RVA_DYNINIT(0x00018720, 0xa, g_levelMsgStrings)
RVA_DYNINIT(0x00018740, 0x79, g_levelMsgStrings)
RVA_DYNINIT(0x000187e0, 0xe, g_levelMsgStrings)
RVA_DYNINIT(0x00018800, 0x14, g_levelMsgStrings)
DATA(0x00229ef8)
CString g_levelMsgStrings[8] = {
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
i32 g_bootyCheatBuilt = 0;

// @early-stop
RVA(0x00018830, 0x380)
i32 CBootyState::LoadGameAssetNamespaces(CGruntzMgr* mgr, i32 areaArg, i32 prevStateId) {

    if (!CState::LoadGameAssetNamespaces(mgr, areaArg, prevStateId)) {
        return 0;
    }

    if (g_bootyCheatBuilt == 0) {
        CString bootyCheatz("BootyCheatz");
        CString empty("");
        CString grp;
        CString text;
        CString desc;
        i32 i = 0;

        AddrWord<char> cur;
        AddrWord<char> last;
        // Retail compares against 0x22aef0 - the 26th row's strB, one row past the
        // array, which lands in the GruntDirStatics copies that follow it.
        last.m_addr = g_secretMsgRows[24].strB + sizeof(SecretMsgRow);
        char* p = g_secretMsgRows[0].strB;
        do {
            grp.Format("A%dC%d", i / 3 + 1, i % 3 + 1);
            i32 id = g_buteMgr.GetIntDef(bootyCheatz, grp, 1);
            grp.Format("Cheat%i", id);
            text = *g_buteMgr.GetStringDef(grp, "Text", &empty);
            desc = *g_buteMgr.GetStringDef(grp, "Desc", &empty);
            strcpy(p - 0x20, text);
            strcpy(p, desc);
            i++;
            p += 0xa0;
            cur.m_addr = p;
        } while (cur.m_word < last.m_word);
        g_bootyCheatBuilt = 1;
    }

    m_mgr->RestoreVideoMode(0);

    m_stateBank = m_symParser->ResolvePath("STATEZ_BOOTY");
    if (!m_stateBank) {
        return 0;
    }
    m_gameBank = m_symParser->ResolvePath("GAME");
    if (!m_gameBank) {
        return 0;
    }
    m_gruntzBank = m_symParser->ResolvePath("GRUNTZ");
    if (!m_gruntzBank) {
        return 0;
    }

    m_world->m_childGroup->ClearChildren();

    {
        CSymTab* soundz = SymTab2c()->FindSub("SOUNDZ");
        if (!soundz) {
            return 0;
        }
        m_world->m_soundRegistry->ScanTree(static_cast<CSymTab*>(soundz), "BOOTY", "_");

        CSymTab* wand = m_gruntzBank->ResolvePath("SOUNDZ_WANDGRUNT");
        if (!wand) {
            return 0;
        }
        m_world->m_soundRegistry->ScanTree(static_cast<CSymTab*>(wand), "GRUNTZ_WANDGRUNT", "_");

        CSymTab* imagez = SymTab2c()->FindSub("IMAGEZ");
        if (!imagez) {
            return 0;
        }
        m_world->m_imageRegistry->InstallTree(imagez, "BOOTY", "_");
    }

    {
        int(WINAPI * sc)(BOOL) = ShowCursor;
        while (sc(0) >= 0) {
        }
    }

    m_mgr->m_gameWnd->PumpMessages(0x100, 0x40);

    m_secretHudHandled = 0;

    if (!BuildWarpStoneGlitterAnimation()) {
        return 0;
    }
    if (!BuildGruntSprintAnimation()) {
        return 0;
    }
    if (!LoadGruntEffectSprites()) {
        return 0;
    }
    if (!BuildBootyWalkingGruntz()) {
        return 0;
    }
    if (!BuildBootyPerfectAnimation()) {
        return 0;
    }

    m_frameIntervalLo = 0x21;
    m_frameIntervalHi = 0;
    m_frameStampLo = g_frameTime;
    m_frameStampHi = 0;
    return 1;
}

// @early-stop
// Extent, calls, CFG, constants, and ordered referents are exact; only the
// scratch-register rotation across the member re-reads remains.
RVA(0x00018c90, 0x72)
void CBootyState::ReleaseResources() {
    SoundStream* r = m_world->m_soundRegistry->m_soundStream;
    if (r) {
        r->Stop();
    }
    m_world->m_soundRegistry->RemoveKeysEqual("BOOTY", "_");
    m_world->m_soundRegistry->RemoveKeysEqual("GRUNTZ_WANDGRUNT", "_");
    m_world->m_imageRegistry->RemoveKeysEqual("BOOTY", "_");
    m_world->m_imageRegistry->RemoveKeysEqual("GRUNTZ_GOKARTGRUNT", "_");
    CState::ReleaseResources();
}

// @early-stop
RVA(0x00018d30, 0xcd)
i32 CBootyState::EnterState(GameStateId) {
    while (ShowCursor(FALSE) >= 0)
        ;
    if (!FadeInTitle("bg", 0, 0, 0, 0, 1)) {
        return 0;
    }
    m_world->m_drawTarget->TransExit();
    RetireScene(0x50, 0x3e8, 0, 1);

    CGruntzMgr* reg = g_gameReg;
    CDDrawSubMgrLeafScan* set = reg->m_world->m_soundRegistry;
    i32 token = reg->m_soundVolume;
    if (set->m_emitGate == 0) {
        LeafCue* found = NULL;
        MapLookup(set->m_cues, "BOOTY_LOOP", found);
        if (found != NULL) {
            PlayLeafCueIfElapsed(found, token, 0, 0, 1);
        }
    }
    return 1;
}

static inline LeafCue* LookupCue(CMapStringToPtr& cues, LPCTSTR name) {
    LeafCue* found = NULL;
    MapLookup(cues, name, found);
    return found;
}

RVA(0x00018e40, 0x81)
i32 CBootyState::LeaveState(GameStateId) {
    LeafCue* found = LookupCue(m_world->m_soundRegistry->m_cues, "BOOTY_LOOP");
    if (found && found->m_sound->IsPlaying()) {
        found->m_sound->CloneAndPlay(0, 0x1f4, 1);
        while (found->m_sound->IsPlaying()) {
            PurgeVoices(m_world->m_soundRegistry);
        }
    }
    return 1;
}

RVA(0x00018f00, 0x4fb)
i32 CBootyState::ShowSecretBonusMessage() {
    if (m_secretBannerOnce != 0 && (g_gameReg->m_scoreHud)->AllRecordsInBounds()) {
        CString s;
        if (!FadeInTitle("multi", 0, 0, 0, 0, 1)) {
            return 0;
        }
        RECT rA, rB, rTitle;
        SetRect(&rA, 0, -15, SCREEN_W_PX, 0x1d1);
        SetRect(&rB, 0, 0x19, SCREEN_W_PX, 0x1f9);
        SetRect(&rTitle, 0, 0x38, SCREEN_W_PX, 0x78);
        s.Format("The Secret of Secretz:");
        ShowHudMessage(m_world, &s, &rTitle, 0x82, 1, 0xff, 0xff, 0, 1);

        CString s2(g_secretMsgRows[24].strA);
        CString s3(g_secretMsgRows[24].strB);
        for (i32 k = 0; k < s2.GetLength(); k++) {
            s2.SetAt(k, static_cast<char>(((static_cast<const char*>(s2))[k] - 0x3d)));
        }
        ShowHudMessage(m_world, &s2, &rA, 0x78, 1, 0xff, 0xff, 0, 1);
        ShowHudMessage(m_world, &s3, &rB, 0x6e, 1, 0xff, 0xff, 0, 1);
        return 1;
    } else {
        i32 count = static_cast<i32>(((g_gameReg->m_scoreHud)->GroupRatio() * g_secretRatioScale));
        i32 rowBase = (g_gameReg->m_scoreHud->m_count - 1) / 4;
        SecretBonusTier category =
            (count >= 0x64) ? SECRET_BONUS_TIER_THREE
                            : ((count >= 0x32) ? SECRET_BONUS_TIER_TWO : SECRET_BONUS_TIER_ONE);

        if (!FadeInTitle("multi", 0, 0, 0, 0, 1)) {
            return 0;
        }
        CString title;
        RECT rTitle;
        SetRect(&rTitle, 0, 0x38, SCREEN_W_PX, 0x78);
        // Retail branches on the tier here and calls Format on both arms with a literal that
        // pools to one address (0x60babc): the singular/plural texts came out identical.
        if (category == SECRET_BONUS_TIER_ONE) {
            title.Format("Secret Bonus Acquired:");
        } else {
            title.Format("Secret Bonus Acquired:");
        }
        ShowHudMessage(m_world, &title, &rTitle, 0x82, 1, 0xff, 0xff, 0, 1);

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
            CString s5(g_secretMsgRows[idx].strA);
            CString s6(g_secretMsgRows[idx].strB);
            for (i32 k = 0; k < s5.GetLength(); k++) {
                s5.SetAt(k, static_cast<char>(((static_cast<const char*>(s5))[k] - 0x3d)));
            }
            ShowHudMessage(m_world, &s5, &rA, 0x78, 1, 0xff, 0xff, 0, 1);
            ShowHudMessage(m_world, &s6, &rB, 0x6e, 1, 0xff, 0xff, 0, 1);
        }
        return 1;
    }
}

RVA(0x00019540, 0x12a)
i32 CBootyState::BuildWarpStoneGlitterAnimation() {
    CWwdGameObjectA** slot = m_trailSprites;
    m_letterIdx = (g_gameReg->m_scoreHud->m_count - 1) % 4;
    m_radius = 0xc8;
    m_angleStep = 0;
    m_scratchX = 0;
    m_scratchY = 0;
    for (i32 i = 0; i < 4; i++) {
        CWwdGameObjectA* a =
            g_gameReg->m_world->m_childGroup
                ->CreateSprite(0, 0, 0, (i != m_letterIdx) ? 1 : 3, "DoNothing", 3);
        slot[i] = a;
        if (a == NULL) {
            return 0;
        }
        a->ApplyLookupSprite("GAME_STATUSBAR_TABZ_GAMETAB_WARPSTONE", i + 2);
        slot[i]->m_stateFlags |= SPRITE_STATE_HIDDEN;
    }
    for (i32 k = 0; k <= m_letterIdx; k++) {
        slot[k]->m_stateFlags &= ~SPRITE_STATE_HIDDEN;
    }
    CWwdGameObjectA* g =
        g_gameReg->m_world->m_childGroup->CreateSprite(0, 0, 0, 4, "SimpleAnimation", 3);
    m_cursorLetter = g;
    if (g == NULL) {
        return 0;
    }
    g->ApplyName("GAME_GLITTERGOLD");
    m_cursorLetter->ApplyLookupGeometry("GAME_CYCLE100", 0);
    return 1;
}

// @early-stop
RVA(0x000196c0, 0x1d3)
i32 CBootyState::StepGlitterAnim() {
    if (m_initGate) {
        for (i32 i = 0; i <= m_letterIdx; i++) {
            CWwdGameObjectA* e = m_trailSprites[i];
            e->m_screenX = g_bootyLetterCoords[i].m_x;
            e = m_trailSprites[i];
            e->m_screenY = g_bootyLetterCoords[i].m_y;
            e = m_trailSprites[i];
            SET_SORT_KEY_IF_CHANGED(e, 1)
        }
        m_cursorLetter->m_screenX = g_bootyLetterCoords[m_letterIdx].m_x;
        m_cursorLetter->m_screenY = g_bootyLetterCoords[m_letterIdx].m_y;
        return 1;
    }

    i32 step = m_angleStep;
    i32 idx = m_letterIdx;
    double r = static_cast<float>(m_radius);
    double ang = (static_cast<float>(step) - kGlitterPhaseBias) * kDegToRad;
    m_scratchX = static_cast<i32>((sin(ang) * r + g_bootyLetterCoords[idx].m_x));
    m_scratchY = static_cast<i32>((cos(ang) * r + g_bootyLetterCoords[idx].m_y));
    m_angleStep = step + 5;
    double shrink = static_cast<float>(step + 5) * kGlitterShrinkRate;
    m_radius = static_cast<i32>((kGlitterStartRadius - shrink * kGlitterStartRadius));

    i32 i = 0;
    if (idx > 0) {
        do {
            CWwdGameObjectA* e = m_trailSprites[i];
            e->m_screenX = g_bootyLetterCoords[i].m_x;
            e = m_trailSprites[i];
            e->m_screenY = g_bootyLetterCoords[i].m_y;
            i++;
        } while (i < m_letterIdx);
    }

    m_cursorLetter->m_screenX = m_scratchX;
    m_cursorLetter->m_screenY = m_scratchY;
    m_trailSprites[i]->m_screenX = m_scratchX;
    m_trailSprites[i]->m_screenY = m_scratchY;

    MoveLettersByDir();

    if (m_radius == 0) {
        CWwdGameObjectA* e = m_trailSprites[i];
        SET_SORT_KEY_IF_CHANGED(e, 1)
        return 1;
    }
    return 0;
}

// @early-stop
RVA(0x00019920, 0x1f0)
i32 CBootyState::BuildGruntSprintAnimation() {
    CShadeTable* h = g_gameReg->m_spriteFactory->GetSel(0, 0);
    if (!h) {
        return 0;
    }

    for (i32 i = 0; i < 8; i++) {
        m_sprintSprites[i] =
            g_gameReg->m_world->m_childGroup->CreateSprite(0, 0, 0, 2, "SimpleAnimation", 3);
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

        m_sprintSprites[i]->ApplyName("GRUNTZ_NORMALGRUNT_" + dir + "_WALK");
        m_sprintSprites[i]->ApplyLookupGeometry("GAME_GRUNTSPRINT", 0);
        {
            CWwdGameObjectA* o = m_sprintSprites[i];
            o->m_drawActive = 1;
            o->m_drawFillCmd = SHADE_PAL_16;
            o->m_drawFillArg = h;
        }

        i32 outX, outY;
        GenMenuRandPos(static_cast<GruntDirection>(i + 1), &outX, &outY);
        m_sprintSprites[i]->m_screenX = outX;
        m_sprintSprites[i]->m_screenY = outY;
    }
    return 1;
}

// @early-stop
// Every arm body and both loops now agree instruction-for-instruction with retail; what
// is left is one whole-function register permutation (retail esi/ecx/edx/edi/ebx/eax for
// e/x/y/p/i/1, ours edx/eax/ecx/esi/edi/ebx), which also picks `add ecx,0x204` over
// retail's `lea edx,[ecx+0x204]` for the cursor.
RVA(0x00019b90, 0xf8)
void CBootyState::MoveLettersByDir() {
    if (m_initGate) {
        CWwdGameObjectA** q = m_sprintSprites;
        i32 n = 8;
        do {
            CGameObject* e = *q;
            q++;
            e->m_stateFlags |= SPRITE_STATE_HIDDEN;
        } while (--n);
        return;
    }
    i32 i = 0;
    CWwdGameObjectA** p = m_sprintSprites;
    for (; i < 8; i++, p++) {
        CGameObject* e = *p;
        i32 x = e->m_screenX;
        i32 y = e->m_screenY;
        if (x < 0 || x > SCREEN_W_PX || y < 0 || y > SCREEN_H_PX) {
            e->m_stateFlags |= SPRITE_STATE_HIDDEN;
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
            (*p)->m_screenX = x;
            (*p)->m_screenY = y;
        }
    }
}

#define STAT(getter, field)                                                                        \
    ((m_initOnce != 0 && g_gameReg->m_scoreHud->m_allDone != 0) ? g_gameReg->m_scoreHud->getter()  \
                                                                : g_gameReg->m_scoreHud->field)

DATA(0x0020b8b8)
Coord g_levelMsgIconPos[8] = {
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
#include <Gruntz/GlyphStringDraw.h>
#include <Mfc.h>
#include <Gruntz/GruntDirection.h>
#include <Wap32/ScreenGeometry.h>
#include <Gruntz/SpriteStateFlags.h>
// @early-stop
RVA(0x00019cd0, 0x200)
void CBootyState::GenMenuRandPos(GruntDirection sel, i32* outX, i32* outY) {
    if (!outX || !outY) {
        return;
    }
    // the coin flip is latched into a local; retail spills it to the (now dead)
    // outX parameter slot in each of the three arms that take it.
    i32 flip;
    switch (sel) {
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
            goto y_1e1;
        case DIR_WEST:
            *outX = SCREEN_W_PX;
            goto y_1e1;
        y_1e1:
            *outY = g_gameReg->Rand() % 0x1e1;
            return;
        case DIR_NORTHEAST:
            flip = g_gameReg->Rand() % 2;
            if (flip) {
                *outX = 0;
                goto y_f1;
            }
            *outX = g_gameReg->Rand() % 0x141;
            *outY = SCREEN_H_PX;
            return;
        case DIR_NORTHWEST:
            flip = g_gameReg->Rand() % 2;
            if (flip) {
                *outX = SCREEN_W_PX;
                goto y_f1;
            }
            *outX = g_gameReg->Rand() % 0x141 + SCREEN_HALF_W_PX;
            *outY = SCREEN_H_PX;
            return;
        y_f1:
            *outY = g_gameReg->Rand() % 0xf1 + SCREEN_HALF_H_PX;
            return;
        case DIR_SOUTHEAST:
            flip = g_gameReg->Rand() % 2;
            if (flip) {
                *outX = g_gameReg->RandRange(0, SCREEN_HALF_W_PX);
                *outY = 0;
                return;
            }
            *outX = 0;
            goto y_f0;
        case DIR_SOUTHWEST:
            if (g_gameReg->RandRange(0, 1)) {
                *outX = g_gameReg->RandRange(0, SCREEN_HALF_W_PX) + SCREEN_HALF_W_PX;
                *outY = 0;
                return;
            }
            *outX = SCREEN_W_PX;
            goto y_f0;
        y_f0:
            *outY = g_gameReg->RandRange(0, SCREEN_HALF_H_PX);
            return;
    }
}

// @early-stop
RVA(0x00019f50, 0xb2)
i32 CGruntzMgr::RandRange(i32 lo, i32 hi) {
    i32 span = hi - lo + 1;
    if (span == 0) {
        if ((GetRandomNumber() & 1)) {
            return lo;
        }
        return hi;
    }
    return lo + (GetRandomNumber()) % span;
}

// @early-stop
RVA(0x0001a040, 0x55e)
i32 CBootyState::LoadGruntEffectSprites() {
    CShadeTable* handleA = g_gameReg->m_spriteFactory->GetSel(0, 0);
    if (handleA == NULL) {
        return 0;
    }
    CShadeTable* handleB = g_gameReg->m_spriteFactory->GetSel(0, 1);

    CSymTab* img = m_gruntzBank->ResolvePath("IMAGEZ_GOKARTGRUNT");
    if (img == NULL) {
        return 0;
    }
    m_world->m_imageRegistry->InstallTree(img, "GRUNTZ_GOKARTGRUNT", "_");

    CDDrawChildGroup* f = g_gameReg->m_world->m_childGroup;

    CWwdGameObjectA* sw = f->CreateSprite(0, 0, 0, 0, "SimpleAnimation", 3);
    m_icons[0] = sw;
    if (sw == NULL) {
        return 0;
    }
    sw->ApplyName("GAME_INGAMEICONZ_POWERUPZ_STOPWATCH");
    m_icons[0]->ApplyLookupGeometry("GAME_CYCLE100", 0);
    m_icons[0]->m_stateFlags |= SPRITE_STATE_HIDDEN;

    CWwdGameObjectA* wh =
        g_gameReg->m_world->m_childGroup->CreateSprite(0, 0, 0, 0, "SimpleAnimation", 3);
    m_icons[7] = wh;
    if (wh == NULL) {
        return 0;
    }
    CLightFxMgr* pump = g_gameReg->m_logicPump;
    CShadeTable* tint = pump->m_tables[g_buteMgr.GetIntDef("Wormhole", "SecretColor", 1)];
    m_icons[7]->ApplyName("GAME_WORMHOLE");
    m_icons[7]->ApplyLookupGeometry("GAME_TELEPORTER", 0);
    m_icons[7]->m_stateFlags |= SPRITE_STATE_HIDDEN;
    CWwdGameObjectA* icon7 = m_icons[7];
    icon7->m_drawActive = 1;
    icon7->m_drawFillCmd = SHADE_DST_BY_SRC_16;
    icon7->m_drawFillArg = tint;

    CWwdGameObjectA* ex =
        g_gameReg->m_world->m_childGroup->CreateSprite(0, 0, 0, 0, "SimpleAnimation", 3);
    m_icons[1] = ex;
    if (ex == NULL) {
        return 0;
    }
    ex->ApplyName("GRUNTZ_EXITZ");
    m_icons[1]->ApplyLookupGeometry("GAME_GRUNTFLEX", 0);
    CWwdGameObjectA* icon1 = m_icons[1];
    icon1->m_drawActive = 1;
    icon1->m_drawFillCmd = SHADE_PAL_16;
    icon1->m_drawFillArg = handleA;
    m_icons[1]->m_stateFlags |= SPRITE_STATE_HIDDEN;

    CWwdGameObjectA* dt =
        g_gameReg->m_world->m_childGroup->CreateSprite(0, 0, 0, 0, "SimpleAnimation", 3);
    m_icons[2] = dt;
    if (dt == NULL) {
        return 0;
    }
    dt->ApplyName("GRUNTZ_NORMALGRUNT_DEATH");
    m_icons[2]->ApplyLookupGeometry("GAME_GRUNTTWITCH", 0);
    CWwdGameObjectA* icon2 = m_icons[2];
    icon2->m_drawActive = 1;
    icon2->m_drawFillCmd = SHADE_PAL_16;
    icon2->m_drawFillArg = handleA;
    m_icons[2]->m_stateFlags |= SPRITE_STATE_HIDDEN;

    CWwdGameObjectA* gl =
        g_gameReg->m_world->m_childGroup->CreateSprite(0, 0, 0, 0, "SimpleAnimation", 3);
    m_icons[3] = gl;
    if (gl == NULL) {
        return 0;
    }
    gl->ApplyName("GAME_INGAMEICONZ_TOOLZ_GAUNTLETZ");
    m_icons[3]->ApplyLookupGeometry("GAME_CYCLE100", 0);
    CWwdGameObjectA* icon3 = m_icons[3];
    icon3->m_drawActive = 1;
    icon3->m_drawFillCmd = SHADE_PAL_16;
    icon3->m_drawFillArg = handleA;
    m_icons[3]->m_stateFlags |= SPRITE_STATE_HIDDEN;

    CWwdGameObjectA* bb =
        g_gameReg->m_world->m_childGroup->CreateSprite(0, 0, 0, 0, "SimpleAnimation", 3);
    m_icons[4] = bb;
    if (bb == NULL) {
        return 0;
    }
    bb->ApplyName("GAME_INGAMEICONZ_TOYZ_BEACHBALLZ");
    m_icons[4]->ApplyLookupGeometry("GAME_CYCLE100", 0);
    CWwdGameObjectA* p30c = m_icons[4];
    p30c->m_drawActive = 1;
    p30c->m_drawFillCmd = SHADE_PAL_16;
    p30c->m_drawFillArg = handleA;
    m_icons[4]->m_stateFlags |= SPRITE_STATE_HIDDEN;

    CWwdGameObjectA* rz =
        g_gameReg->m_world->m_childGroup->CreateSprite(0, 0, 0, 0, "SimpleAnimation", 3);
    m_icons[5] = rz;
    if (rz == NULL) {
        return 0;
    }
    rz->ApplyName("GAME_INGAMEICONZ_POWERUPZ_ROIDZ");
    m_icons[5]->ApplyLookupGeometry("GAME_CYCLE100", 0);
    CWwdGameObjectA* icon5 = m_icons[5];
    icon5->m_drawActive = 1;
    icon5->m_drawFillCmd = SHADE_PAL_16;
    icon5->m_drawFillArg = handleA;
    m_icons[5]->m_stateFlags |= SPRITE_STATE_HIDDEN;

    CWwdGameObjectA* cn =
        g_gameReg->m_world->m_childGroup->CreateSprite(0, 0, 0, 0, "SimpleAnimation", 3);
    m_icons[6] = cn;
    if (cn == NULL) {
        return 0;
    }
    cn->ApplyName("GAME_INGAMEICONZ_POWERUPZ_COIN");
    m_icons[6]->ApplyLookupGeometry("GAME_CYCLE100", 0);
    CWwdGameObjectA* icon6 = m_icons[6];
    icon6->m_drawActive = 1;
    icon6->m_drawFillCmd = SHADE_PAL_16;
    icon6->m_drawFillArg = handleA;
    m_icons[6]->m_stateFlags |= SPRITE_STATE_HIDDEN;

    for (i32 i = 0; i < 8; i++) {
        CWwdGameObjectA* b =
            g_gameReg->m_world->m_childGroup->CreateSprite(0, 0, 0, 2, "SimpleAnimation", 3);
        m_bomb[i] = b;
        if (b == NULL) {
            return 0;
        }
        b->ApplyName("GRUNTZ_BOMBGRUNT_WEST_ITEM");
        m_bomb[i]->ApplyLookupGeometry("GAME_GRUNTBOMBSPRINT", 0);
        CWwdGameObjectA* bp = m_bomb[i];
        bp->m_drawActive = 1;
        bp->m_drawFillCmd = SHADE_PAL_16;
        bp->m_drawFillArg = handleA;
        m_bomb[i]->m_screenX = 0x2c6;
        m_bomb[i]->m_screenY = (g_levelMsgRectsB[i].top + g_levelMsgRectsB[i].bottom) / 2;
        m_bomb[i]->m_stateFlags |= SPRITE_STATE_HIDDEN;

        CWwdGameObjectA* e =
            g_gameReg->m_world->m_childGroup->CreateSprite(0, 0, 0, 2, "SimpleAnimation", 3);
        m_expl[i] = e;
        if (e == NULL) {
            return 0;
        }
        e->ApplyName("GAME_EXPLOSION");
        m_expl[i]->m_stateFlags |= SPRITE_STATE_HIDDEN;

        CWwdGameObjectA* g =
            g_gameReg->m_world->m_childGroup->CreateSprite(0, 0, 0, 2, "SimpleAnimation", 3);
        m_gokart[i] = g;
        if (g == NULL) {
            return 0;
        }
        g->ApplyName("GRUNTZ_GOKARTGRUNT_EAST");
        m_gokart[i]->ApplyLookupGeometry("GAME_CYCLE100", 0);
        CWwdGameObjectA* gp = m_gokart[i];
        gp->m_drawActive = 1;
        gp->m_drawFillCmd = SHADE_PAL_16;
        gp->m_drawFillArg = handleB;
        m_gokart[i]->m_screenX = -70;
        m_gokart[i]->m_screenY = (g_levelMsgRectsB[i].top + g_levelMsgRectsB[i].bottom) / 2;
        m_gokart[i]->m_stateFlags |= SPRITE_STATE_HIDDEN;
    }
    return 1;
}

// @early-stop
RVA(0x0001a700, 0x6b6)
i32 CBootyState::LevelMsgHudDriver() {
    if (m_initGate != 0) {

        if (m_slot == BOOTY_EXPLOSION_COUNT) {

            for (i32 i = 0; i < 8; i++) {
                CWwdGameObjectA* e = m_expl[i];
                if (IsAniCursorComplete(&e->m_animCursor)) {
                    e->m_stateFlags |= SPRITE_STATE_HIDDEN;
                }
            }
            return 1;
        }

        i32 shown = 0;
        for (i32 i = 0; i < 8; i++) {
            RECT box;
            m_bomb[i]->m_stateFlags |= SPRITE_STATE_HIDDEN;
            m_gokart[i]->m_stateFlags |= SPRITE_STATE_HIDDEN;
            m_icons[i]->m_stateFlags &= ~SPRITE_STATE_HIDDEN;
            m_icons[i]->m_screenX = g_levelMsgIconPos[i].m_x;
            m_icons[i]->m_screenY = g_levelMsgIconPos[i].m_y;
            CopyRect(&box, &g_levelMsgRectsA[i]);
            CString text = g_levelMsgStrings[i];
            m_templateFlags[i] = 1;
            ShowHudMessage(m_world, &text, &box, 0x78, 1, 0xff, 0xff, 0, 1);
            CopyRect(&box, &g_levelMsgRectsB[i]);
            this->FormatHudText(&text, static_cast<BootyStatRow>(i));
            m_readyFlags[i] = 1;
            ShowHudMessage(m_world, &text, &box, 0x78, 1, 0xff, 0xff, 0, 1);
            if (i >= m_slot && (i != m_slot || m_expl[i]->m_animCursor.m_animation == NULL)) {
                m_expl[i]->m_stateFlags &= ~SPRITE_STATE_HIDDEN;
                m_expl[i]->ApplyLookupGeometry("GAME_EXPLOSION1", 0);
                m_expl[i]->m_screenX = (g_levelMsgRectsB[i].right + g_levelMsgRectsB[i].left) / 2;
                m_expl[i]->m_screenY =
                    (g_levelMsgRectsB[i].bottom + g_levelMsgRectsB[i].top) / 2 - 0x10;
                if (shown == 0) {

                    CDDrawSubMgrLeafScan* host = g_gameReg->m_world->m_soundRegistry;
                    if (host->m_emitGate == 0) {
                        LeafCue* cue = NULL;
                        MapLookup(host->m_cues, "GAME_EXPLOSION1", cue);
                        if (cue != NULL) {
                            cue->PlayIfElapsed(g_sndCueTag, 0, 0, 0);
                        }
                    }
                    shown = 1;
                }
            }
        }
        m_slot = 8;
        return 1;
    }

    if (m_slot < 8) {
        if (m_slot == 0
            && (HAS(m_bomb[0]->m_stateFlags, SPRITE_STATE_HIDDEN)
                || HAS(m_gokart[0]->m_stateFlags, SPRITE_STATE_HIDDEN))) {
            m_bomb[0]->m_stateFlags &= ~SPRITE_STATE_HIDDEN;
            m_gokart[0]->m_stateFlags &= ~SPRITE_STATE_HIDDEN;
        }
        m_bomb[m_slot]->m_screenX -= 10;
        i32 gx = m_gokart[m_slot]->m_screenX + 10;
        m_gokart[m_slot]->m_screenX = gx;
        i32 s = m_slot;

        if (m_templateFlags[s] == 0
            && gx >= (g_levelMsgRectsA[s].right + g_levelMsgRectsA[s].left) / 2) {
            RECT box;
            m_templateFlags[s] = 1;
            CopyRect(&box, &g_levelMsgRectsA[m_slot]);
            CString text = g_levelMsgStrings[m_slot];
            m_templateFlags[m_slot] = 1;
            ShowHudMessage(m_world, &text, &box, 0x78, 1, 0xff, 0xff, 0, 1);
        }
        s = m_slot;
        if (m_readyFlags[s] == 0 && gx >= g_levelMsgIconPos[s].m_x) {
            m_readyFlags[s] = 1;
            m_icons[m_slot]->m_stateFlags &= ~SPRITE_STATE_HIDDEN;
            m_icons[m_slot]->m_screenX = g_levelMsgIconPos[m_slot].m_x;
            m_icons[m_slot]->m_screenY = g_levelMsgIconPos[m_slot].m_y;
        }
    }

    for (i32 j = 0; j < m_slot; j++) {
        CWwdGameObjectA* e = m_expl[j];
        if (IsAniCursorComplete(&e->m_animCursor)) {
            e->m_stateFlags |= SPRITE_STATE_HIDDEN;
        }
    }

    for (i32 i = m_slot; i < 8; i++) {
        if (m_gokart[i]->m_screenX >= m_bomb[i]->m_screenX) {
            RECT box;
            CString text;
            CopyRect(&box, &g_levelMsgRectsB[i]);
            this->FormatHudText(&text, static_cast<BootyStatRow>(i));
            m_readyFlags[i] = 1;
            ShowHudMessage(m_world, &text, &box, 0x78, 1, 0xff, 0xff, 0, 1);
            m_expl[i]->m_stateFlags &= ~SPRITE_STATE_HIDDEN;
            m_expl[i]->ApplyLookupGeometry("GAME_EXPLOSION1", 0);
            m_expl[i]->m_screenX = (g_levelMsgRectsB[i].left + g_levelMsgRectsB[i].right) / 2;
            m_expl[i]->m_screenY =
                (g_levelMsgRectsB[i].top + g_levelMsgRectsB[i].bottom) / 2 - 0x10;
            m_bomb[i]->m_stateFlags |= SPRITE_STATE_HIDDEN;
            m_gokart[i]->m_stateFlags |= SPRITE_STATE_HIDDEN;
            m_slot++;
            CDDrawSubMgrLeafScan* host = g_gameReg->m_world->m_soundRegistry;
            if (host->m_emitGate == 0) {
                LeafCue* found = NULL;
                MapLookup(host->m_cues, "GAME_EXPLOSION1", found);
                LeafCue* cue = found;
                if (cue != NULL) {
                    i32 gate = g_sndEnabled;
                    i32 item = g_sndCueTag;
                    if (gate != 0
                        && static_cast<u32>((g_killCueClock - cue->m_lastPlayTime))
                               >= static_cast<u32>(cue->m_replayDelay)) {
                        cue->m_lastPlayTime = g_killCueClock;
                        cue->m_sound->ConfigureItem(item, 0, 0, 0);
                    }
                }
            }
            if (m_slot >= 8) {
                return 1;
            }
            m_bomb[m_slot]->m_stateFlags &= ~SPRITE_STATE_HIDDEN;
            m_gokart[m_slot]->m_stateFlags &= ~SPRITE_STATE_HIDDEN;
        }
    }
    return 0;
}

RVA(0x0001af70, 0x3e0)
void CBootyState::FormatHudText(CString* buf, BootyStatRow sel) {
    switch (sel) {
        case BOOTYSTAT_TIME: {
            u32 secs = static_cast<u32>((STAT(SumElapsedTimeForGroup, m_elapsedTimeMs) / 1000));
            buf->Format("%d:%2.2d", secs / 60, secs % 60);
            return;
        }
        case BOOTYSTAT_GRUNTZ_EXITED:
            buf->Format("%d", STAT(SumGruntzExitedForGroup, m_gruntzExited));
            return;
        case BOOTYSTAT_GRUNTZ_LOST:
            buf->Format("%d", STAT(SumGruntzLostForGroup, m_gruntzLost));
            return;
        case BOOTYSTAT_TOOLZ: {
            i32 total = STAT(SumToolzAvailableForGroup, m_toolzAvailable);
            i32 cap = STAT(SumToolzAvailableForGroup, m_toolzAvailable);
            i32 cur = STAT(SumToolzCollectedForGroup, m_toolzCount);
            if (cur >= cap) {
                cur = cap;
            }
            buf->Format("%d of %d", cur, total);
            return;
        }
        case BOOTYSTAT_TOYZ: {
            i32 total = STAT(SumToyzAvailableForGroup, m_toyzAvailable);
            i32 cap = STAT(SumToyzAvailableForGroup, m_toyzAvailable);
            i32 cur = STAT(SumToyzCollectedForGroup, m_toyzCount);
            if (cur >= cap) {
                cur = cap;
            }
            buf->Format("%d of %d", cur, total);
            return;
        }
        case BOOTYSTAT_POWERUPZ: {
            i32 total = STAT(SumPowerupzAvailableForGroup, m_powerupzAvailable);
            i32 cap = STAT(SumPowerupzAvailableForGroup, m_powerupzAvailable);
            i32 cur = STAT(SumPowerupzCollectedForGroup, m_powerupCount);
            if (cur >= cap) {
                cur = cap;
            }
            buf->Format("%d of %d", cur, total);
            return;
        }
        case BOOTYSTAT_COINZ: {
            i32 total = STAT(SumCoinsAvailableForGroup, m_coinsAvailable);
            i32 cap = STAT(SumCoinsAvailableForGroup, m_coinsAvailable);
            i32 cur = STAT(SumCoinsCollectedForGroup, m_coinsCollected);
            if (cur >= cap) {
                cur = cap;
            }
            buf->Format("%d of %d", cur, total);
            return;
        }
        case BOOTYSTAT_SECRETZ: {
            i32 total = STAT(SumSecretsAvailableForGroup, m_secretsAvailable);
            i32 cap = STAT(SumSecretsAvailableForGroup, m_secretsAvailable);
            i32 cur = STAT(SumSecretsFoundForGroup, m_secretsFound);
            if (cur >= cap) {
                cur = cap;
            }
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
    if (g_gameReg->m_scoreHud->m_isCustomLevel != 0) {
        return 1;
    }
    if (g_gameReg->m_scoreHud->m_count > IDX(QUESTLEVEL_LAST)) {
        return 1;
    }
    CShadeTable* sel = g_gameReg->m_spriteFactory->GetSel(0, 0);
    if (sel == NULL) {
        return 0;
    }
    for (i32 i = 0; i < WARPLETTER_COUNT; i++) {
        m_animSprites[i] =
            g_gameReg->m_world->m_childGroup->CreateSprite(0, 0, 0, 1, "SimpleAnimation", 3);
        if (m_animSprites[i] == NULL) {
            return 0;
        }
        m_animSprites[i]->ApplyName("GRUNTZ_NORMALGRUNT_NORTH_WALK");
        m_animSprites[i]->ApplyLookupGeometry("GRUNTZ_NORMALGRUNT_WALK", 0);
        m_animSprites[i]->m_stateFlags |= SPRITE_STATE_HIDDEN;
        CWwdGameObjectA* anim = m_animSprites[i];
        anim->m_drawActive = 1;
        anim->m_drawFillCmd = SHADE_PAL_16;
        anim->m_drawFillArg = sel;
        m_visSprites[i] =
            g_gameReg->m_world->m_childGroup->CreateSprite(0, 0, 0, 1, "SimpleAnimation", 3);
        if (m_visSprites[i] == NULL) {
            return 0;
        }
        RVA_DYNINIT(0x0001b670, 0xa, buf)
        DATA(0x0022af0c)
        static CString buf;
        const char* prefix =
            (i < (g_gameReg->m_scoreHud->m_count - 1) % 4 + 1) ? "GAME_INGAMEICONZ_" : "BOOTY_DIM";
        buf.Format("%sSECRET%c", prefix, g_secretChars[i]);
        m_visSprites[i]->ApplyName(buf);
        m_visSprites[i]->ApplyLookupGeometry("GAME_CYCLE100", 0);
        m_visSprites[i]->m_screenX = g_idleSpriteIds[i] + 0xfa;
        m_visSprites[i]->m_screenY = 0xdc;
    }
    return 1;
}

// @early-stop
RVA(0x0001b690, 0x7e0)
i32 CBootyState::UpdateBootyWalkingGruntz() {
    CBattlezData* rec = g_gameReg->m_scoreHud;
    if (rec->m_isCustomLevel != 0) {
        return 1;
    }
    i32 n = rec->m_count;
    if (n > 0x24) {
        return 1;
    }
    if (m_stepIndex >= WARPLETTER_COUNT) {
        return 1;
    }

    if (m_initGate != 0) {

        if (n < 0x24) {
            for (i32 i = 0; i < WARPLETTER_COUNT; i++) {
                if (i <= (g_gameReg->m_scoreHud->m_count - 1) % 4) {
                    m_visSprites[i]->m_stateFlags |= SPRITE_STATE_HIDDEN;
                    m_animSprites[i]->m_screenX = g_idleSpriteIds[i];
                    m_animSprites[i]->m_screenY = 0xdc;
                    m_animSprites[i]->m_stateFlags &= ~SPRITE_STATE_HIDDEN;
                    if ((g_gameReg->m_scoreHud)->GetRecordValue(i) == 0) {
                        m_animSprites[i]->ApplyName("GRUNTZ_NORMALGRUNT_SOUTH_IDLE");
                        m_animSprites[i]->ApplyLookupGeometry("GRUNTZ_NORMALGRUNT_IDLE4", 0);
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
                        m_animSprites[i]->ApplyName("GRUNTZ_PICKUPS");
                        m_animSprites[i]->ApplyLookupGeometry("GRUNTZ_PICKUPS_" + letter, 0);
                    }
                } else {
                    m_visSprites[i]->m_screenX = g_idleSpriteIds[i];
                    m_visSprites[i]->m_screenY = 0xdc;
                    m_visSprites[i]->m_stateFlags &= ~SPRITE_STATE_HIDDEN;
                    m_animSprites[i]->m_stateFlags |= SPRITE_STATE_HIDDEN;
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
    if (m_stepIndex == 0 && HAS(m_animSprites[0]->m_stateFlags, SPRITE_STATE_HIDDEN)) {
        m_animSprites[0]->m_stateFlags &= ~SPRITE_STATE_HIDDEN;
        m_animSprites[0]->m_screenX = g_idleSpriteIds[0];
        m_animSprites[0]->m_screenY = 0x1f4;
    }

    if (m_soundStarted == 0 && m_animSprites[m_stepIndex]->m_screenY <= 0x195) {
        if ((g_gameReg->m_scoreHud)->GetRecordValue(m_stepIndex) == 0) {
            m_soundStarted = 1;
            CDDrawSubMgrLeafScan* ss = g_gameReg->m_world->m_soundRegistry;
            if (ss->m_emitGate == 0) {
                LeafCue* res = 0;
                MapLookup(ss->m_cues, "GRUNTZ_WANDGRUNT_WANDZGRUNTUI1D", res);
                if (res != NULL) {
                    res->PlayIfElapsed(g_sndCueTag, 0, 0, 0);
                }
            }
        }
    }

    if (m_soundStarted != 0) {
        CDDrawSubMgrLeafScan* ss = g_gameReg->m_world->m_soundRegistry;
        LeafCue* res = 0;
        MapLookup(ss->m_cues, "GRUNTZ_WANDGRUNT_WANDZGRUNTUI1D", res);
        if (res == NULL) {
            return 1;
        }
        if (res->m_sound->IsPlaying() != 0) {
            m_visSprites[m_stepIndex]->m_stateFlags ^= SPRITE_STATE_HIDDEN;
        } else {
            m_visSprites[m_stepIndex]->m_stateFlags |= SPRITE_STATE_HIDDEN;
        }
    }

    if (m_walkStarted == 0 && m_animSprites[m_stepIndex]->m_screenY <= 0xdc) {
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
            CShadeTable* sel = g_gameReg->m_spriteFactory->GetSel(0, 0);
            if (sel != NULL) {
                if ((g_gameReg->m_scoreHud)->GetRecordValue(m_stepIndex) != 0) {
                    CDDrawSubMgrLeafScan* ss = g_gameReg->m_world->m_soundRegistry;
                    if (ss->m_emitGate == 0) {
                        LeafCue* res = NULL;
                        MapLookup(ss->m_cues, "GAME_FLAGRISE", res);
                        if (res != NULL) {
                            PlayLeafCueIfElapsed(res, g_sndCueTag, 0, 0, 0);
                        }
                    }
                    m_animSprites[m_stepIndex]->ApplyName("GRUNTZ_PICKUPS");
                    m_animSprites[m_stepIndex]->ApplyLookupGeometry("GRUNTZ_PICKUPS_" + letter, 0);
                    CWwdGameObjectA* g = m_animSprites[m_stepIndex];
                    g->m_drawActive = 1;
                    g->m_drawFillCmd = SHADE_PAL_16;
                    g->m_drawFillArg = sel;
                    m_visSprites[m_stepIndex]->m_stateFlags |= SPRITE_STATE_HIDDEN;
                    g_gameReg->m_cueSink
                        ->SpawnVoiceDriver(0, 0x3bf, GetRandomNumber() % 0x11, 1, -1, -1);
                    m_walkStarted = 1;
                } else {
                    m_animSprites[m_stepIndex]->ApplyName("GRUNTZ_NORMALGRUNT_SOUTH_IDLE");
                    m_animSprites[m_stepIndex]->ApplyLookupGeometry("GRUNTZ_NORMALGRUNT_IDLE4", 0);
                    CWwdGameObjectA* g = m_animSprites[m_stepIndex];
                    g->m_drawActive = 1;
                    g->m_drawFillCmd = SHADE_PAL_16;
                    g->m_drawFillArg = sel;
                    m_visSprites[m_stepIndex]->m_stateFlags |= SPRITE_STATE_HIDDEN;
                    m_stepIndex++;
                    g_gameReg->m_cueSink->SpawnVoiceDriver(0, 0x441, 0, 1, -1, -1);
                    if (m_stepIndex == g_gameReg->m_scoreHud->m_count % 4) {
                        m_stepIndex = 4;
                        return 1;
                    }
                    if (m_stepIndex < 4) {
                        m_animSprites[m_stepIndex]->m_stateFlags &= ~SPRITE_STATE_HIDDEN;
                        m_animSprites[m_stepIndex]->m_screenX = g_idleSpriteIds[m_stepIndex];
                        m_animSprites[m_stepIndex]->m_screenY = 0x1f4;
                        m_soundStarted = 0;
                        m_walkStarted = 0;
                    }
                }
            }
        }
    } else if (m_walkStarted != 0) {

        CWwdGameObjectA* spr = m_animSprites[m_stepIndex];
        if (IsAniCursorComplete(&spr->m_animCursor)) {
            m_stepIndex++;
            if (m_stepIndex == g_gameReg->m_scoreHud->m_count % 4) {
                m_stepIndex = 4;
                return 1;
            }
            if (m_stepIndex < 4) {
                m_animSprites[m_stepIndex]->m_stateFlags &= ~SPRITE_STATE_HIDDEN;
                m_animSprites[m_stepIndex]->m_screenX = g_idleSpriteIds[m_stepIndex];
                m_animSprites[m_stepIndex]->m_screenY = 0x1f4;
                m_walkStarted = 0;
                m_soundStarted = 0;
            }
        }
    } else {
        m_animSprites[m_stepIndex]->m_screenY -= 3;
    }
    return 0;
}

RVA(0x0001c070, 0x59)
i32 CBootyState::BuildBootyPerfectAnimation() {
    CWwdGameObjectA* spr =
        g_gameReg->m_world->m_childGroup
            ->CreateSprite(0, static_cast<i32>(0xffffff7e), 0xf0, 0x64, "SimpleAnimation", 3);
    m_bootyPerfectSprite = spr;
    if (!spr) {
        return 0;
    }
    spr->ApplyName("BOOTY_PERFECT");
    m_bootyPerfectSprite->ApplyLookupGeometry("GAME_CYCLE100", 0);
    return 1;
}

RVA(0x0001c0f0, 0xd5)
i32 CBootyState::CheckPerfectBonus() {
    if (!g_gameReg->m_scoreHud->InBounds(-1)) {
        return 1;
    }
    CWwdGameObjectA* st = m_bootyPerfectSprite;
    i32 phase = st->m_screenX;
    if (phase == static_cast<i32>(0xffffff7e)) {
        CDDrawSurfaceMgr* host = g_gameReg->m_world;
        i32 item = g_gameReg->m_soundVolume;
        CDDrawSubMgrLeafScan* m28 = host->m_soundRegistry;
        if (m28->m_emitGate == 0) {
            LeafCue* found = NULL;
            MapLookup(m28->m_cues, "BOOTY_PERFECT", found);
            if (found) {
                PlayLeafCueIfElapsed(found, item, 0, 0, 0);
            }
        }
    }
    if (phase >= 0x302) {
        m_bootyPerfectSprite->m_flags |= 0x10000;
        return 1;
    }
    m_bootyPerfectSprite->m_screenX = phase + 0xa;
    return 1;
}

// @early-stop
RVA(0x0001c210, 0x540)
i32 CBootyState::Render() {
    IDirectDrawSurface* frameSurf = m_world->m_drawTarget->m_frontPair->m_surface->m_ddSurface;
    if (frameSurf == NULL || frameSurf->IsLost() != 0) {
        if (InputVirtual() == 0) {
            m_mgr->ReportError(IDX(IDS_RESTORE_GAME), 0x459);
            return 0;
        }
    }
    SoundStream* snd = m_world->m_soundStream;
    if (snd != NULL) {
        i32 now = static_cast<i32>(timeGetTime());
        snd->PurgeVoiceList(now);
        snd->TickSubManagers(now);
    }

    i64 elapsed = static_cast<i64>(g_frameTime) - m_frameStamp64;
    if (elapsed < m_frameInterval64) {
        return 0;
    }
    m_frameIntervalLo = 0x21;
    m_frameIntervalHi = 0;
    m_frameStampLo = g_frameTime;
    m_frameStampHi = 0;

    switch (m_activation) {
        case BOOTYSEQ_WARP_CUE: {
            m_activation = BOOTYSEQ_GLITTER;
            CDDrawSubMgrLeafScan* set = g_gameReg->m_world->m_soundRegistry;
            if (set->m_emitGate == 0) {
                LeafCue* cue = 0;
                MapLookup(set->m_cues, "BOOTY_WARP", cue);
                if (cue != NULL) {
                    cue->PlayIfElapsed(g_sndCueTag, 0, 0, 0);
                }
            }
        }
            // FALL THROUGH

        case BOOTYSEQ_GLITTER: {
            if (StepGlitterAnim() == 0) {
                break;
            }
            m_activation = BOOTYSEQ_LETTERS;
            CDDrawSubMgrLeafScan* set = g_gameReg->m_world->m_soundRegistry;
            if (set->m_emitGate == 0) {
                LeafCue* cue = 0;
                MapLookup(set->m_cues, "BOOTY_BOOM", cue);
                if (cue != NULL) {
                    cue->PlayIfElapsed(g_sndCueTag, 0, 0, 0);
                }
            }
            if (m_initOnce != 0 && g_gameReg->m_scoreHud->m_allDone != 0 && g_levelBias100 == 0) {
                RECT rc;
                rc.left = 0;
                rc.top = 0x24;
                rc.right = 0x1ea;
                rc.bottom = 0x64;
                CString s("World Completed!");
                m_levelCompleteGate = 1;
                ShowHudMessage(m_world, &s, &rc, 0x82, 1, 0xff, 0xff, 0, 1);
            } else {
                RECT rc;
                rc.left = 0;
                rc.top = 0x24;
                rc.right = 0x1ea;
                rc.bottom = 0x64;
                CString s("Level Completed!");
                m_levelCompleteGate = 1;
                ShowHudMessage(m_world, &s, &rc, 0x82, 1, 0xff, 0xff, 0, 1);
            }
        }
        // FALL THROUGH
        case BOOTYSEQ_LETTERS:
            MoveLettersByDir();
            if (LevelMsgHudDriver() == 0) {
                break;
            }
            m_activation = BOOTYSEQ_WALK;
        // FALL THROUGH
        case BOOTYSEQ_WALK:
            LevelMsgHudDriver();
            if (UpdateBootyWalkingGruntz() == 0) {
                break;
            }
            m_activation = BOOTYSEQ_PERFECT_BONUS;
            break;
        case BOOTYSEQ_PERFECT_BONUS: {
            LevelMsgHudDriver();
            UpdateBootyWalkingGruntz();
            CheckPerfectBonus();
            if (m_secretHudHandled == 0 && g_gameReg->m_scoreHud->m_isCustomLevel == 0) {
                CString s;
                RECT rc;
                CBattlezData* hud = g_gameReg->m_scoreHud;
                if (hud->m_count > IDX(QUESTLEVEL_LAST)) {

                    if (hud->m_allDone != 0) {
                        s = "You have completed training! Now, grab the pebble from my hand.";
                    } else {
                        s = "You are closer to achieving mastery! Keep training!";
                    }
                    SetRect(&rc, 0x194, 0xaa, 0x263, SCREEN_H_PX);
                } else {
                    if (hud->m_allDone != 0) {
                        if (hud->GroupAllScored()) {
                            s.Format(
                                "WARP letterz recovered! Prepare to receive your cheat codez!"
                            );
                        } else {
                            s = "WARP letterz not recovered! No cheatz for you.";
                        }
                    } else if (hud->m_scoreValue != 0) {
                        s = "Keep finding those WARP letterz!";
                    } else {
                        s = "Collect all four WARP letterz to receive secret bonus!";
                    }
                    SetRect(&rc, 0x194, 0xe6, 0x263, SCREEN_H_PX);
                }
                m_secretGate = 1;
                ShowHudMessage(m_world, &s, &rc, 0x6e, 1, 0xff, 0xff, 0, 1);
                m_secretHudHandled = 1;
            } else if (g_gameReg->m_scoreHud->m_isCustomLevel != 0) {
                m_secretHudHandled = 1;
            }
            break;
        }
        case BOOTYSEQ_DONE:
            return 1;
    }

    m_world->m_childGroup->TickKillCues(1);
    m_world->m_childGroup->RenderChildren(m_world->m_drawTarget->m_backPair);
    CDDrawSubMgrPages* dt = m_world->m_drawTarget;
    FlipFrontAndRestoreOverlay(dt);
    PurgeVoices(m_world->m_soundRegistry);
    return 1;
}

RVA(0x0001c8a0, 0xec)
i32 CBootyState::InputVirtual() {
    if (CState::InputVirtual() == 0) {
        return 0;
    }
    int(WINAPI * sc)(BOOL) = ShowCursor;
    i32 r = sc(0);
    while (r >= 0) {
        r = sc(0);
    }
    CSymTab* booty = SymTab2c()->ResolvePath("IMAGEZ");
    if (booty == NULL) {
        return 0;
    }
    if (m_world->m_imageRegistry->LoadNamespace(booty, "BOOTY", "_") == -1) {
        return 0;
    }
    CSymTab* gruntz = m_gruntzBank->ResolvePath("IMAGEZ");
    if (gruntz == NULL) {
        return 0;
    }
    if (m_world->m_imageRegistry->LoadNamespace(gruntz, "GRUNTZ", "_") == -1) {
        return 0;
    }
    if (m_activation != BOOTYSEQ_DONE) {
        if (FadeInTitle("bg", 0, 0, 0, 0, 1) == 0) {
            return 0;
        }
        ShowLevelCompleteMessage();
    } else {
        ShowSecretBonusMessage();
    }
    m_world->m_drawTarget->TransExit();
    RetireScene(0x50, 0x3e8, 0, 1);
    return 1;
}

// @early-stop
// The eight-element g_levelMsgStrings walk relocates its one-past bound at the
// following static guard byte; the address agrees and only the delinked symbol
// name differs.
RVA(0x0001c9d0, 0x351)
void CBootyState::ShowLevelCompleteMessage() {
    for (i32 i = 0; i < 8; i++) {
        if (m_templateFlags[i]) {
            RECT r1;
            CopyRect(&r1, &g_levelMsgRectsA[i]);
            CString t(g_levelMsgStrings[i]);
            ShowHudMessage(m_world, &t, &r1, 0x78, 1, 0xff, 0xff, 0, 1);
        }
        if (m_readyFlags[i]) {
            RECT r2;
            CopyRect(&r2, &g_levelMsgRectsB[i]);
            CString t2;
            FormatHudText(&t2, static_cast<BootyStatRow>(i));
            ShowHudMessage(m_world, &t2, &r2, 0x78, 1, 0xff, 0xff, 0, 1);
        }
    }

    if (m_levelCompleteGate) {
        if (g_gameReg->m_scoreHud->m_allDone != 0) {
            RECT r = {0, 0x24, 0x1ea, 0x64};
            CString s("World Completed!");
            ShowHudMessage(m_world, &s, &r, 0x82, 1, 0xff, 0xff, 0, 1);
        } else {
            RECT r = {0, 0x24, 0x1ea, 0x64};
            CString s("Level Completed!");
            ShowHudMessage(m_world, &s, &r, 0x82, 1, 0xff, 0xff, 0, 1);
        }
    }

    if (g_gameReg->m_scoreHud->m_isCustomLevel == 0 && m_secretGate != 0) {
        CString s;
        RECT r;
        CBattlezData* rec = g_gameReg->m_scoreHud;
        if (rec->m_count > IDX(QUESTLEVEL_LAST)) {
            if (rec->m_allDone != 0) {
                s = "You have completed training! Now, grab the pebble from my hand.";
            } else {
                s = "You are closer to achieving mastery! Keep training!";
            }
            SetRect(&r, 0x194, 0xaa, 0x263, SCREEN_H_PX);
        } else {
            if (rec->m_allDone != 0) {
                if ((rec)->GroupAllScored()) {
                    s.Format("WARP letterz recovered! Prepare to receive your cheat codez!");
                } else {
                    s = "WARP letterz not recovered! No cheatz for you.";
                }
            } else {
                if (rec->m_scoreValue != 0) {
                    s = "Keep finding those WARP letterz!";
                } else {
                    s = "Collect all four WARP letterz to receive secret bonus!";
                }
            }
            SetRect(&r, 0x194, 0xe6, 0x263, SCREEN_H_PX);
        }
        ShowHudMessage(m_world, &s, &r, 0x6e, 1, 0xff, 0xff, 0, 1);
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
// One scheduling slot in the m_initOnce dispatch; both shape bugs (the separate second
// `if`, and `<` not `!=` on the letter-coords walk) are settled.
RVA(0x0001ce60, 0x460)
i32 CBootyState::BuildBootyGruntIdleAnimation() {
    BootySeqPhase state = m_activation;
    if (state != BOOTYSEQ_PERFECT_BONUS && state != BOOTYSEQ_DONE) {
        m_initGate = 1;
        return 1;
    }
    CBattlezData* rec = g_gameReg->m_scoreHud;
    if (rec->m_isCustomLevel != 0) {
        PostMessageA(g_gameReg->m_gameWnd->m_hwnd, WM_COMMAND, IDX(CMD_MAIN_MENU), 0);
    } else {
        if (m_initOnce == 0) {
            if (rec->m_allDone != 0) {
                m_initOnce = 1;
                CDDrawSubMgrLeafScan* ss = g_gameReg->m_world->m_soundRegistry;
                if (ss->m_emitGate == 0) {
                    LeafCue* res = 0;
                    MapLookup(ss->m_cues, "GRUNTZ_WANDGRUNT_WANDZGRUNTI3A", res);
                    if (res != NULL) {
                        res->PlayIfElapsed(g_sndCueTag, 0, 0, 0);
                    }
                }
                if (g_gameReg->m_scoreHud->m_count < 0x24) {
                    for (i32 p = 0; p < 4; p++) {
                        m_visSprites[p]->m_stateFlags |= SPRITE_STATE_HIDDEN;
                        m_animSprites[p]->m_screenX = g_idleSpriteIds[p];
                        m_animSprites[p]->m_screenY = 0xdc;
                        m_animSprites[p]->m_stateFlags &= ~SPRITE_STATE_HIDDEN;
                        if ((g_gameReg->m_scoreHud)->GetRecordValue(p) == 0) {
                            m_animSprites[p]->ApplyName("GRUNTZ_NORMALGRUNT_SOUTH_IDLE");
                            m_animSprites[p]->ApplyLookupGeometry("GRUNTZ_NORMALGRUNT_IDLE4", 0);
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
                            m_animSprites[p]->ApplyName("GRUNTZ_PICKUPS");
                            m_animSprites[p]->ApplyLookupGeometry("GRUNTZ_PICKUPS_" + letter, 0);
                        }
                    }
                }
                // The bound is a SIGNED int compare (`jl`), so the loop counts the table
                // index rather than comparing pointers.
                CWwdGameObjectA** ap = m_trailSprites;
                for (i32 k = 0; k < 4; k++) {
                    (*ap)->m_screenX = g_bootyLetterCoords[k].m_x;
                    (*ap)->m_screenY = g_bootyLetterCoords[k].m_y;
                    (*ap)->m_stateFlags &= ~SPRITE_STATE_HIDDEN;
                    ap++;
                }
                if (!FadeInTitle("bg", 0, 0, 0, 0, 1)) {
                    return 0;
                }
                ShowLevelCompleteMessage();
                m_world->m_drawTarget->TransExit();
                m_world->m_childGroup->RenderChildren(m_world->m_drawTarget->m_backPair);
                m_world->m_drawTarget->TransTitle();
                RetireScene(0x50, 0x3e8, 0, 1);
                if (!FadeInTitle("bg", 0, 0, 0, 0, 1)) {
                    return 0;
                }
                ShowLevelCompleteMessage();
                return 1;
            }
        }
        if (m_initOnce != 0 && rec->m_allDone != 0 && rec->m_count < IDX(QUESTLEVEL_LAST)
            && state == BOOTYSEQ_PERFECT_BONUS) {
            if ((rec)->GroupAllScored()) {
                if (!ShowSecretBonusMessage()) {
                    return 0;
                }
                m_world->m_drawTarget->TransExit();
                RetireScene(0x50, 0x3e8, 0, 1);
                m_activation = BOOTYSEQ_SECRET_PENDING;
                return 1;
            }
        }

        if (m_activation == BOOTYSEQ_SECRET_PENDING && (g_gameReg->m_scoreHud)->AllRecordsInBounds()
            && m_secretBannerOnce == 0) {
            m_secretBannerOnce = 1;
            if (!ShowSecretBonusMessage()) {
                return 0;
            }
            m_world->m_drawTarget->TransExit();
            RetireScene(0x50, 0x3e8, 0, 1);
            return 1;
        }

        CBattlezData* rec2 = g_gameReg->m_scoreHud;
        if (rec2->m_count == IDX(QUESTLEVEL_CAMPAIGN_LAST)) {
            SoundStream* sub = m_world->m_soundRegistry->m_soundStream;
            if (sub != NULL) {
                sub->Stop();
            }
            g_gameReg->ChangeState(3);
            PostMessageA(g_gameReg->m_gameWnd->m_hwnd, WM_COMMAND, IDX(CMD_SHOW_HELP), 0);
        } else {

            g_gameReg->PassClickToPlayState((rec2->m_count % 0x28) + 1, 0, 1);
        }
    }
    return 1;
}

RVA(0x0001d3e0, 0x8)
i32 CBootyState::OnLButtonDown(i32, i32, i32) {
    return BuildBootyGruntIdleAnimation();
}

RVA(0x0001d400, 0x8)
i32 CBootyState::OnRButtonDown(i32, i32, i32) {
    return BuildBootyGruntIdleAnimation();
}

RVA(0x0001d420, 0x8)
i32 CBootyState::OnKeyDown(i32, i32) {
    return BuildBootyGruntIdleAnimation();
}

// @early-stop
// The 42-branch, 74-block CFG and referent multiset agree. Retail spills `tint`,
// `best`, `bestIdx` and the tally cursor
// where cl enregisters them - it burns ebx on the constant 1 and keeps `this` in ebp,
// cl does the opposite - so retail's frame is 0x14 wider and the registers rotate.
RVA(0x0001d440, 0xd7d)
i32 CMultiBootyState::LoadGameAssetNamespaces(CGruntzMgr* mgr, i32 areaArg, i32 prevStateId) {
    if (!CState::LoadGameAssetNamespaces(mgr, areaArg, prevStateId)) {
        return 0;
    }
    m_mgr->RestoreVideoMode(0);
    m_stateBank = m_symParser->ResolvePath("STATEZ_BOOTY");
    if (!m_stateBank) {
        return 0;
    }
    m_gameBank = m_symParser->ResolvePath("GAME");
    if (!m_gameBank) {
        return 0;
    }
    m_gruntzBank = m_symParser->ResolvePath("GRUNTZ");
    if (!m_gruntzBank) {
        return 0;
    }
    {
        char area[128];
        sprintf(area, "AREA%i", (g_gameReg->m_scoreHud->m_count - 1) % 0x24 / 4 + 1);
        m_levelBank = m_symParser->ResolvePath(area);
    }
    if (!m_levelBank) {
        return 0;
    }
    m_world->m_childGroup->ClearChildren();
    {
        CSymTab* soundz = m_stateBank->FindSub("SOUNDZ");
        if (!soundz) {
            return 0;
        }
        m_world->m_soundRegistry->ScanTree(static_cast<CSymTab*>(soundz), "BOOTY", "_");
    }
    {
        int(WINAPI * sc)(BOOL) = ShowCursor;
        while (sc(0) >= 0) {
        }
    }
    m_mgr->m_gameWnd->PumpMessages(0x100, 0x40);

    m_reserved1b4 = 0;
    for (i32 i = 0; i < 4; i++) {
        if (g_gameReg->m_options[i].m_joined == 0) {
            continue;
        }
        CShadeTable* tint =
            g_gameReg->m_spriteFactory->GetSel(IDX(g_gameReg->m_options[i].m_colorIndex), 0);
        if (tint == NULL) {
            return 0;
        }
        CString key;

        m_puddleSprites[i] =
            g_gameReg->m_world->m_childGroup->CreateSprite(0, 0, 0, 0, "SimpleAnimation", 3);
        if (m_puddleSprites[i] == NULL) {
            return 0;
        }
        m_puddleSprites[i]->ApplyName("GRUNTZ_GRUNTPUDDLE");
        m_puddleSprites[i]->ApplyLookupGeometry(g_puddleSpriteKey, 0);
        {
            CWwdGameObjectA* o = m_puddleSprites[i];
            o->m_drawActive = 1;
            o->m_drawFillCmd = SHADE_PAL_16;
            o->m_drawFillArg = tint;
        }
        m_puddleSprites[i]->m_stateFlags |= SPRITE_STATE_HIDDEN;

        if (i == QueryGruntSlots()) {
            m_gruntSprites[i] =
                g_gameReg->m_world->m_childGroup->CreateSprite(0, 0, 0, 0, "SimpleAnimation", 3);
            if (m_gruntSprites[i] == NULL) {
                return 0;
            }
            m_gruntSprites[i]->ApplyName("GRUNTZ_EXITZ");
            m_gruntSprites[i]->ApplyLookupGeometry("GAME_GRUNTFLEX", 0);
            m_gruntSprites[i]->m_drawActive = 1;
            m_gruntSprites[i]->m_drawFillCmd = SHADE_PAL_16;
            m_gruntSprites[i]->m_drawFillArg = tint;
        } else {
            key.Format("GRUNTZ_NORMALGRUNT_IDLE%d", (g_gameReg->Rand() % 2 != 0) ? 1 : 4);
            m_gruntSprites[i] =
                g_gameReg->m_world->m_childGroup->CreateSprite(0, 0, 0, 0, "SimpleAnimation", 3);
            if (m_gruntSprites[i] == NULL) {
                return 0;
            }
            m_gruntSprites[i]->ApplyName("GRUNTZ_NORMALGRUNT_SOUTH_IDLE");
            m_gruntSprites[i]->ApplyLookupGeometry(key, 0);
            CWwdGameObjectA* o = m_gruntSprites[i];
            o->m_drawActive = 1;
            o->m_drawFillCmd = SHADE_PAL_16;
            o->m_drawFillArg = tint;
        }
        m_gruntSprites[i]->m_stateFlags |= SPRITE_STATE_HIDDEN;

        {
            i32 best = -1;
            i32 bestIdx = 0;
            const i32* tally = &g_gameReg->m_scoreHud->m_weaponPickupz[i * 22];
            for (i32 j = 0; j < 22; j++) {
                if (tally[j] > best) {
                    best = tally[j];
                    bestIdx = j;
                }
            }
            BuildPowerupIconKeys(&key, bestIdx + 1);
        }
        m_weaponIcons[i] =
            g_gameReg->m_world->m_childGroup->CreateSprite(0, 0, 0, 0, "SimpleAnimation", 3);
        if (m_weaponIcons[i] == NULL) {
            return 0;
        }
        m_weaponIcons[i]->ApplyName(key);
        m_weaponIcons[i]->ApplyLookupGeometry("GAME_CYCLE100", 0);
        {
            CWwdGameObjectA* o = m_weaponIcons[i];
            o->m_drawActive = 1;
            o->m_drawFillCmd = SHADE_PAL_16;
            o->m_drawFillArg = tint;
        }
        m_weaponIcons[i]->m_stateFlags |= SPRITE_STATE_HIDDEN;

        {
            CShadeTable* iconTint = g_gameReg->m_spriteFactory->GetSel(0x10, 0);
            if (iconTint == NULL) {
                return 0;
            }
            {
                i32 best = -1;
                i32 bestIdx = 0;
                const i32* tally = &g_gameReg->m_scoreHud->m_toyPickupz[i * 10];
                for (i32 j = 0; j < 10; j++) {
                    if (tally[j] > best) {
                        best = tally[j];
                        bestIdx = j;
                    }
                }
                BuildPowerupIconKeys(&key, bestIdx + 0x17);
            }
            m_toyIcons[i] =
                g_gameReg->m_world->m_childGroup->CreateSprite(0, 0, 0, 0, "SimpleAnimation", 3);
            if (m_toyIcons[i] == NULL) {
                return 0;
            }
            m_toyIcons[i]->ApplyName(key);
            m_toyIcons[i]->ApplyLookupGeometry("GAME_CYCLE100", 0);
            {
                CWwdGameObjectA* o = m_toyIcons[i];
                o->m_drawActive = 1;
                o->m_drawFillCmd = SHADE_PAL_16;
                o->m_drawFillArg = iconTint;
            }
            m_toyIcons[i]->m_stateFlags |= SPRITE_STATE_HIDDEN;

            {
                i32 best = -1;
                i32 bestIdx = 0;
                const i32* tally = &g_gameReg->m_scoreHud->m_powerupPickupz[i * 7];
                for (i32 j = 0; j < 7; j++) {
                    if (tally[j] > best) {
                        best = tally[j];
                        bestIdx = j;
                    }
                }
                BuildPowerupIconKeys(&key, bestIdx + 0x36);
            }
            m_powerupIcons[i] =
                g_gameReg->m_world->m_childGroup->CreateSprite(0, 0, 0, 0, "SimpleAnimation", 3);
            if (m_powerupIcons[i] == NULL) {
                return 0;
            }
            m_powerupIcons[i]->ApplyName(key);
            m_powerupIcons[i]->ApplyLookupGeometry("GAME_CYCLE100", 0);
            {
                CWwdGameObjectA* o = m_powerupIcons[i];
                o->m_drawActive = 1;
                o->m_drawFillCmd = SHADE_PAL_16;
                o->m_drawFillArg = iconTint;
            }
            m_powerupIcons[i]->m_stateFlags |= SPRITE_STATE_HIDDEN;

            {
                i32 best = -1;
                i32 bestIdx = 0;
                const i32* tally = &g_gameReg->m_scoreHud->m_miscPickupz[i * 4];
                for (i32 j = 0; j < 4; j++) {
                    if (tally[j] > best) {
                        best = tally[j];
                        bestIdx = j;
                    }
                }
                BuildPowerupIconKeys(&key, bestIdx + 0x3d);
            }
            m_miscIcons[i] =
                g_gameReg->m_world->m_childGroup->CreateSprite(0, 0, 0, 0, "SimpleAnimation", 3);
            if (m_miscIcons[i] == NULL) {
                return 0;
            }
            m_miscIcons[i]->ApplyName(key);
            m_miscIcons[i]->ApplyLookupGeometry("GAME_CYCLE100", 0);
            {
                CWwdGameObjectA* o = m_miscIcons[i];
                o->m_drawActive = 1;
                o->m_drawFillCmd = SHADE_PAL_16;
                o->m_drawFillArg = iconTint;
            }
            m_miscIcons[i]->m_stateFlags |= SPRITE_STATE_HIDDEN;
        }

        m_puddleSprites[i]->m_screenX = g_bootyPuddlePos[i].m_x;
        m_puddleSprites[i]->m_screenY = g_bootyPuddlePos[i].m_y;
        m_puddleSprites[i]->m_stateFlags &= ~SPRITE_STATE_HIDDEN;
        m_gruntSprites[i]->m_screenX = g_bootyGruntPos[i].m_x;
        m_gruntSprites[i]->m_screenY = g_bootyGruntPos[i].m_y;
        m_gruntSprites[i]->m_stateFlags &= ~SPRITE_STATE_HIDDEN;
        m_weaponIcons[i]->m_screenX = g_bootyWeaponPos[i].m_x;
        m_weaponIcons[i]->m_screenY = g_bootyWeaponPos[i].m_y;
        m_weaponIcons[i]->m_stateFlags &= ~SPRITE_STATE_HIDDEN;
        m_toyIcons[i]->m_screenX = g_bootyToyPos[i].m_x;
        m_toyIcons[i]->m_screenY = g_bootyToyPos[i].m_y;
        m_toyIcons[i]->m_stateFlags &= ~SPRITE_STATE_HIDDEN;
        m_powerupIcons[i]->m_screenX = g_bootyPowerupPos[i].m_x;
        m_powerupIcons[i]->m_screenY = g_bootyPowerupPos[i].m_y;
        m_powerupIcons[i]->m_stateFlags &= ~SPRITE_STATE_HIDDEN;
        m_miscIcons[i]->m_screenX = g_bootyMiscPos[i].m_x;
        m_miscIcons[i]->m_screenY = g_bootyMiscPos[i].m_y;
        m_miscIcons[i]->m_stateFlags &= ~SPRITE_STATE_HIDDEN;
    }

    for (i32 t = 0; t < 4; t++) {
        CString tabKey;
        CString flagKey;
        GruntzPlayer* pl = &g_gameReg->m_options[t];
        CShadeTable* tint = g_gameReg->m_spriteFactory->GetSel(IDX(pl->m_colorIndex), 0);
        if (tint == NULL) {
            return 0;
        }
        tabKey.Format("GAME_STATUSBAR_TABZ_MULTIPLAYERTAB_HEAD%d", t + 1);
        flagKey.Format("GAME_FORTRESSFLAGZ_%s", static_cast<const char*>(GetWarlordName(t)));

        m_tabSprites[t] =
            g_gameReg->m_world->m_childGroup->CreateSprite(0, 0, 0, 0, "DoNothing", 3);
        if (m_tabSprites[t] == NULL) {
            return 0;
        }
        m_tabSprites[t]->ApplyName(tabKey);
        m_tabSprites[t]->ApplyLookupGeometry("GAME_CYCLE100", 0);
        {
            CWwdGameObjectA* o = m_tabSprites[t];
            o->m_drawActive = 1;
            o->m_drawFillCmd = SHADE_PAL_16;
            o->m_drawFillArg = tint;
        }
        m_tabSprites[t]->m_stateFlags |= SPRITE_STATE_HIDDEN;

        m_flagSprites[t] =
            g_gameReg->m_world->m_childGroup->CreateSprite(0, 0, 0, 0, "DoNothing", 3);
        if (m_flagSprites[t] == NULL) {
            return 0;
        }
        m_flagSprites[t]->ApplyName(flagKey);
        m_flagSprites[t]->ApplyLookupGeometry("GAME_CYCLE100", 0);
        {
            CWwdGameObjectA* o = m_flagSprites[t];
            o->m_drawActive = 1;
            o->m_drawFillCmd = SHADE_PAL_16;
            o->m_drawFillArg = tint;
        }
        m_flagSprites[t]->m_stateFlags |= SPRITE_STATE_HIDDEN;

        m_tabSprites[t]->m_screenX = g_bootyTabPos[t].m_x;
        m_tabSprites[t]->m_screenY = g_bootyTabPos[t].m_y;
        {

            i32 frame = (pl->m_joined != 0) ? 1 : 2;
            CWwdGameObjectA* o = m_tabSprites[t];
            CDDrawWorker* set = o->m_frameSet;
            if (set != NULL) {
                CImage* mapped = set->GetAt(frame);
                o->m_layer = mapped;
                o->m_frameIndex = frame;
            }
        }
        m_tabSprites[t]->m_stateFlags &= ~SPRITE_STATE_HIDDEN;
    }

    {
        CShadeTable* tint = g_gameReg->m_spriteFactory->GetSel(
            IDX(g_gameReg->m_options[QueryGruntSlots()].m_colorIndex),
            0
        );
        if (tint == NULL) {
            return 0;
        }
        m_fortSprite =
            g_gameReg->m_world->m_childGroup->CreateSprite(0, 0, 0, 0, "SimpleAnimation", 3);
        if (m_fortSprite == NULL) {
            return 0;
        }
        m_fortSprite->ApplyName("LEVEL_FORT");
        m_fortSprite->ApplyLookupGeometry("GAME_CYCLE100", 0);
        {
            CWwdGameObjectA* o = m_fortSprite;
            o->m_drawActive = 1;
            o->m_drawFillCmd = SHADE_PAL_16;
            o->m_drawFillArg = tint;
        }
        m_fortSprite->m_stateFlags |= SPRITE_STATE_HIDDEN;
        m_fortSprite->m_screenX = 0x64;
        m_fortSprite->m_screenY = 0x64;
        m_fortSprite->m_stateFlags &= ~SPRITE_STATE_HIDDEN;

        CString joyKey;
        CString bootyKey;
        joyKey.Format(
            "GRUNTZ_WARLORDZ_%s_JOY",
            static_cast<const char*>(GetWarlordName(QueryGruntSlots()))
        );
        bootyKey.Format(
            "GRUNTZ_WARLORDZ_%s_BOOTY",
            static_cast<const char*>(GetWarlordName(QueryGruntSlots()))
        );
        m_warlordBooty =
            g_gameReg->m_world->m_childGroup->CreateSprite(0, 0, 0, 0, "SimpleAnimation", 3);
        if (m_warlordBooty == NULL) {
            return 0;
        }
        m_warlordBooty->ApplyName(joyKey);
        m_warlordBooty->ApplyLookupGeometry(bootyKey, 0);
        {
            CWwdGameObjectA* o = m_warlordBooty;
            o->m_drawActive = 1;
            o->m_drawFillCmd = SHADE_PAL_16;
            o->m_drawFillArg = tint;
        }
        m_warlordBooty->m_stateFlags |= SPRITE_STATE_HIDDEN;
        m_warlordBooty->m_screenX = 0x64;
        m_warlordBooty->m_screenY = 0x64;
        CWwdGameObjectA* sorted = m_warlordBooty;
        SET_SORT_KEY_IF_CHANGED(sorted, SORTKEY_BOOTY_WARLORD)
        m_warlordBooty->m_stateFlags &= ~SPRITE_STATE_HIDDEN;

        AddrWord<const Coord> flagPos;
        AddrWord<const Coord> flagEnd;
        flagPos.m_addr = g_bootyFlagPos;
        flagEnd.m_addr = g_bootyTabPos;
        i32 w = 0;
        do {
            i32 held = g_gameReg->m_scoreHud->SumFlags(w);
            i32 placed = 0;
            for (i32 c = 0; c < 4; c++) {
                if (g_gameReg->m_scoreHud->GetFlag(w, c) != 0) {
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
                    m_flagSprites[c]->m_screenX =
                        (spread[held - 1][placed] << 4) + flagPos.m_addr->m_x;
                    m_flagSprites[c]->m_screenY = flagPos.m_addr->m_y;
                    m_flagSprites[c]->m_stateFlags &= ~SPRITE_STATE_HIDDEN;
                    placed++;
                }
            }
            w++;
            flagPos.m_addr++;
        } while (flagPos.m_word < flagEnd.m_word);
    }
    return 1;
}

RVA(0x0001e520, 0x3e)
void CMultiBootyState::ReleaseResources() {

    CDDrawSubMgrLeafScan* reg = m_world->m_soundRegistry;
    if (reg->m_soundStream) {
        reg->m_soundStream->Stop();
    }
    m_world->m_soundRegistry->RemoveKeysEqual("BOOTY", "_");

    m_mgr->m_cueSink->PauseAllVoices();
    CState::ReleaseResources();
}

RVA(0x0001e570, 0xb4)
i32 CMultiBootyState::EnterState(GameStateId) {
    i32 ok = FadeInTitle("multi", 0, 0, 0, 0, 1);
    if (!ok) {
        return ok;
    }
    m_world->m_drawTarget->TransExit();
    RetireScene(0x50, 0x3e8, 0, 1);

    CDDrawSurfaceMgr* host = g_gameReg->m_world;
    i32 item = g_gameReg->m_soundVolume;
    CDDrawSubMgrLeafScan* m28 = host->m_soundRegistry;
    if (m28->m_emitGate == 0) {
        LeafCue* found = NULL;
        MapLookup(m28->m_cues, "BOOTY_LOOP", found);
        if (found) {
            PlayLeafCueIfElapsed(found, item, 0, 0, 1);
        }
    }
    return 1;
}

RVA(0x0001e660, 0x81)
i32 CMultiBootyState::LeaveState(GameStateId) {
    LeafCue* found = LookupCue(m_world->m_soundRegistry->m_cues, "BOOTY_LOOP");
    if (found && found->m_sound->IsPlaying()) {
        found->m_sound->CloneAndPlay(0, 0x1f4, 1);
        while (found->m_sound->IsPlaying()) {
            PurgeVoices(m_world->m_soundRegistry);
        }
    }
    return 1;
}

RVA(0x0001e720, 0x400)
void CMultiBootyState::BuildPowerupIconKeys(CString* reg, i32 key) {
    *reg = "GAME_INGAMEICONZ_";
    // Callers hand in <category base> + <index within the category>, so the id is
    // formed by arithmetic and enters the domain here.
    switch (static_cast<PickupType>(key)) {
        case PICKUP_BOMB:
            *reg += "TOOLZ_BOMBZ";
            return;
        case PICKUP_BOOMERANG:
            *reg += "TOOLZ_BOOMERANGZ";
            return;
        case PICKUP_BRICK:
            *reg += "TOOLZ_BRICKZ";
            return;
        case PICKUP_CLUB:
            *reg += "TOOLZ_CLUBZ";
            return;
        case PICKUP_GAUNTLETZ:
            *reg += "TOOLZ_GAUNTLETZ";
            return;
        case PICKUP_GLOVEZ:
            *reg += "TOOLZ_GLOVEZ";
            return;
        case PICKUP_GOOBER:
            *reg += "TOOLZ_GOOBERZ";
            return;
        case PICKUP_GRAVITYBOOTZ:
            *reg += "TOOLZ_GRAVITYBOOTZ";
            return;
        case PICKUP_GUNHAT:
            *reg += "TOOLZ_GUNHATZ";
            return;
        case PICKUP_NERFGUN:
            *reg += "TOOLZ_NERFGUNZ";
            return;
        case PICKUP_ROCK:
            *reg += "TOOLZ_ROCKZ";
            return;
        case PICKUP_SHIELD:
            *reg += "TOOLZ_SHIELDZ";
            return;
        case PICKUP_SHOVEL:
            *reg += "TOOLZ_SHOVELZ";
            return;
        case PICKUP_SPRING:
            *reg += "TOOLZ_SPRINGZ";
            return;
        case PICKUP_SPY:
            *reg += "TOOLZ_SPYZ";
            return;
        case PICKUP_SWORD:
            *reg += "TOOLZ_SWORDZ";
            return;
        case PICKUP_TIMEBOMB:
            *reg += "TOOLZ_TIMEBOMBZ";
            return;
        case PICKUP_TOOB:
            *reg += "TOOLZ_TOOBZ";
            return;
        case PICKUP_WAND:
            *reg += "TOOLZ_WANDZ";
            return;
        case PICKUP_WARPSTONE:
            *reg += "TOOLZ_WARPSTONEZ1";
            return;
        case PICKUP_WELDER:
            *reg += "TOOLZ_WELDERZ";
            return;
        case PICKUP_WINGZ:
            *reg += "TOOLZ_WINGZ";
            return;
        case PICKUP_BABYWALKER:
            *reg += "TOYZ_BABYWALKERZ";
            return;
        case PICKUP_BEACHBALL:
            *reg += "TOYZ_BEACHBALLZ";
            return;
        case PICKUP_BIGWHEEL:
            *reg += "TOYZ_BIGWHEELZ";
            return;
        case PICKUP_GOKART:
            *reg += "TOYZ_GOKARTZ";
            return;
        case PICKUP_JACKINTHEBOX:
            *reg += "TOYZ_JACKINTHEBOXZ";
            return;
        case PICKUP_JUMPROPE:
            *reg += "TOYZ_JUMPROPEZ";
            return;
        case PICKUP_POGOSTICK:
            *reg += "TOYZ_POGOSTICKZ";
            return;
        case PICKUP_SCROLL:
            *reg += "TOYZ_SCROLLZ";
            return;
        case PICKUP_SQUEAKTOY:
            *reg += "TOYZ_SQUEAKTOYZ";
            return;
        case PICKUP_YOYO:
            *reg += "TOYZ_YOYOZ";
            return;
        case PICKUP_MEGAPHONE:
            *reg += "POWERUPZ_MEGAPHONEZ";
            return;
        case PICKUP_GHOST:
            *reg += "POWERUPZ_GHOST";
            return;
        case PICKUP_SUPERSPEED:
            *reg += "POWERUPZ_SUPERSPEED";
            return;
        case PICKUP_INVULNERABILITY:
            *reg += "POWERUPZ_INVULNERABILITY";
            return;
        case PICKUP_CONVERSION:
            *reg += "POWERUPZ_CONVERSION";
            return;
        case PICKUP_DEATHTOUCH:
            *reg += "POWERUPZ_DEATHTOUCH";
            return;
        case PICKUP_ROIDZ:
            *reg += "POWERUPZ_ROIDZ";
            return;
        case PICKUP_REACTIVEARMOR:
            *reg += "POWERUPZ_REACTIVEARMOR";
            return;
        case PICKUP_RANDOMCOLORZ:
            *reg += "POWERUPZ_RANDOMCOLORZ";
            return;
        case PICKUP_SCREENSHAKE:
            *reg += "POWERUPZ_SCREENSHAKE";
            return;
        case PICKUP_BLACKSCREEN:
            *reg += "POWERUPZ_BLACKSCREEN";
            return;
        case PICKUP_MINICAM:
            *reg += "POWERUPZ_MINICAM";
            return;
        default:
            *reg += "POWERUPZ_COIN";
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
i32 g_panMinX;
DATA(0x0024550c)
i32 g_panMaxX;

RVA_DYNINIT(0x00082b80, 0xa, g_brickText1)
RVA_DYNINIT(0x00082ba0, 0xa, g_brickText1)
RVA_DYNINIT(0x00082bc0, 0xe, g_brickText1)
RVA_DYNINIT(0x00082be0, 0xa, g_brickText1)
DATA(0x00245524)
CString g_brickText1;

RVA_DYNINIT(0x00082c00, 0xa, g_brickText2)
RVA_DYNINIT(0x00082c20, 0xa, g_brickText2)
RVA_DYNINIT(0x00082c40, 0xe, g_brickText2)
RVA_DYNINIT(0x00082c60, 0xa, g_brickText2)
DATA(0x00245528)
CString g_brickText2;

RVA_DYNINIT(0x00082c80, 0xa, g_brickText3)
RVA_DYNINIT(0x00082ca0, 0xa, g_brickText3)
RVA_DYNINIT(0x00082cc0, 0xe, g_brickText3)
RVA_DYNINIT(0x00082ce0, 0xa, g_brickText3)
DATA(0x0024552c)
CString g_brickText3;

RVA_DYNINIT(0x00082d00, 0xa, g_brickText4)
RVA_DYNINIT(0x00082d20, 0xa, g_brickText4)
RVA_DYNINIT(0x00082d40, 0xe, g_brickText4)
RVA_DYNINIT(0x00082d60, 0xa, g_brickText4)
DATA(0x00245530)
CString g_brickText4;

RVA_DYNINIT(0x00082d80, 0xa, g_brickText5)
RVA_DYNINIT(0x00082da0, 0xa, g_brickText5)
RVA_DYNINIT(0x00082dc0, 0xe, g_brickText5)
RVA_DYNINIT(0x00082de0, 0xa, g_brickText5)
DATA(0x00245514)
CString g_brickText5;

RVA_DYNINIT(0x00082e00, 0xa, g_brickText6)
RVA_DYNINIT(0x00082e20, 0xa, g_brickText6)
RVA_DYNINIT(0x00082e40, 0xe, g_brickText6)
RVA_DYNINIT(0x00082e60, 0xa, g_brickText6)
DATA(0x00245518)
CString g_brickText6;

RVA_DYNINIT(0x00082e80, 0xa, g_brickText7)
RVA_DYNINIT(0x00082ea0, 0xa, g_brickText7)
RVA_DYNINIT(0x00082ec0, 0xe, g_brickText7)
RVA_DYNINIT(0x00082ee0, 0xa, g_brickText7)
DATA(0x0024551c)
CString g_brickText7;

RVA_DYNINIT(0x00082f00, 0xa, g_brickText8)
RVA_DYNINIT(0x00082f20, 0xa, g_brickText8)
RVA_DYNINIT(0x00082f40, 0xe, g_brickText8)
RVA_DYNINIT(0x00082f60, 0xa, g_brickText8)
DATA(0x00245520)
CString g_brickText8;

DATA(0x00245534)
i32 g_attractStateCount = 0;
DATA(0x0024553c)
GruntDeathType g_areaHazardDeath = DEATH_DROP;

RVA_DYNINIT(0x00082f80, 0xa, g_coordPool)
RVA_DYNINIT(0x00082fa0, 0x17, g_coordPool)
RVA_DYNINIT(0x00082fd0, 0xe, g_coordPool)
RVA_DYNINIT(0x00082ff0, 0x2f, g_coordPool)
DATA(0x00245540)
FreeNodePool g_coordPool;

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
i32 CMultiBootyState::QueryGruntSlots() {
    i32 i = 0;
    while (i < 4) {
        GruntzPlayer* p = &g_gameReg->m_options[i];
        if (p->m_joined != 0 && p->m_clearedRound == 0) {
            return p->m_playerIndex;
        }
        i++;
    }
    return 0;
}

static __inline i32 sumRun(i32* p, i32 n) {
    i32 s = 0;
    i32 k;
    for (k = 0; k < n; k++) {
        s += p[k];
    }
    return s;
}

// @early-stop
RVA(0x0001ed30, 0x5ac)
void CMultiBootyState::DrawBattleStats() {
    CString s;
    RECT rc;
    BOOL(WINAPI * copyRect)(LPRECT, const RECT*) = CopyRect;
    i32 i;
    i32 c;

    for (i = 0; i < 4; i++) {
        if (g_gameReg->m_options[i].m_joined != 0) {
            s.Format("%d", sumRun(&g_gameReg->m_scoreHud->m_miscPickupz[i * 4], 4));
            copyRect(&rc, &g_col1Rects[i]);
            ShowHudMessage(m_world, &s, &rc, 0x78, 1, 0xff, 0xff, 0, 1);

            s.Format("%d", sumRun(&g_gameReg->m_scoreHud->m_powerupPickupz[i * 7], 7));
            copyRect(&rc, &g_col2Rects[i]);
            ShowHudMessage(m_world, &s, &rc, 0x78, 1, 0xff, 0xff, 0, 1);

            s.Format("%d", sumRun(&g_gameReg->m_scoreHud->m_toyPickupz[i * 10], 10));
            copyRect(&rc, &g_col3Rects[i]);
            ShowHudMessage(m_world, &s, &rc, 0x78, 1, 0xff, 0xff, 0, 1);

            s.Format("%d", sumRun(&g_gameReg->m_scoreHud->m_weaponPickupz[i * 22], 22));
            copyRect(&rc, &g_col4Rects[i]);
            ShowHudMessage(m_world, &s, &rc, 0x78, 1, 0xff, 0xff, 0, 1);

            s.Format("%d", g_gameReg->m_scoreHud->m_counts[i]);
            copyRect(&rc, &g_col5Rects[i]);
            ShowHudMessage(m_world, &s, &rc, 0x78, 1, 0xff, 0xff, 0, 1);

            s.Format("%d", (g_gameReg->m_scoreHud)->SumWinRow(i));
            copyRect(&rc, &g_col6Rects[i]);
            ShowHudMessage(m_world, &s, &rc, 0x78, 1, 0xff, 0xff, 0, 1);
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
        copyRect(&rc, &g_labelRects[c]);
        ShowHudMessage(m_world, &s, &rc, 0x78, 1, 0xff, 0xff, 0, 1);
    }

    for (i = 0; i < 4; i++) {
        if (g_gameReg->m_options[i].m_joined != 0) {
            i32 color;
            switch (g_gameReg->m_options[i].m_colorIndex) {
                case TINT_ORANGE:
                    color = 0x80ff;
                    break;
                case TINT_GREEN:
                    color = 0xff00;
                    break;
                case TINT_BLUE:
                    color = 0xff0000;
                    break;
                case TINT_RED:
                    color = 0xff;
                    break;
                case TINT_PURPLE:
                    color = 0x800080;
                    break;
                case TINT_YELLOW:
                    color = 0xffff;
                    break;
                case TINT_HOTPINK:
                    color = 0x8000ff;
                    break;
                case TINT_DKBLUE:
                    color = 0x800000;
                    break;
                case TINT_DKGREEN:
                    color = 0x8000;
                    break;
                case TINT_TURQ:
                    color = 0x808000;
                    break;
                case TINT_DKRED:
                    color = 0x80;
                    break;
                case TINT_PINK:
                    color = 0xff00ff;
                    break;
                case TINT_DKYELLOW:
                    color = 0x8080;
                    break;
                case TINT_GREY:
                    color = 0x808080;
                    break;
                case TINT_CYAN:
                    color = 0xffff00;
                    break;
                case TINT_WHITE:
                    color = 0xffffff;
                    break;
                default:
                    color = 0;
                    break;
            }
            s.Format("%s", static_cast<const char*>(g_gameReg->m_options[i].GetName()));
            copyRect(&rc, &g_colorRects[i]);
            ShowHudMessage(
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
    rc.left = 0x96;
    rc.top = 0xf;
    rc.right = SCREEN_W_PX;
    rc.bottom = 0x73;
    ShowHudMessage(m_world, &s, &rc, 0x82, 1, 0xff, 0xff, 0, 1);
}

RVA(0x0001f480, 0x1e9)
i32 CMultiBootyState::Render() {
    IDirectDrawSurface* frameSurf = m_world->m_drawTarget->m_frontPair->m_surface->m_ddSurface;
    if (frameSurf == NULL || frameSurf->IsLost() != 0) {
        if (InputVirtual() == 0) {
            m_mgr->ReportError(IDX(IDS_RESTORE_GAME), 0x459);
            return 0;
        }
    }
    if (m_sequenceState == BOOTYSEQ_WARP_CUE) {
        DrawBattleStats();
        m_sequenceState = BOOTYSEQ_PERFECT_BONUS;
    }
    m_world->m_childGroup->TickKillCues(1);
    m_world->m_childGroup->RenderChildren(m_world->m_drawTarget->m_backPair);

    u32 secs = g_gameReg->m_scoreHud->m_elapsedTimeMs / 1000;
    CString s;
    RECT rc;
    SetRect(&rc, 8, 0x41, 0xcb, 0xae);
    if (secs / 3600 != 0) {
        s.Format("%d:%2.2d:%2.2d", secs / 3600, (secs / 60) % 60, secs % 60);
    } else {
        s.Format("%d:%2.2d", secs / 60, secs % 60);
    }
    ShowHudMessageAlt(m_world, &s, &rc, 0x6e, 1, 0xff, 0xff, 0, 1);

    CDDrawSubMgrPages* dt = m_world->m_drawTarget;
    FlipFrontAndRestoreOverlay(dt);
    PurgeVoices(m_world->m_soundRegistry);
    return 1;
}

RVA(0x0001f6f0, 0x10b)
i32 CMultiBootyState::InputVirtual() {
    if (!CState::InputVirtual()) {
        return 0;
    }

    while (ShowCursor(FALSE) >= 0)
        ;

    CSymTab* tree = SymTab2c()->ResolvePath("IMAGEZ");
    if (!tree) {
        return 0;
    }
    CDDrawWorkerRegistry* reg = m_world->m_imageRegistry;
    if (reg->LoadNamespace(tree, "BOOTY", "_") == -1) {
        return 0;
    }

    tree = m_gruntzBank->ResolvePath("IMAGEZ");
    if (!tree) {
        return 0;
    }
    reg = m_world->m_imageRegistry;
    if (reg->LoadNamespace(tree, "GRUNTZ", "_") == -1) {
        return 0;
    }

    tree = m_levelBank->ResolvePath("IMAGEZ");
    if (!tree) {
        return 0;
    }
    reg = m_world->m_imageRegistry;
    if (reg->LoadNamespace(tree, "LEVEL", "_") == -1) {
        return 0;
    }

    if (!FadeInTitle("multi", 0, 0, 0, 0, 1)) {
        return 0;
    }

    DrawBattleStats();
    m_world->m_drawTarget->TransExit();
    RetireScene(0x50, 0x3e8, 0, 1);
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
        PostMessageA(g_gameReg->m_gameWnd->m_hwnd, WM_COMMAND, IDX(CMD_MAIN_MENU), 0);
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
i32 LeafCue::PlayIfElapsed(i32 vol, i32 pan, i32 freqPct, i32 loop) {
    return PlayLeafCueIfElapsed(this, vol, pan, freqPct, loop);
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
