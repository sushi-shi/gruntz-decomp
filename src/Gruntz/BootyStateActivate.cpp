#include <rva.h>

#include <Gruntz/BootyStateActivate.h>

#include <Mfc.h>

#include <Bute/SymParser.h>
#include <Bute/SymTab.h>
#include <DDrawMgr/DDrawChildGroup.h>
#include <DDrawMgr/DDrawSubMgrLeafScan.h>
#include <DDrawMgr/DDrawSubMgrPages.h>
#include <DDrawMgr/DDrawSurfacePair.h>
#include <DDrawMgr/DDrawWorkerRegistry.h>
#include <DDrawMgr/DDSurface.h>
#include <Dsndmgr/DirectSoundMgr.h>
#include <Dsndmgr/SoundStream.h>
#include <Enums.h>
#include <Gruntz/BankMgr.h>
#include <Gruntz/BattlezData.h>
#include <Gruntz/BootyMessages.h>
#include <Gruntz/BootyWalkAnim.h>
#include <Gruntz/CoordNode.h>
#include <Gruntz/GameMode.h>
#include <Gruntz/GameRegistry.h>
#include <Gruntz/GameRegMfcPtr.h>
#include <Gruntz/GameStateId.h>
#include <Gruntz/GruntPuddle.h>
#include <Gruntz/GruntSpawnConfig.h>
#include <Gruntz/GruntzMgr.h>
#include <Gruntz/LeafCue.h>
#include <Gruntz/Play.h>
#include <Gruntz/SoundState.h>
#include <Gruntz/Sprite.h>
#include <Gruntz/SpriteRefTable.h>
#include <Gruntz/UserLogic.h>
#include <Gruntz/WwdGameReg.h>
#include <Image/CImage.h>
#include <Rez/FrameClock.h>
#include <Utils/MapTyped.h>

#include <ddraw.h>
#include <math.h>
#include <stdio.h>

DATA(0x001e8fe8)

const i32 g_bootyLetterCoords[32] = {
    472, 101, 525, 98,  474, 146, 525, 144, 127, 170, 215, 262, 301, 345, 386, 427,
    127, 170, 215, 262, 301, 345, 386, 427, 127, 170, 215, 262, 301, 345, 386, 427,
};

static const float kGlitterPhaseBias = -225.0f;
static const double kDegToRad = 0.017453292;
static const double kGlitterShrinkRate = 0.002;
static const double kGlitterStartRadius = 350.0;

DATA(0x001e9178)
RECT g_col1Rects[4] =
    {{200, 415, 284, 465}, {316, 415, 400, 465}, {432, 415, 516, 465}, {548, 415, 632, 465}};
DATA(0x001e91b8)
RECT g_col2Rects[4] =
    {{200, 372, 284, 422}, {316, 372, 400, 422}, {432, 372, 516, 422}, {548, 372, 632, 422}};
DATA(0x001e91f8)
RECT g_col3Rects[4] =
    {{200, 329, 284, 379}, {316, 329, 400, 379}, {432, 329, 516, 379}, {548, 329, 632, 379}};
DATA(0x001e9238)
RECT g_col4Rects[4] =
    {{200, 286, 284, 336}, {316, 286, 400, 336}, {432, 286, 516, 336}, {548, 286, 632, 336}};
DATA(0x001e9278)
RECT g_col5Rects[4] =
    {{200, 243, 284, 293}, {316, 243, 400, 293}, {432, 243, 516, 293}, {548, 243, 632, 293}};
DATA(0x001e92b8)
RECT g_col6Rects[4] =
    {{200, 200, 284, 250}, {316, 200, 400, 250}, {432, 200, 516, 250}, {548, 200, 632, 250}};
DATA(0x001e92f8)
RECT g_colorRects[4] =
    {{50, 87, 390, 115}, {166, 87, 506, 115}, {282, 87, 622, 115}, {398, 87, 738, 115}};
DATA(0x001e9338)
RECT g_labelRects[7] = {
    {45, 155, 175, 215},
    {50, 198, 180, 258},
    {34, 241, 172, 301},
    {55, 284, 172, 344},
    {66, 327, 174, 387},
    {0, 370, 172, 430},
    {38, 413, 172, 473}
};
VTBL(CMultiBootyState, 0x001e9bdc);
VTBL(CBootyState, 0x001e9cec);

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
        LeafCue* res = 0;
        MapLookup(set->m_cues, "BOOTY_LOOP", res);
        if (res != 0 && g_sndEnabled != 0) {
            u32 now = g_killCueClock;
            if (now - static_cast<u32>(res->m_lastPlayTime)
                >= static_cast<u32>(res->m_replayDelay)) {
                res->m_lastPlayTime = now;
                res->m_sound->ConfigureItem(token, 0, 0, 1);
            }
        }
    }
    return 1;
}

// @early-stop
RVA(0x00018e40, 0x81)
i32 CBootyState::LeaveState(GameStateId) {
    void* obj = 0;
    m_world->m_soundRegistry->m_cues.Lookup("BOOTY_LOOP", obj);
    LeafCue* found = static_cast<LeafCue*>(obj);
    if (found && (static_cast<DirectSoundMgr*>(found->m_sound))->IsPlaying()) {
        (static_cast<DirectSoundMgr*>(found->m_sound))->CloneAndPlay(0, 0x1f4, 1);
        while ((static_cast<DirectSoundMgr*>(found->m_sound))->IsPlaying()) {
            if (m_world->m_soundRegistry->m_soundStream != 0) {
                m_world->m_soundRegistry->m_soundStream->PurgeVoiceList(-1);
            }
        }
    }
    return 1;
}

// @early-stop
RVA(0x00019540, 0x12a)
i32 CBootyState::BuildWarpStoneGlitterAnimation() {
    CGruntzMgr* reg = g_gameReg;

    CWwdGameObjectA** slot = m_trailSprites;
    m_radius = 0xc8;
    m_letterIdx = (reg->m_scoreHud->m_count - 1) % 4;
    m_angleStep = 0;
    m_scratchX = 0;
    m_scratchY = 0;
    for (i32 i = 0; i < 4; i++) {
        CWwdGameObjectA* a =
            g_gameReg->m_world->m_childGroup
                ->CreateSprite(0, 0, 0, (i != m_letterIdx) ? 1 : 3, "DoNothing", 3);
        slot[i] = a;
        if (a == 0) {
            return 0;
        }
        a->ApplyLookupSprite("GAME_STATUSBAR_TABZ_GAMETAB_WARP", i + 2);
        a->m_stateFlags |= 1;
    }
    for (i32 k = 0; k <= m_letterIdx; k++) {
        slot[k]->m_stateFlags &= ~1;
    }
    CWwdGameObjectA* g =
        g_gameReg->m_world->m_childGroup->CreateSprite(0, 0, 0, 4, "SimpleAnimation", 3);
    m_cursorLetter = g;
    if (g == 0) {
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
        if (m_letterIdx >= 0) {
            const i32* tbl = g_bootyLetterCoords + 1;
            CWwdGameObjectA** ap = m_trailSprites;
            for (i32 i = 0; i <= m_letterIdx; i++) {
                CWwdGameObjectA* e = *ap;
                e->m_screenX = tbl[-1];
                e = *ap;
                e->m_screenY = tbl[0];
                e = *ap;
                if (e->m_sortKey != 1) {
                    e->m_sortKey = 1;
                    e->m_flags |= 0x20000;
                }
                ap++;
                tbl += 2;
            }
        }
        m_cursorLetter->m_screenX = g_bootyLetterCoords[m_letterIdx * 2];
        m_cursorLetter->m_screenY = g_bootyLetterCoords[m_letterIdx * 2 + 1];
        return 1;
    }

    i32 step = m_angleStep;
    i32 idx = m_letterIdx;
    double r = static_cast<float>(m_radius);
    double ang = (static_cast<float>(step) - kGlitterPhaseBias) * kDegToRad;
    m_scratchX =
        static_cast<i32>((sin(ang) * r + static_cast<float>(g_bootyLetterCoords[idx * 2])));
    m_scratchY =
        static_cast<i32>((cos(ang) * r + static_cast<float>(g_bootyLetterCoords[idx * 2 + 1])));
    m_angleStep = step + 5;
    m_radius = static_cast<i32>(
        (kGlitterStartRadius
         - static_cast<float>((step + 5)) * kGlitterShrinkRate * kGlitterStartRadius)
    );

    i32 i = 0;
    CWwdGameObjectA** arr1ec = m_trailSprites;
    if (idx > 0) {
        const i32* tbl = g_bootyLetterCoords + 1;
        CWwdGameObjectA** ap = arr1ec;
        do {
            CWwdGameObjectA* e = *ap;
            i++;
            ap++;
            e->m_screenX = tbl[-1];
            e = ap[-1];
            e->m_screenY = tbl[0];
            tbl += 2;
        } while (i < m_letterIdx);
    }

    m_cursorLetter->m_screenX = m_scratchX;
    m_cursorLetter->m_screenY = m_scratchY;
    arr1ec[i]->m_screenX = m_scratchX;
    arr1ec[i]->m_screenY = m_scratchY;

    MoveLettersByDir();

    if (m_radius != 0) {
        return 0;
    }
    CWwdGameObjectA* e = arr1ec[i];
    if (e->m_sortKey != 1) {
        e->m_sortKey = 1;
        e->m_flags |= 0x20000;
    }
    return 1;
}

// @early-stop
RVA(0x00019b90, 0xf8)
void CBootyState::MoveLettersByDir() {
    if (m_initGate) {
        CWwdGameObjectA** p = m_sprintSprites;
        i32 n = 8;
        do {
            CGameObject* e = *p;
            p++;
            e->m_stateFlags |= 1;
        } while (--n);
        return;
    }
    CWwdGameObjectA** p = m_sprintSprites;
    for (i32 i = 0; i < 8; i++, p++) {
        CGameObject* e = *p;
        i32 x = e->m_screenX;
        i32 y = e->m_screenY;
        if (x < 0 || x > 0x280 || y < 0 || y > 0x1e0) {
            e->m_stateFlags |= 1;
        } else {
            switch (i) {
                case 0:
                    e->m_screenX = x;
                    (*p)->m_screenY = y - 4;
                    break;
                case 1:
                    e->m_screenX = x + 4;
                    (*p)->m_screenY = y - 4;
                    break;
                case 2:
                    e->m_screenX = x + 4;
                    (*p)->m_screenY = y;
                    break;
                case 3:
                    e->m_screenX = x + 4;
                    (*p)->m_screenY = y + 4;
                    break;
                case 4:
                    e->m_screenX = x;
                    (*p)->m_screenY = y + 4;
                    break;
                case 5:
                    e->m_screenX = x - 4;
                    (*p)->m_screenY = y + 4;
                    break;
                case 6:
                    e->m_screenX = x - 4;
                    (*p)->m_screenY = y;
                    break;
                case 7:
                    e->m_screenX = x - 4;
                    (*p)->m_screenY = y - 4;
                    break;
            }
        }
    }
}

#define STAT(getter, field)                                                                        \
    ((m_initOnce != 0 && g_gameReg->m_scoreHud->m_allDone != 0) ? g_gameReg->m_scoreHud->getter()  \
                                                                : g_gameReg->m_scoreHud->field)

RVA(0x0001af70, 0x3e0)
void CBootyState::FormatHudText(CString* buf, i32 sel) {
    switch (sel) {
        case 0: {
            u32 secs = static_cast<u32>((STAT(SumElapsedTimeForGroup, m_elapsedTimeMs) / 1000));
            buf->Format("%d:%2.2d", secs / 60, secs % 60);
            return;
        }
        case 1:
            buf->Format("%d", STAT(SumGruntzExitedForGroup, m_gruntzExited));
            return;
        case 2:
            buf->Format("%d", STAT(SumGruntzLostForGroup, m_gruntzLost));
            return;
        case 3: {
            i32 total = STAT(SumToolzAvailableForGroup, m_toolzAvailable);
            i32 cap = STAT(SumToolzAvailableForGroup, m_toolzAvailable);
            i32 cur = STAT(SumToolzCollectedForGroup, m_toolzCount);
            if (cur >= cap) {
                cur = cap;
            }
            buf->Format("%d of %d", cur, total);
            return;
        }
        case 4: {
            i32 total = STAT(SumToyzAvailableForGroup, m_toyzAvailable);
            i32 cap = STAT(SumToyzAvailableForGroup, m_toyzAvailable);
            i32 cur = STAT(SumToyzCollectedForGroup, m_toyzCount);
            if (cur >= cap) {
                cur = cap;
            }
            buf->Format("%d of %d", cur, total);
            return;
        }
        case 5: {
            i32 total = STAT(SumPowerupzAvailableForGroup, m_powerupzAvailable);
            i32 cap = STAT(SumPowerupzAvailableForGroup, m_powerupzAvailable);
            i32 cur = STAT(SumPowerupzCollectedForGroup, m_powerupCount);
            if (cur >= cap) {
                cur = cap;
            }
            buf->Format("%d of %d", cur, total);
            return;
        }
        case 6: {
            i32 total = STAT(SumCoinsAvailableForGroup, m_coinsAvailable);
            i32 cap = STAT(SumCoinsAvailableForGroup, m_coinsAvailable);
            i32 cur = STAT(SumCoinsCollectedForGroup, m_coinsCollected);
            if (cur >= cap) {
                cur = cap;
            }
            buf->Format("%d of %d", cur, total);
            return;
        }
        case 7: {
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
            void* found = 0;
            m28->m_cues.Lookup("BOOTY_PERFECT", found);
            if (found && g_sndEnabled != 0) {
                LeafCue* p = static_cast<LeafCue*>(found);
                if (g_killCueClock - static_cast<u32>(p->m_lastPlayTime)
                    >= static_cast<u32>(p->m_replayDelay)) {
                    p->m_lastPlayTime = g_killCueClock;
                    p->m_sound->ConfigureItem(item, 0, 0, 0);
                }
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
    if (frameSurf == 0 || frameSurf->IsLost() != 0) {
        if (InputVirtual() == 0) {
            m_mgr->ReportError(IDX(CMD_RETURN_TO_MENU), 0x459);
            return 0;
        }
    }
    SoundStream* snd = m_world->m_soundStream;
    if (snd != 0) {
        i32 now = static_cast<i32>(timeGetTime());
        snd->PurgeVoiceList(now);
        snd->TickSubManagers(now);
    }

    i64 elapsed = static_cast<i64>(static_cast<u32>(g_frameTime)) - m_frameStamp64;
    if (elapsed < m_frameInterval64) {
        return 0;
    }
    m_frameIntervalLo = 0x21;
    m_frameIntervalHi = 0;
    m_frameStampLo = g_frameTime;
    m_frameStampHi = 0;

    switch (m_activation) {
        case 100: {
            m_activation = 101;
            CDDrawSubMgrLeafScan* set = g_gameReg->m_world->m_soundRegistry;
            if (set->m_emitGate == 0) {
                LeafCue* cue = 0;
                MapLookup(set->m_cues, "BOOTY_WARP", cue);
                if (cue != 0) {
                    cue->PlayIfElapsed(g_sndCueTag, 0, 0, 0);
                }
            }
        }
            // FALL THROUGH

        case 101: {
            if (StepGlitterAnim() == 0) {
                break;
            }
            m_activation = 102;
            CDDrawSubMgrLeafScan* set = g_gameReg->m_world->m_soundRegistry;
            if (set->m_emitGate == 0) {
                LeafCue* cue = 0;
                MapLookup(set->m_cues, "BOOTY_BOOM", cue);
                if (cue != 0) {
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
        case 102:
            MoveLettersByDir();
            if (LevelMsgHudDriver() == 0) {
                break;
            }
            m_activation = 103;
        // FALL THROUGH
        case 103:
            LevelMsgHudDriver();
            if (UpdateBootyWalkingGruntz() == 0) {
                break;
            }
            m_activation = 0xc7;
            break;
        case 199: {
            LevelMsgHudDriver();
            UpdateBootyWalkingGruntz();
            CheckPerfectBonus();
            if (m_secretHudHandled == 0 && g_gameReg->m_scoreHud->m_isCustomLevel == 0) {
                CString s;
                RECT rc;
                CBattlezData* hud = g_gameReg->m_scoreHud;
                if (hud->m_count > 0x24) {

                    if (hud->m_allDone != 0) {
                        s = "You have completed training! Now, grab the pebble from my hand.";
                    } else {
                        s = "You are closer to achieving mastery! Keep training!";
                    }
                    SetRect(&rc, 0x194, 0xaa, 0x263, 0x1e0);
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
                    SetRect(&rc, 0x194, 0xe6, 0x263, 0x1e0);
                }
                m_secretGate = 1;
                ShowHudMessage(m_world, &s, &rc, 0x6e, 1, 0xff, 0xff, 0, 1);
                m_secretHudHandled = 1;
            } else if (g_gameReg->m_scoreHud->m_isCustomLevel != 0) {
                m_secretHudHandled = 1;
            }
            break;
        }
        case 200:
            return 1;
    }

    m_world->m_childGroup->TickKillCues(1);
    m_world->m_childGroup->RenderChildren(m_world->m_drawTarget->m_backPair);
    CDDrawSubMgrPages* dt = m_world->m_drawTarget;
    dt->m_frontPair->m_surface->Flip(0);
    dt->m_backPair->m_surface
        ->BltFast(0, 0, dt->m_overlayPair->m_surface, &dt->m_overlayPair->m_srcRect, 0x10);
    if (m_world->m_soundRegistry->m_soundStream != 0) {
        m_world->m_soundRegistry->m_soundStream->PurgeVoiceList(-1);
    }
    return 1;
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
RVA(0x0001d440, 0xd7d)
i32 CMultiBootyState::LoadGameAssetNamespaces(CGruntzMgr* mgr, i32 areaArg, i32 prevStateId) {
    if (!CState::LoadGameAssetNamespaces(mgr, areaArg, prevStateId)) {
        return 0;
    }
    m_mgr->RestoreVideoMode(0);
    m_stateBank = static_cast<CSymTab*>(m_symParser->ResolvePath("STATEZ_BOOTY"));
    if (!m_stateBank) {
        return 0;
    }
    m_gameBank = static_cast<CSymTab*>(m_symParser->ResolvePath("GAME"));
    if (!m_gameBank) {
        return 0;
    }
    m_gruntzBank = static_cast<CSymTab*>(m_symParser->ResolvePath("GRUNTZ"));
    if (!m_gruntzBank) {
        return 0;
    }
    {
        char area[128];
        sprintf(area, "AREA%i", (g_gameReg->m_scoreHud->m_count - 1) % 0x24 / 4 + 1);
        m_levelBank = static_cast<CSymTab*>(m_symParser->ResolvePath(area));
    }
    if (!m_levelBank) {
        return 0;
    }
    m_world->m_childGroup->ClearChildren();
    {
        void* soundz = m_stateBank->FindSub("SOUNDZ");
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
            g_gameReg->m_spriteFactory->GetSel(g_gameReg->m_options[i].m_colorIndex, 0);
        if (tint == 0) {
            return 0;
        }
        CString key;

        m_puddleSprites[i] =
            g_gameReg->m_world->m_childGroup->CreateSprite(0, 0, 0, 0, "SimpleAnimation", 3);
        if (m_puddleSprites[i] == 0) {
            return 0;
        }
        m_puddleSprites[i]->ApplyName("GRUNTZ_GRUNTPUDDLE");
        m_puddleSprites[i]->ApplyLookupGeometry(g_puddleSpriteKey, 0);
        {
            CWwdGameObjectA* o = m_puddleSprites[i];
            o->m_drawActive = 1;
            o->m_drawFillCmd = 0xa;
            o->m_drawFillArg = tint;
        }
        m_puddleSprites[i]->m_stateFlags |= 1;

        if (i == QueryGruntSlots()) {
            m_gruntSprites[i] =
                g_gameReg->m_world->m_childGroup->CreateSprite(0, 0, 0, 0, "SimpleAnimation", 3);
            if (m_gruntSprites[i] == 0) {
                return 0;
            }
            m_gruntSprites[i]->ApplyName("GRUNTZ_EXITZ");
            m_gruntSprites[i]->ApplyLookupGeometry("GAME_GRUNTFLEX", 0);
            CWwdGameObjectA* o = m_gruntSprites[i];
            o->m_drawActive = 1;
            o->m_drawFillCmd = 0xa;
            o->m_drawFillArg = tint;
        } else {
            key.Format("GRUNTZ_NORMALGRUNT_IDLE%d", (g_gameReg->Rand() % 2 != 0) ? 1 : 4);
            m_gruntSprites[i] =
                g_gameReg->m_world->m_childGroup->CreateSprite(0, 0, 0, 0, "SimpleAnimation", 3);
            if (m_gruntSprites[i] == 0) {
                return 0;
            }
            m_gruntSprites[i]->ApplyName("GRUNTZ_NORMALGRUNT_SOUTH_IDLE");
            m_gruntSprites[i]->ApplyLookupGeometry(key, 0);
            CWwdGameObjectA* o = m_gruntSprites[i];
            o->m_drawActive = 1;
            o->m_drawFillCmd = 0xa;
            o->m_drawFillArg = tint;
        }
        m_gruntSprites[i]->m_stateFlags |= 1;

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
        if (m_weaponIcons[i] == 0) {
            return 0;
        }
        m_weaponIcons[i]->ApplyName(key);
        m_weaponIcons[i]->ApplyLookupGeometry("GAME_CYCLE100", 0);
        {
            CWwdGameObjectA* o = m_weaponIcons[i];
            o->m_drawActive = 1;
            o->m_drawFillCmd = 0xa;
            o->m_drawFillArg = tint;
        }
        m_weaponIcons[i]->m_stateFlags |= 1;

        {
            CShadeTable* iconTint = g_gameReg->m_spriteFactory->GetSel(0x10, 0);
            if (iconTint == 0) {
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
            if (m_toyIcons[i] == 0) {
                return 0;
            }
            m_toyIcons[i]->ApplyName(key);
            m_toyIcons[i]->ApplyLookupGeometry("GAME_CYCLE100", 0);
            {
                CWwdGameObjectA* o = m_toyIcons[i];
                o->m_drawActive = 1;
                o->m_drawFillCmd = 0xa;
                o->m_drawFillArg = iconTint;
            }
            m_toyIcons[i]->m_stateFlags |= 1;

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
            if (m_powerupIcons[i] == 0) {
                return 0;
            }
            m_powerupIcons[i]->ApplyName(key);
            m_powerupIcons[i]->ApplyLookupGeometry("GAME_CYCLE100", 0);
            {
                CWwdGameObjectA* o = m_powerupIcons[i];
                o->m_drawActive = 1;
                o->m_drawFillCmd = 0xa;
                o->m_drawFillArg = iconTint;
            }
            m_powerupIcons[i]->m_stateFlags |= 1;

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
            if (m_miscIcons[i] == 0) {
                return 0;
            }
            m_miscIcons[i]->ApplyName(key);
            m_miscIcons[i]->ApplyLookupGeometry("GAME_CYCLE100", 0);
            {
                CWwdGameObjectA* o = m_miscIcons[i];
                o->m_drawActive = 1;
                o->m_drawFillCmd = 0xa;
                o->m_drawFillArg = iconTint;
            }
            m_miscIcons[i]->m_stateFlags |= 1;
        }

        m_puddleSprites[i]->m_screenX = g_multiBootyGeom[5][i].m_x;
        m_puddleSprites[i]->m_screenY = g_multiBootyGeom[5][i].m_y;
        m_puddleSprites[i]->m_stateFlags &= ~1;
        m_gruntSprites[i]->m_screenX = g_multiBootyGeom[4][i].m_x;
        m_gruntSprites[i]->m_screenY = g_multiBootyGeom[4][i].m_y;
        m_gruntSprites[i]->m_stateFlags &= ~1;
        m_weaponIcons[i]->m_screenX = g_multiBootyGeom[3][i].m_x;
        m_weaponIcons[i]->m_screenY = g_multiBootyGeom[3][i].m_y;
        m_weaponIcons[i]->m_stateFlags &= ~1;
        m_toyIcons[i]->m_screenX = g_multiBootyGeom[2][i].m_x;
        m_toyIcons[i]->m_screenY = g_multiBootyGeom[2][i].m_y;
        m_toyIcons[i]->m_stateFlags &= ~1;
        m_powerupIcons[i]->m_screenX = g_multiBootyGeom[1][i].m_x;
        m_powerupIcons[i]->m_screenY = g_multiBootyGeom[1][i].m_y;
        m_powerupIcons[i]->m_stateFlags &= ~1;
        m_miscIcons[i]->m_screenX = g_multiBootyGeom[0][i].m_x;
        m_miscIcons[i]->m_screenY = g_multiBootyGeom[0][i].m_y;
        m_miscIcons[i]->m_stateFlags &= ~1;
    }

    for (i32 t = 0; t < 4; t++) {
        CString tabKey;
        CString flagKey;
        GruntzPlayer* pl = &g_gameReg->m_options[t];
        CShadeTable* tint = g_gameReg->m_spriteFactory->GetSel(pl->m_colorIndex, 0);
        if (tint == 0) {
            return 0;
        }
        tabKey.Format("GAME_STATUSBAR_TABZ_MULTIPLAYERT%d", t + 1);
        flagKey.Format("GAME_FORTRESSFLAGZ_%s", static_cast<const char*>(GetWarlordName(t)));

        m_tabSprites[t] =
            g_gameReg->m_world->m_childGroup->CreateSprite(0, 0, 0, 0, "DoNothing", 3);
        if (m_tabSprites[t] == 0) {
            return 0;
        }
        m_tabSprites[t]->ApplyName(tabKey);
        m_tabSprites[t]->ApplyLookupGeometry("GAME_CYCLE100", 0);
        {
            CWwdGameObjectA* o = m_tabSprites[t];
            o->m_drawActive = 1;
            o->m_drawFillCmd = 0xa;
            o->m_drawFillArg = tint;
        }
        m_tabSprites[t]->m_stateFlags |= 1;

        m_flagSprites[t] =
            g_gameReg->m_world->m_childGroup->CreateSprite(0, 0, 0, 0, "DoNothing", 3);
        if (m_flagSprites[t] == 0) {
            return 0;
        }
        m_flagSprites[t]->ApplyName(flagKey);
        m_flagSprites[t]->ApplyLookupGeometry("GAME_CYCLE100", 0);
        {
            CWwdGameObjectA* o = m_flagSprites[t];
            o->m_drawActive = 1;
            o->m_drawFillCmd = 0xa;
            o->m_drawFillArg = tint;
        }
        m_flagSprites[t]->m_stateFlags |= 1;

        m_tabSprites[t]->m_screenX = g_multiBootyGeom[7][t].m_x;
        m_tabSprites[t]->m_screenY = g_multiBootyGeom[7][t].m_y;
        {

            i32 frame = (pl->m_joined != 0) ? 1 : 2;
            CWwdGameObjectA* o = m_tabSprites[t];
            CDDrawWorker* set = o->m_frameSet;
            if (set != 0) {
                CImage* mapped;
                if (frame >= set->m_minIndex && frame <= set->m_maxIndex) {
                    mapped = static_cast<CImage*>(set->m_items.GetAt(frame));
                } else {
                    mapped = 0;
                }
                o->m_layer = mapped;
                o->m_frameIndex = frame;
            }
        }
        m_tabSprites[t]->m_stateFlags &= ~1;
    }

    {
        CShadeTable* tint = g_gameReg->m_spriteFactory->GetSel(
            g_gameReg->m_options[QueryGruntSlots()].m_colorIndex,
            0
        );
        if (tint == 0) {
            return 0;
        }
        m_fortSprite =
            g_gameReg->m_world->m_childGroup->CreateSprite(0, 0, 0, 0, "SimpleAnimation", 3);
        if (m_fortSprite == 0) {
            return 0;
        }
        m_fortSprite->ApplyName("LEVEL_FORT");
        m_fortSprite->ApplyLookupGeometry("GAME_CYCLE100", 0);
        {
            CWwdGameObjectA* o = m_fortSprite;
            o->m_drawActive = 1;
            o->m_drawFillCmd = 0xa;
            o->m_drawFillArg = tint;
        }
        m_fortSprite->m_stateFlags |= 1;
        m_fortSprite->m_screenX = 0x64;
        m_fortSprite->m_screenY = 0x64;
        m_fortSprite->m_stateFlags &= ~1;

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
        if (m_warlordBooty == 0) {
            return 0;
        }
        m_warlordBooty->ApplyName(joyKey);
        m_warlordBooty->ApplyLookupGeometry(bootyKey, 0);
        {
            CWwdGameObjectA* o = m_warlordBooty;
            o->m_drawActive = 1;
            o->m_drawFillCmd = 0xa;
            o->m_drawFillArg = tint;
        }
        m_warlordBooty->m_stateFlags |= 1;
        m_warlordBooty->m_screenX = 0x64;
        m_warlordBooty->m_screenY = 0x64;
        if (m_warlordBooty->m_sortKey != 2) {
            m_warlordBooty->m_sortKey = 2;
            m_warlordBooty->m_flags |= 0x20000;
        }
        m_warlordBooty->m_stateFlags &= ~1;

        for (i32 w = 0; w < 4; w++) {
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
                        (spread[held - 1][placed] << 4) + g_multiBootyGeom[6][w].m_x;
                    m_flagSprites[c]->m_screenY = g_multiBootyGeom[6][w].m_y;
                    m_flagSprites[c]->m_stateFlags &= ~1;
                    placed++;
                }
            }
        }
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
        void* found = 0;
        m28->m_cues.Lookup("BOOTY_LOOP", found);
        if (found && g_sndEnabled != 0) {
            LeafCue* p = static_cast<LeafCue*>(found);
            if (g_killCueClock - static_cast<u32>(p->m_lastPlayTime)
                >= static_cast<u32>(p->m_replayDelay)) {
                p->m_lastPlayTime = g_killCueClock;
                p->m_sound->ConfigureItem(item, 0, 0, 1);
            }
        }
    }
    return 1;
}

// @early-stop
RVA(0x0001e660, 0x81)
i32 CMultiBootyState::LeaveState(GameStateId) {
    void* obj = 0;
    m_world->m_soundRegistry->m_cues.Lookup("BOOTY_LOOP", obj);
    LeafCue* found = static_cast<LeafCue*>(obj);
    if (found && (static_cast<DirectSoundMgr*>(found->m_sound))->IsPlaying()) {
        (static_cast<DirectSoundMgr*>(found->m_sound))->CloneAndPlay(0, 0x1f4, 1);
        while ((static_cast<DirectSoundMgr*>(found->m_sound))->IsPlaying()) {
            if (m_world->m_soundRegistry->m_soundStream != 0) {
                m_world->m_soundRegistry->m_soundStream->PurgeVoiceList(-1);
            }
        }
    }
    return 1;
}

// @early-stop
RVA(0x0001ecf0, 0x2a)
i32 CMultiBootyState::QueryGruntSlots() {
    GruntzPlayer* p = g_gameReg->m_options;
    i32 i = 0;
    while (i < 4) {
        if (p->m_joined != 0 && p->m_clearedRound == 0) {
            return p->m_playerIndex;
        }
        i++;
        p++;
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

    for (c = 0; c <= 6; c++) {
        switch (c) {
            case 0:
                s = "Fortz:";
                break;
            case 1:
                s = "Killz:";
                break;
            case 2:
                s = "Gruntz:";
                break;
            case 3:
                s = "Toolz:";
                break;
            case 4:
                s = "Toyz:";
                break;
            case 5:
                s = "Powerupz:";
                break;
            case 6:
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
                case 0:
                    color = 0x80ff;
                    break;
                case 1:
                    color = 0xff00;
                    break;
                case 2:
                    color = 0xff0000;
                    break;
                case 3:
                    color = 0xff;
                    break;
                case 4:
                    color = 0x800080;
                    break;
                case 5:
                    color = 0xffff;
                    break;
                case 6:
                    color = 0x8000ff;
                    break;
                case 8:
                    color = 0x800000;
                    break;
                case 9:
                    color = 0x8000;
                    break;
                case 10:
                    color = 0x808000;
                    break;
                case 11:
                    color = 0x80;
                    break;
                case 12:
                    color = 0xff00ff;
                    break;
                case 13:
                    color = 0x8080;
                    break;
                case 14:
                    color = 0x808080;
                    break;
                case 15:
                    color = 0xffff00;
                    break;
                case 16:
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
    rc.right = 0x280;
    rc.bottom = 0x73;
    ShowHudMessage(m_world, &s, &rc, 0x82, 1, 0xff, 0xff, 0, 1);
}

RVA(0x0001f480, 0x1e9)
i32 CMultiBootyState::Render() {
    IDirectDrawSurface* frameSurf = m_world->m_drawTarget->m_frontPair->m_surface->m_ddSurface;
    if (frameSurf == 0 || frameSurf->IsLost() != 0) {
        if (InputVirtual() == 0) {
            m_mgr->ReportError(IDX(CMD_RETURN_TO_MENU), 0x459);
            return 0;
        }
    }
    if (m_sequenceState == 0x64) {
        DrawBattleStats();
        m_sequenceState = 0xc7;
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
    dt->m_frontPair->m_surface->Flip(0);
    dt->m_backPair->m_surface
        ->BltFast(0, 0, dt->m_overlayPair->m_surface, &dt->m_overlayPair->m_srcRect, 0x10);
    if (m_world->m_soundRegistry->m_soundStream != 0) {
        m_world->m_soundRegistry->m_soundStream->PurgeVoiceList(-1);
    }
    return 1;
}

RVA(0x0001f6f0, 0x10b)
i32 CMultiBootyState::InputVirtual() {
    if (!CState::InputVirtual()) {
        return 0;
    }

    while (ShowCursor(FALSE) >= 0)
        ;

    void* tree = SymTab2c()->ResolvePath("IMAGEZ");
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
    if (m_sequenceState == 0xc7) {
        PostMessageA(g_gameReg->m_gameWnd->m_hwnd, 0x111, 0x8023, 0);
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
