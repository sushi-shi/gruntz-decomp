#ifndef SRC_GRUNTZ_GAMEMODE_H
#define SRC_GRUNTZ_GAMEMODE_H

#include <rva.h>

#include <Mfc.h>
#include <MfcWin.h>

#include <DDrawMgr/DDrawSurfaceMgr.h>
#include <Gruntz/BootySeqPhase.h>
#include <Gruntz/BootyStatRow.h>
#include <Gruntz/ChatBox.h>
#include <Gruntz/FixedPtrArray32.h>
#include <Gruntz/GameRegistry.h>
#include <Gruntz/GameStateId.h>
#include <Gruntz/GlyphStringDraw.h>
#include <Gruntz/State.h>
#include <Gruntz/View.h>

extern "C" tagRECT g_versionRect;

#include <Rez/FrameClock.h>

GZ_ENUM_FORWARD(GruntDirection);

struct BzGeomPair {
    i32 m_y;
    i32 m_x;
};
SIZE_UNKNOWN();

extern RECT g_levelMsgRectsB[8];

struct LeafCue;
class CMoviePlayer;
struct CGameObject;
class CWwdGameObjectA;

class CMenuState : public CState {
public:
    CMenuState() {
        m_menuTree = NULL;
    }
    virtual i32 RestoreDisplay() OVERRIDE;

    virtual i32 LoadGameAssetNamespaces(CGruntzMgr* mgr, i32 areaArg, i32 prevStateId) OVERRIDE;
    virtual i32 OnPaint() OVERRIDE;
    virtual i32 InputVirtual() OVERRIDE;
    virtual i32 EnterState(GameStateId) OVERRIDE;
    virtual i32 OnKeyDown(i32, i32) OVERRIDE;
    virtual i32 OnLButtonDown(i32, i32, i32) OVERRIDE;
    virtual i32 OnLButtonDblClk(i32, i32, i32) OVERRIDE;
    virtual i32 OnMouseMove(i32, i32, i32) OVERRIDE;

    virtual ~CMenuState() OVERRIDE;
    RVA(0x0008ce10, 0x6)
    virtual GameStateId Update() OVERRIDE {
        return GAMESTATE_MENU;
    }
    virtual i32 Render() OVERRIDE;
    virtual void ReleaseResources() OVERRIDE;
    virtual i32 LeaveState(GameStateId arg) OVERRIDE;

    void StartMusic();
    void StopMusicChain();

    i32 CommitState();

    CChatBox* m_menuTree;
    i32 m_activateCueDurationMs;
    LeafCue* m_menuMusicCue;

    void BuildVersionString(tagRECT r);
};
SIZE_UNKNOWN();
SIZE_UNKNOWN();

class CCreditsState : public CState {
public:
    CCreditsState() {
        m_flashColor = 0;
        m_flashTimer = 0;
        m_fadeCountdown = 0;
        m_fxEnabled = 0;
        m_scrollReseedTimer = 0;
        m_scrollAccum = 0;
        m_scrollStep = 0;
        // Two-CRects knot: retail calls the GAME CRect::SetRect (ILT 0x278e), but
        // this header rides in afxwin TUs where CRect is MFC's - its inline COMDAT
        // collides with the game def at link. Hand-expanded (the inline's own body)
        // until CRect unification; costs this ctor's %, buys a /FORCE-free link.
        ::SetRect(&m_scrollRect, 0, 0, 0x280, 0x1e0);
        ::SetRect(&m_drawRect, 0, 0, 0x280, 0x1e0);
        m_reserved20c = 1;
        m_videoHandle = NULL;
        m_videoPlaying = 0;
        m_musicStarted = 0;
    }

    virtual i32 LoadGameAssetNamespaces(CGruntzMgr* mgr, i32 areaArg, i32 prevStateId) OVERRIDE;

    virtual ~CCreditsState() OVERRIDE;
    virtual void ReleaseResources() OVERRIDE;
    RVA(0x0008d590, 0x6)
    virtual GameStateId Update() OVERRIDE {
        return GAMESTATE_CREDITS;
    }
    virtual i32 Render() OVERRIDE;
    virtual i32 RestoreDisplay() OVERRIDE;
    virtual i32 InputVirtual() OVERRIDE;
    virtual i32 EnterState(GameStateId) OVERRIDE;
    virtual i32 LeaveState(GameStateId) OVERRIDE;
    virtual i32 OnKeyDown(i32, i32) OVERRIDE;
    virtual i32 OnLButtonDown(i32, i32, i32) OVERRIDE;

    i32 DrawScrollingCredits();

    i32 FinishState();
    i32 StepVideo();
    i32 FlashColor();

    void LoadCreditzAssets();

    i32 m_musicStarted;
    i32 m_flashColor;
    i32 m_flashTimer;
    i32 m_fadeCountdown;
    i32 m_fxEnabled;

    CRect m_scrollRect;
    CRect m_drawRect;
    CRgn m_clipRegion;
    CString m_caption;
    i32 m_scrollReseedTimer;

    double m_scrollAccum;
    double m_scrollStep;
    i32 m_videoPlaying;
    i32 m_reserved20c; // 1 in init, 2 in credits; never read
    CMoviePlayer* m_videoHandle;

    char m_pad214[0x218 - 0x214];

    i32 InitAttractTitle();

    i32 SetupTitle();
};
SIZE_UNKNOWN();
SIZE_UNKNOWN();

class CBootyState : public CState {
public:
    CBootyState() {
        m_frameStampLo = 0;
        m_frameIntervalLo = 0;
        m_frameStampHi = 0;
        m_frameIntervalHi = 0;
        m_secretHudHandled = 0;
        m_activation = BOOTYSEQ_WARP_CUE;
        m_slot = 0;
        m_stepIndex = 0;
        m_walkStarted = 0;
        m_soundStarted = 0;
        m_initGate = 0;
        m_secretGate = 0;
        m_levelCompleteGate = 0;
        m_initOnce = 0;
        m_secretBannerOnce = 0;
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
    RVA(0x0008d3f0, 0x6)
    virtual GameStateId Update() OVERRIDE {
        return GAMESTATE_BOOTY;
    }
    virtual i32 Render() OVERRIDE;
    virtual i32 RestoreDisplay() OVERRIDE;
    virtual i32 OnPaint() OVERRIDE;
    virtual i32 InputVirtual() OVERRIDE;
    virtual i32 EnterState(GameStateId) OVERRIDE;
    virtual i32 LeaveState(GameStateId) OVERRIDE;
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

    i32 m_initGate;
    i32 m_secretHudHandled;
    i32 m_activation;

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
    i32 m_initOnce;
    i32 m_secretBannerOnce;

    i32 m_letterIdx;
    i32 m_radius;
    i32 m_angleStep;
    i32 m_scratchX;
    i32 m_scratchY;
    CWwdGameObjectA* m_trailSprites[4];

    CWwdGameObjectA* m_cursorLetter;
    i32 m_levelCompleteGate;

    CWwdGameObjectA* m_sprintSprites[8];

    CWwdGameObjectA* m_bomb[8];
    CWwdGameObjectA* m_gokart[8];
    CWwdGameObjectA* m_expl[8];

    i32 m_readyFlags[8];
    i32 m_templateFlags[8];
    i32 m_slot;
    CWwdGameObjectA* m_visSprites[4];
    CWwdGameObjectA* m_animSprites[4];
    i32 m_stepIndex;
    i32 m_walkStarted;
    i32 m_soundStarted;
    i32 m_secretGate;

    CWwdGameObjectA* m_bootyPerfectSprite;

    CWwdGameObjectA* m_icons[8];

    char m_pad31c[0x320 - 0x31c];
};
SIZE_UNKNOWN();
SIZE_UNKNOWN();

class CMultiBootyState : public CState {
public:
    CMultiBootyState() {
        m_reserved1b4 = 0;
        m_sequenceState = 0x64;
    }

    virtual i32 LoadGameAssetNamespaces(CGruntzMgr* mgr, i32 areaArg, i32 prevStateId) OVERRIDE;

    virtual ~CMultiBootyState() OVERRIDE;
    virtual void ReleaseResources() OVERRIDE;
    RVA(0x0008d4c0, 0x6)
    virtual GameStateId Update() OVERRIDE {
        return GAMESTATE_MULTIBOOTY;
    }
    virtual i32 Render() OVERRIDE;
    virtual i32 RestoreDisplay() OVERRIDE;
    virtual i32 OnPaint() OVERRIDE;
    virtual i32 InputVirtual() OVERRIDE;
    virtual i32 EnterState(GameStateId) OVERRIDE;
    virtual i32 LeaveState(GameStateId) OVERRIDE;
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
    i32 m_sequenceState;
    CWwdGameObjectA* m_puddleSprites[4];
    CWwdGameObjectA* m_gruntSprites[4];
    CWwdGameObjectA* m_weaponIcons[4];
    CWwdGameObjectA* m_toyIcons[4];
    CWwdGameObjectA* m_powerupIcons[4];
    CWwdGameObjectA* m_miscIcons[4];
    CWwdGameObjectA* m_tabSprites[4];
    CWwdGameObjectA* m_flagSprites[4];
    CWwdGameObjectA* m_warlordBooty;
    CWwdGameObjectA* m_fortSprite;
};
SIZE_UNKNOWN();
SIZE_UNKNOWN();

#endif // SRC_GRUNTZ_GAMEMODE_H
