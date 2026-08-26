#ifndef SRC_GRUNTZ_GAMEMODE_H
#define SRC_GRUNTZ_GAMEMODE_H

#include <rva.h>

#include <Mfc.h>
#include <MfcWin.h>

#include <DDrawMgr/DDrawSurfaceMgr.h>
#include <Gruntz/BootySeqPhase.h>
#include <Gruntz/BootyStatRow.h>
#include <Gruntz/FixedPtrArray32.h>
#include <Gruntz/GameRegistry.h>
#include <Gruntz/GameStateId.h>
#include <Gruntz/GlyphStringDraw.h>
#include <Gruntz/MenuTree.h>
#include <Gruntz/State.h>
#include <Gruntz/View.h>

extern CRect g_versionRect;

#include <Rez/FrameClock.h>
#include <Gruntz/WarpLetter.h>

GZ_ENUM_FORWARD(GruntDirection);

extern RECT g_levelMsgRectsB[8];

struct SoundCue;
class CMoviePlayer;
struct CGameObject;
class CWwdSpriteObject;

class CMenuState : public CState {
public:
    CMenuState() {
        m_menuTree = NULL;
    }
    virtual i32 RestoreDisplay() OVERRIDE;

    virtual i32 LoadGameAssetNamespaces(CGruntzMgr* mgr, i32 areaArg, i32 prevStateId) OVERRIDE;
    virtual i32 OnPaint() OVERRIDE;
    virtual i32 InputVirtual() OVERRIDE;
    virtual i32 EnterState(GameStateId previousState) OVERRIDE;
    virtual i32 OnKeyDown(i32, i32) OVERRIDE;
    virtual i32 OnLButtonDown(i32, i32, i32) OVERRIDE;
    virtual i32 OnLButtonDblClk(i32, i32, i32) OVERRIDE;
    virtual i32 OnMouseMove(i32, i32, i32) OVERRIDE;

    virtual ~CMenuState() OVERRIDE;
    RVA(0x0008cd30, 0x6)
    virtual GameStateId Update() OVERRIDE {
        return GAMESTATE_MENU;
    }
    virtual i32 Render() OVERRIDE;
    virtual void ReleaseResources() OVERRIDE;
    virtual i32 LeaveState(GameStateId nextState) OVERRIDE;

    void StartMusic();
    void StopMusicChain();

    i32 CommitState();

    CMenuTree* m_menuTree;
    i32 m_activateCueDurationMs;
    SoundCue* m_menuMusicCue;

    void BuildVersionString(CRect r);
};

class CCreditsState : public CState {
public:
    CCreditsState() {
        m_flashColor = 0;
        m_flashTimer = 0;
        m_fadeCountdown = 0;
        m_fxEnabled = false;
        m_scrollReseedTimer = 0;
        m_scrollAccum = 0;
        m_scrollStep = 0;
        m_scrollRect.SetRect(0, 0, 0x280, 0x1e0);
        m_drawRect.SetRect(0, 0, 0x280, 0x1e0);
        m_reserved20c = 1;
        m_videoHandle = NULL;
        m_videoPlaying = false;
        m_musicStarted = false;
    }

    virtual i32 LoadGameAssetNamespaces(CGruntzMgr* mgr, i32 areaArg, i32 prevStateId) OVERRIDE;

    virtual ~CCreditsState() OVERRIDE;
    virtual void ReleaseResources() OVERRIDE;
    RVA(0x0008d4b0, 0x6)
    virtual GameStateId Update() OVERRIDE {
        return GAMESTATE_CREDITS;
    }
    virtual i32 Render() OVERRIDE;
    virtual i32 RestoreDisplay() OVERRIDE;
    virtual i32 InputVirtual() OVERRIDE;
    virtual i32 EnterState(GameStateId previousState) OVERRIDE;
    virtual i32 LeaveState(GameStateId nextState) OVERRIDE;
    virtual i32 OnKeyDown(i32, i32) OVERRIDE;
    virtual i32 OnLButtonDown(i32, i32, i32) OVERRIDE;

    i32 DrawScrollingCredits();

    i32 FinishState();
    i32 StepVideo();
    i32 FlashColor();

    void LoadCreditzAssets();

    b32 m_musicStarted;
    i32 m_flashColor;
    i32 m_flashTimer;
    i32 m_fadeCountdown;
    b32 m_fxEnabled;

    CRect m_scrollRect;
    CRect m_drawRect;
    CRgn m_clipRegion;
    CString m_caption;
    i32 m_scrollReseedTimer;

    double m_scrollAccum;
    double m_scrollStep;
    b32 m_videoPlaying;
    i32 m_reserved20c; // 1 in init, 2 in credits; never read
    CMoviePlayer* m_videoHandle;

    char m_pad214[0x218 - 0x214];

    i32 InitAttractTitle();

    i32 SetupTitle();
};

class CBootyState : public CState {
public:
    CBootyState() {
        m_frameStampLo = 0;
        m_frameIntervalLo = 0;
        m_frameStampHi = 0;
        m_frameIntervalHi = 0;
        m_secretHudHandled = false;
        m_activation = BOOTYSEQ_WARP_CUE;
        m_slot = 0;
        m_stepIndex = 0;
        m_walkStarted = false;
        m_soundStarted = false;
        m_initGate = false;
        m_secretGate = false;
        m_levelCompleteGate = false;
        m_initOnce = false;
        m_secretBannerOnce = false;
        for (i32 t = 0; t < 4; t++) {
            m_trailSprites[t] = NULL;
        }
        for (i32 i = 0; i < 8; i++) {
            m_readyFlags[i] = 0;
            m_templateFlags[i] = 0;
        }
    }

    virtual i32 LoadGameAssetNamespaces(CGruntzMgr* mgr, i32 areaArg, i32 prevStateId) OVERRIDE;

    virtual ~CBootyState() OVERRIDE;
    virtual void ReleaseResources() OVERRIDE;
    RVA(0x0008d310, 0x6)
    virtual GameStateId Update() OVERRIDE {
        return GAMESTATE_BOOTY;
    }
    virtual i32 Render() OVERRIDE;
    virtual i32 RestoreDisplay() OVERRIDE;
    virtual i32 OnPaint() OVERRIDE;
    virtual i32 InputVirtual() OVERRIDE;
    virtual i32 EnterState(GameStateId previousState) OVERRIDE;
    virtual i32 LeaveState(GameStateId nextState) OVERRIDE;
    virtual i32 OnKeyDown(i32, i32) OVERRIDE;
    virtual i32 OnLButtonDown(i32, i32, i32) OVERRIDE;
    virtual i32 OnRButtonDown(i32, i32, i32) OVERRIDE;

    i32 BuildBootyGruntIdleAnimation();
    i32 ShowSecretBonusMessage();
    void ShowLevelCompleteMessage();
    i32 BuildBootyWalkingGruntz();
    i32 UpdateBootyWalkingGruntz();

    i32 LoadGruntEffectSprites();
    i32 LevelMsgHudDriver();
    void FormatHudText(CString* buf, BootyStatRow sel);

    i32 BuildWarpStoneGlitterAnimation();

    i32 StepGlitterAnim();
    void MoveLettersByDir();
    i32 BuildGruntSprintAnimation();
    i32 BuildBootyPerfectAnimation();

    i32 CheckPerfectBonus();

    void GenMenuRandPos(GruntDirection sel, i32* outX, i32* outY);

    b32 m_initGate;
    b32 m_secretHudHandled;
    BootySeqPhase m_activation;

    union {
        i64 m_frameStamp64;
        struct {
            i32 m_frameStampLo;
            i32 m_frameStampHi;
        };
    };
    union {
        i64 m_frameInterval64;
        struct {
            i32 m_frameIntervalLo;
            i32 m_frameIntervalHi;
        };
    };
    b32 m_initOnce;
    b32 m_secretBannerOnce;

    i32 m_letterIdx;
    i32 m_radius;
    i32 m_angleStep;
    i32 m_scratchX;
    i32 m_scratchY;
    CWwdSpriteObject* m_trailSprites[4];

    CWwdSpriteObject* m_cursorLetter;
    b32 m_levelCompleteGate;

    CWwdSpriteObject* m_sprintSprites[8];

    CWwdSpriteObject* m_bomb[8];
    CWwdSpriteObject* m_gokart[8];
    CWwdSpriteObject* m_expl[8];

    i32 m_readyFlags[8];
    i32 m_templateFlags[8];
    i32 m_slot;
    CWwdSpriteObject* m_visSprites[4];
    CWwdSpriteObject* m_animSprites[WARPLETTER_COUNT];
    i32 m_stepIndex;
    b32 m_walkStarted;
    b32 m_soundStarted;
    b32 m_secretGate;

    CWwdSpriteObject* m_bootyPerfectSprite;

    CWwdSpriteObject* m_icons[8];

    char m_pad31c[0x320 - 0x31c];
};

class CMultiBootyState : public CState {
public:
    CMultiBootyState() {
        m_reserved1b4 = 0;
        m_sequenceState = BOOTYSEQ_WARP_CUE;
    }

    virtual i32 LoadGameAssetNamespaces(CGruntzMgr* mgr, i32 areaArg, i32 prevStateId) OVERRIDE;

    virtual ~CMultiBootyState() OVERRIDE;
    virtual void ReleaseResources() OVERRIDE;
    RVA(0x0008d3e0, 0x6)
    virtual GameStateId Update() OVERRIDE {
        return GAMESTATE_MULTIBOOTY;
    }
    virtual i32 Render() OVERRIDE;
    virtual i32 RestoreDisplay() OVERRIDE;
    virtual i32 OnPaint() OVERRIDE;
    virtual i32 InputVirtual() OVERRIDE;
    virtual i32 EnterState(GameStateId previousState) OVERRIDE;
    virtual i32 LeaveState(GameStateId nextState) OVERRIDE;
    virtual i32 OnKeyDown(i32, i32) OVERRIDE;
    virtual i32 OnLButtonDown(i32, i32, i32) OVERRIDE;
    virtual i32 OnRButtonDown(i32, i32, i32) OVERRIDE;

    void DrawBattleStats();

    i32 QueryGruntSlots();

    void BuildPowerupIconKeys(CString* reg, i32 key);

    CString GetWarlordName(i32 id);

    void OnActivated();

    i32 ForwardIdleAnim(i32 a, i32 b);
    i32 Paint();
    i32 BuildBootyGruntIdleAnimation();

    i32 PostCommandIfKey();

    i32 m_reserved1b4;
    BootySeqPhase m_sequenceState;
    CWwdSpriteObject* m_puddleSprites[4];
    CWwdSpriteObject* m_gruntSprites[4];
    CWwdSpriteObject* m_weaponIcons[4];
    CWwdSpriteObject* m_toyIcons[4];
    CWwdSpriteObject* m_powerupIcons[4];
    CWwdSpriteObject* m_miscIcons[4];
    CWwdSpriteObject* m_tabSprites[4];
    CWwdSpriteObject* m_flagSprites[4];
    CWwdSpriteObject* m_warlordBooty;
    CWwdSpriteObject* m_fortSprite;
};

#endif // SRC_GRUNTZ_GAMEMODE_H
