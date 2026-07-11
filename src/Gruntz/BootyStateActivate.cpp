// BootyStateActivate.cpp - the CBootyState / CMultiBootyState per-frame activation
// slot bodies (C:\Proj\Gruntz), FOLDED onto the canonical GameMode.h siblings.
//
// Attribution is vtable-proven (see the retail state vtables):
//   0x18d30 = CBootyState      slot 9 (+0x24)  vtbl 0x5e9cec  (Vslot09)
//   0x1f6f0 = CMultiBootyState slot 8 (+0x20)  vtbl 0x5e9bdc  (InputVirtual / OnActivate2)
// The two are CONFIRMED-distinct siblings over CState (<Gruntz/GameMode.h>). The old
// per-TU CBootyState/CMultiBootyState views (with BootyAssetRoot @+0x0c, BootyRegistrar,
// BootyNamespace @+0x2c/+0x28/+0x30) are folded away here onto the CState + CSpriteFactoryHolder facets:
//   - m_c (+0x0c)  == CSpriteFactoryHolder (the shared render/resource context - see <Gruntz/View.h>).
//     The +0x04 loader (was CGruntDataLoader::Load) is CSpriteFactoryHolder's m_renderState->Flush
//     (0x158ee0); the +0x10 registrar (was BootyRegistrar::CallRegister) is CSpriteFactoryHolder's
//     m_imageRegistry->LoadNamespace (vtable slot +0x4c).
//   - the +0x2c/+0x28/+0x30 asset sources (were BootyNamespace) are CState::m_2c /
//     m_levelBank / m_gruntzBank (CResSource; LookupSet == the old Lookup, 0x13bae0).
// Only the g_gameReg world sound set below stays a local facet view - a separate
// object web from the +0x0c context (deferred canonical world-sound model). Only offsets
// / code bytes are load-bearing; every helper is a reloc-masked external.
#include <Gruntz/SoundCueMgr.h>
#include <Bute/SymTab.h>
#include <Bute/SymParser.h>
#include <DDrawMgr/DDrawSubMgrPages.h>
#include <DDrawMgr/DDrawSubMgrLeafScan.h> // RemoveKeysEqual_157c70 (Booty/MultiBooty ReleaseResources)
#include <DDrawMgr/DDrawWorkerRegistry.h> // RemoveKeysEqual_155360 (CBootyState::ReleaseResources)
#include <DDrawMgr/DDSurface.h> // CMultiBootyState::Render: CDDSurface Flip/BltFast on the frame surfaces
#include <Mfc.h>                // ShowCursor (reloc-masked); GameMode.h needs the afx umbrella
#include <ddraw.h> // CMultiBootyState::Render: real IDirectDrawSurface::IsLost dispatch (slot 24)
#include <math.h>  // sin/cos (StepGlitterAnim sine spiral)

#include <rva.h>
#include <Gruntz/BankMgr.h> // CResSource::LookupSet (CState::m_2c/m_levelBank/m_gruntzBank)
#include <Gruntz/GameMode.h> // canonical CBootyState/CMultiBootyState : CState + CSpriteFactoryHolder facet
#include <Gruntz/GruntzMgr.h>     // CMultiBootyState::Render: CState::m_4 owner (ReportError)
#include <Gruntz/SpriteFactory.h> // CMultiBootyState::Render: m_c->m_8 frame-worker slots 9/10
#include <Gruntz/LeafCue.h>       // LeafCue (Booty/MultiBooty FrameSlot28 BOOTY_LOOP cue)
#include <Gruntz/UserLogic.h>     // CGameObject (glitter/letter sprites; StepGlitterAnim/BuildWarp)
#include <Gruntz/BattlezData.h>   // CBattlezData::InBounds (CheckPerfectBonus frame-ready gate)
#include <Gruntz/WwdGameReg.h> // WwdGameReg (g_gameReg; CheckPerfectBonus/Vslot09/QueryGruntSlots)
#include <Gruntz/GameRegistry.h> // CGameRegistry (g_mgr; CBattleStatsView::DrawBattleStats, waveP)
#include <Io/MoviePlayer.h>      // CMoviePlayer (~; CMultiBootyState::ReleaseResources m_4->m_60)

// CMultiBootyState::Render's HUD line is drawn through the shared GlyphStringDraw.cpp
// free function (0x115520); declared here (reloc-masked) so the call co-names with retail.
struct HudMsgSink;
void ShowHudMessageAlt(
    HudMsgSink* sink,
    i32 rect,
    i32 str,
    i32 a4,
    i32 a5,
    i32 a6,
    i32 a7,
    i32 a8,
    i32 a9
);

// ---------------------------------------------------------------------------
// The game-registry (*0x24556c == g_gameReg, the CGameRegistry singleton) WORLD
// sound set, re-triggered by CBootyState::Vslot09's ambient BOOTY_LOOP poll. This is a
// SEPARATE object web from the +0x0c CSpriteFactoryHolder context (the world holder's sound SET, reached
// as reg->m_world(+0x30)->m_28); the vfunc_9 global-load path reads it off the singleton,
// not off `this`, so it stays a local facet view here pending a canonical world-sound
// model. Reloc-masked __thiscall entries.
SIZE_UNKNOWN(BootySndEntry);
struct BootySndEntry {
    char m_pad00[0x10];
    CSoundCueMgr* m_player; // +0x10  the player
    u32 m_lastPlayed;       // +0x14  last-played frame stamp
    u32 m_interval;         // +0x18  min replay interval
};
SIZE_UNKNOWN(BootySndTable);
SIZE_UNKNOWN(BootySndSet);
struct BootySndSet {
    char m_pad00[0x10];
    char m_table; // +0x10 (CMapStringToPtr body starts here; cast at Find)
    char m_pad11[0x30 - 0x11];
    i32 m_activeGuard; // +0x30  active guard (nonzero -> skip the ambient poll)
};
SIZE_UNKNOWN(BootySndWorld);
struct BootySndWorld { // g_gameReg->m_world (+0x30) sound facet
    char m_pad00[0x28];
    BootySndSet* m_soundSet; // +0x28
};
SIZE_UNKNOWN(BzGameWnd);
struct BzGameWnd { // g_gameReg->m_gameWnd (+0x04): the game window; m_hwnd @+0x04
    char m_pad00[4];
    HWND m_hwnd; // +0x04
};
// The elapsed-time clock the booty countdown reads (g_gameReg->m_7c->m_10 ms).
SIZE_UNKNOWN(BzGameClock);
struct BzGameClock {
    char m_pad00[0x10];
    i32 m_elapsedMs; // +0x10  elapsed game milliseconds (Render / 1000 -> seconds)
};
SIZE_UNKNOWN(BzGameReg);
struct BzGameReg { // == *0x24556c (g_gameReg), viewed for the world sound set
    char m_pad00[4];
    BzGameWnd* m_gameWnd; // +0x04  the game window (KeyHost::Check posts to m_hwnd)
    char m_pad08[0x30 - 0x08];
    BootySndWorld* m_world; // +0x30
    char m_pad34[0x7c - 0x34];
    BzGameClock* m_7c; // +0x7c  elapsed-time clock (Render countdown source)
    char m_pad80[0x11c - 0x80];
    i32 m_soundToken; // +0x11c  ambient sound token
};
DATA(0x0024556c)
extern "C" BzGameReg* g_gameReg;
// USER32 PostMessageA reached through the game-owned IAT-style fn-ptr (ff 15 [ptr]);
// same global CGruntzMgr/Attract/Play bind. KeyHost::Check posts through it.
DATA(0x002c44c8)
extern i32(WINAPI* g_pPostMessageA)(void*, u32, u32, i32); // 0x6c44c8
DATA(0x0021ab20)
extern i32 g_sndEnabled; // BOOTY_LOOP enable gate
DATA(0x002bf3c0)
extern "C" u32 g_killCueClock; // wrap-safe draw clock

// The owning game-manager (CState::m_4, a real CGruntzMgr*) reached through the
// gamemode-local CGMOwner reduced view (same helper the sibling state TUs use).
static inline CGMOwner* Owner(CState* s) {
    return (CGMOwner*)s->m_4;
}
// The scalar-deleting dtor's operator delete (declared so /GX tracks the EH state).
void operator delete(void*);

// ---------------------------------------------------------------------------
// CGlitterMgr / CBooty* views of the *0x24556c singleton (aliased with the BzGameReg
// view above): the glitter animator reads it as CGlitterMgr (m_world sprite factory +
// m_7c letter-count set), the perfect-bonus / cue pollers read it as CBootyGameReg
// (m_30 music host, m_7c draw-ready object, m_11c cue item) through the g_gameReg
// WwdGameReg alias. Same address; codegen-neutral reloc-masked loads. The 0x24556c
// canonical-type unification is a separate worklist item.
// ---------------------------------------------------------------------------
// g_6bf3c0 / g_61ab20 are the draw-clock mirror + reentrancy gate (same 0x6bf3c0 /
// 0x61ab20 data as g_killCueClock / g_sndEnabled above; the moved bodies use these names).
extern "C" u32 g_6bf3c0; // draw-clock mirror
extern i32 g_61ab20;     // DAT_0061ab20 reentrancy gate

// The glitter factory chain (CGlitterMgr view of g_gameReg): m_world->m_8 sprite
// factory (CreateSprite @0x1597b0), m_7c->m_4 active letter count.
SIZE_UNKNOWN(CGlitterMgrM30);
struct CGlitterMgrM30 {
    char m_pad00[0x8];
    CSpriteFactory* m_8; // +0x08 the animation factory
};
SIZE_UNKNOWN(CGlitterMgrSet);
struct CGlitterMgrSet {
    char m_pad00[0x4];
    i32 m_4; // +0x04 element count
};
SIZE_UNKNOWN(CGlitterMgr);
struct CGlitterMgr {
    char m_pad00[0x30];
    CGlitterMgrM30* m_world; // +0x30
    char m_pad34[0x7c - 0x34];
    CGlitterMgrSet* m_7c; // +0x7c
};

// The packed {x,y} spawn-coordinate table StepGlitterAnim indexes by m_letterIdx
// (DAT_005e8fe8; x via [tbl], y via [tbl+4], stride 8), + the trig constants
// (deg->rad, -225.0f phase bias, the 350.0-step*0.002*350.0 shrink curve).
extern "C" i32 g_5e8fe8[];
extern "C" float g_5e93b4;  // -225.0f  (phase bias, fsub'd)
extern "C" double g_5e93b8; // 0.017453292  (pi/180)
extern "C" double g_5e93c0; // 0.002
extern "C" double g_5e93c8; // 350.0

// The bonus state object (CMultiBootyState+0x2f8): flags @+0x8, a scroll phase @+0x5c.
SIZE_UNKNOWN(CBootyBonusState);
struct CBootyBonusState {
    char m_pad00[0x8];
    i32 m_8; // +0x08 flags
    char m_pad0c[0x5c - 0xc];
    i32 m_5c; // +0x5c scroll phase
};

// The draw object (g_gameReg+0x7c) the frame-ready gate runs on (FrameReady @0xfcd70
// IS CBattlezData::InBounds; cast at the call).
SIZE_UNKNOWN(CBootyDrawObj);
struct CBootyDrawObj {};
struct CBootyMusicHost; // the g_gameReg+0x30 music host (defined below)
SIZE_UNKNOWN(CBootyGameReg);
struct CBootyGameReg {
    char m_pad00[0x30];
    CBootyMusicHost* m_30; // +0x30 music host
    char m_pad34[0x7c - 0x34];
    CBootyDrawObj* m_7c; // +0x7c draw object
    char m_pad80[0x11c - 0x80];
    i32 m_11c; // +0x11c configured music item
};
#define BOOTY_REG ((CBootyGameReg*)g_gameReg)
// The Lookup output ("BOOTY_LOOP"/"BOOTY_PERFECT" cue entry): a player @+0x10 (the
// ConfigureItem `this`), and a draw-clock gate (last @+0x14, interval @+0x18).
SIZE_UNKNOWN(CBootyFound);
struct CBootyFound {
    char m_pad00[0x10];
    CSoundCueMgr* m_10; // +0x10 player (ConfigureItem this)
    i32 m_14;           // +0x14 last draw-clock
    i32 m_18;           // +0x18 interval
};
// The music host chain g_gameReg->m_30->{m_28->m_30 gate, Lookup map @m_28+0x10}.
SIZE_UNKNOWN(CBootyMusicHost);
struct CBootyMusicHost {
    char m_pad00[0x28];
    struct M28 {
        char m_pad00[0x30];
        void* m_30; // +0x30 gate (non-null => skip); the lookup map is at this+0x10
    }* m_28;
};

// ReleaseResources teardown chain: m_4 (owner) -> m_60 sub-object (a CMoviePlayer the
// booty state owns; its ~CMoviePlayer runs before BaseCleanup).
SIZE_UNKNOWN(CBootyM4Sub);
struct CBootyM4Sub {};
SIZE_UNKNOWN(CBootyOwnerView);
struct CBootyOwnerView {
    char m_pad00[0x60];
    CBootyM4Sub* m_60; // +0x60
};

// ---------------------------------------------------------------------------
// CBootyState::ReleaseResources() (slot 2 / +0x8, 0x18c90): release the BOOTY resource
// set, then chain BaseCleanup. The `m_c` view's leaf registry (m_28) holds a pooled
// resource (Free/Stop if set) and releases two named sound sets; the name registry
// (m_10) releases two named sprite sets. Also reached directly from ~CBootyState.
RVA(0x00018c90, 0x72)
void CBootyState::ReleaseResources() {
    CViewPooledRes* r = ((CSoundRegistry*)m_c->m_28)->m_2c;
    if (r) {
        ((SoundStream*)r)->Stop();
    }
    ((CDDrawSubMgrLeafScan*)m_c->m_28)->RemoveKeysEqual_157c70("BOOTY", "_");
    ((CDDrawSubMgrLeafScan*)m_c->m_28)->RemoveKeysEqual_157c70("GRUNTZ_WANDGRUNT", "_");
    ((CDDrawWorkerRegistry*)m_c->m_10)->RemoveKeysEqual_155360("BOOTY", "_");
    ((CDDrawWorkerRegistry*)m_c->m_10)->RemoveKeysEqual_155360("GRUNTZ_GOKARTGRUNT", "_");
    ((CGameModeBase*)this)->BaseCleanup();
}

// ---------------------------------------------------------------------------
// CBootyState::Vslot09 (slot 9, 0x18d30) - the booty "bg" activation tick: hide the
// cursor, fade in the "bg" title, kick the render worker apply (m_c->m_drawTarget->Flush)
// + the booty page timer, and (when the BOOTY_LOOP ambient sound entry exists and is
// enabled by g_sndEnabled) re-trigger it on a rate-limited timer keyed off the
// g_killCueClock frame counter vs the entry's last-played stamp + interval.
// @early-stop
// regalloc wall (~95%): retail holds `set` (reg->m_world->m_28) in eax and the play entry
// `res` live in eax with no reload; the /O2 recompile pins `set` in ecx and spills/reloads
// `res` at the Play call. Logic + all externs/strings named/folded.
RVA(0x00018d30, 0xcd)
i32 CBootyState::Vslot09(i32) {
    while (ShowCursor(FALSE) >= 0)
        ;
    if (!FadeInTitle("bg", 0, 0, 0, 0, 1)) {
        return 0;
    }
    ((CDDrawSubMgrPages*)m_c->m_drawTarget)->Method_158ee0();
    BuildPage(0x50, 0x3e8, 0, 1);

    BzGameReg* reg = g_gameReg;
    BootySndSet* set = reg->m_world->m_soundSet;
    i32 token = reg->m_soundToken;
    if (set->m_activeGuard == 0) {
        BootySndEntry* res = 0;
        ((CMapStringToPtr*)&set->m_table)->Lookup("BOOTY_LOOP", (void*&)res);
        if (res != 0 && g_sndEnabled != 0) {
            u32 now = g_killCueClock;
            if (now - res->m_lastPlayed >= res->m_interval) {
                res->m_lastPlayed = now;
                res->m_player->ConfigureItem(token, 0, 0, 1);
            }
        }
    }
    return 1;
}

// CBootyState::FrameSlot28 (slot 10 / +0x28, 0x18e40): the BOOTY_LOOP ambient
// voice-loop driver. Look up the "BOOTY_LOOP" cue in the m_c cue registry; if its
// DirectSound buffer is playing, re-trigger it (CloneAndPlay) and spin the audio-kill
// voice reaper until the buffer stops. Returns 1.
// @early-stop
// regalloc register-assignment wall (~79%): logic byte-exact, but retail keeps the
// looked-up cue in esi (this in edi), whereas cl allocates this->esi and strength-
// reduces &found->m_10 into edi. A pure esi<->edi coin-flip on two loop-live base
// pointers; not steerable.
RVA(0x00018e40, 0x81)
i32 CBootyState::FrameSlot28(i32) {
    CObject* obj = 0;
    CMapStringToOb* map = (CMapStringToOb*)((char*)m_c->m_28 + 0x10);
    map->Lookup("BOOTY_LOOP", obj);
    LeafCue* found = (LeafCue*)obj;
    if (found && ((DirectSoundMgr*)found->m_10)->IsPlaying()) {
        ((DirectSoundMgr*)found->m_10)->CloneAndPlay(0, 0x1f4, 1);
        while (((DirectSoundMgr*)found->m_10)->IsPlaying()) {
            if (m_c->m_28->m_2c != 0) {
                m_c->m_28->m_2c->PurgeVoiceList(-1);
            }
        }
    }
    return 1;
}

// CMultiBootyState::BuildWarpStoneGlitterAnimation (0x19540): build 4 "DoNothing" warp-
// letter animations through the glitter factory (g_gameReg viewed as CGlitterMgr:
// m_world->m_8), stash them in the +0x1ec ptr array, set/clear their active bit, then
// build the trailing "SimpleAnimation" glitter sprite.
// @early-stop
// 88.1%: logic byte-faithful. Residual is the branchless-select codegen for the per-letter
// `(i != m_letterIdx) ? 1 : 3` kind + the per-iteration g_gameReg reload scheduling.
RVA(0x00019540, 0x12a)
i32 CMultiBootyState::BuildWarpStoneGlitterAnimation() {
    CGlitterMgr* reg = (CGlitterMgr*)g_gameReg;
    // The +0x1ec and +0x204 arrays overlap; reach the letter-sprite array by offset
    // (naming-independent, campaign doctrine) - the rest are real CMultiBootyState members.
    CGameObject** slot = (CGameObject**)((char*)this + 0x1ec);
    m_radius = 0xc8;
    m_letterIdx = (reg->m_7c->m_4 - 1) % 4;
    m_angleStep = 0;
    m_scratchX = 0;
    m_1e8 = 0;
    for (i32 i = 0; i < 4; i++) {
        CGameObject* a =
            ((CGlitterMgr*)g_gameReg)
                ->m_world->m_8->CreateSprite(0, 0, 0, (i != m_letterIdx) ? 1 : 3, "DoNothing", 3);
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
    CGameObject* g =
        ((CGlitterMgr*)g_gameReg)->m_world->m_8->CreateSprite(0, 0, 0, 4, "SimpleAnimation", 3);
    m_cursorLetter = g;
    if (g == 0) {
        return 0;
    }
    g->ApplyName("GAME_GLITTERGOLD");
    m_cursorLetter->ApplyLookupGeometry("GAME_CYCLE100", 0);
    return 1;
}

// CMultiBootyState::StepGlitterAnim() (0x196c0): the glitter/spawn positioner. With
// m_1b4 set it snaps the eight letter sprites to the static spawn table; otherwise it
// walks a sine spiral (radius m_radius, angle (m_angleStep+225)*pi/180), advances the
// step by 5, shrinks the radius along the 350.0-step*0.002*350.0 curve, then latches the
// trailing sprite's spawn flag when the radius reaches zero.
// @early-stop
// regalloc wall (~80%): the float branch is byte-exact (sin/cos/__ftol chain matches);
// the residual is the two integer letter-loops + the final latch block, a pure
// register-allocation coin-flip (docs/patterns/zero-register-pinning.md).
RVA(0x000196c0, 0x1d3)
void CMultiBootyState::StepGlitterAnim() {
    if (m_1b4) {
        if (m_letterIdx >= 0) {
            i32* tbl = g_5e8fe8 + 1; // walks: tbl[-1]=x, tbl[0]=y; advances by 2
            CGameObject** ap = (CGameObject**)((char*)this + 0x1ec); // walks arr1ec by 1
            for (i32 i = 0; i <= m_letterIdx; i++) {
                CGameObject* e = *ap;
                e->m_screenX = tbl[-1];
                e = *ap;
                e->m_screenY = tbl[0];
                e = *ap;
                if (e->m_latchedAnimId != 1) {
                    e->m_latchedAnimId = 1;
                    e->m_flags |= 0x20000;
                }
                ap++;
                tbl += 2;
            }
        }
        m_cursorLetter->m_screenX = g_5e8fe8[m_letterIdx * 2];
        m_cursorLetter->m_screenY = g_5e8fe8[m_letterIdx * 2 + 1];
        return;
    }

    i32 step = m_angleStep;
    i32 idx = m_letterIdx;
    double r = (float)m_radius; // load (float)m_radius first; shared across sin/cos terms
    double ang = ((float)step - g_5e93b4) * g_5e93b8;
    m_scratchX = (i32)(sin(ang) * r + (float)g_5e8fe8[idx * 2]);
    m_1e8 = (i32)(cos(ang) * r + (float)g_5e8fe8[idx * 2 + 1]);
    m_angleStep = step + 5;
    m_radius = (i32)(g_5e93c8 - (float)(step + 5) * g_5e93c0 * g_5e93c8);

    // Snap the leading sprites (0..m_letterIdx-1) to their static table coords (pointer walk).
    i32 i = 0;
    CGameObject** arr1ec = (CGameObject**)((char*)this + 0x1ec);
    if (idx > 0) {
        i32* tbl = g_5e8fe8 + 1;   // ecx: tbl[-1]=x, tbl[0]=y
        CGameObject** ap = arr1ec; // eax
        do {
            CGameObject* e = *ap;
            i++;
            ap++;
            e->m_screenX = tbl[-1];
            e = ap[-1];
            e->m_screenY = tbl[0];
            tbl += 2;
        } while (i < m_letterIdx);
    }
    // The trailing sprite + the i'th (== m_letterIdx) sprite get the computed scratch coords.
    m_cursorLetter->m_screenX = m_scratchX;
    m_cursorLetter->m_screenY = m_1e8;
    arr1ec[i]->m_screenX = m_scratchX;
    arr1ec[i]->m_screenY = m_1e8;

    MoveLettersByDir();

    if (m_radius == 0) {
        CGameObject* e = arr1ec[i];
        if (e->m_latchedAnimId != 1) {
            e->m_latchedAnimId = 1;
            e->m_flags |= 0x20000;
        }
    }
}

// CMultiBootyState::MoveLettersByDir() (0x19b90): if the anim-mode latch (m_1b4) is set,
// OR the spawn bit into all eight letters' flags; otherwise step each of the eight
// letters one cell (+/-4 px) along its compass direction (an 8-way jump table), flagging
// any that leave the [0,0x280]x[0,0x1e0] play field.
// @early-stop
// regalloc wall (~60%): logic/offsets/control-flow/jump-table all match; the residual is
// pure register allocation (docs/patterns/zero-register-pinning.md). The 8-way switch
// body itself is byte-aligned.
RVA(0x00019b90, 0xd7)
void CMultiBootyState::MoveLettersByDir() {
    if (m_1b4) {
        CGameObject** p = (CGameObject**)((char*)this + 0x204);
        i32 n = 8;
        do {
            CGameObject* e = *p;
            p++;
            e->m_stateFlags |= 1;
        } while (--n);
        return;
    }
    CGameObject** p = (CGameObject**)((char*)this + 0x204);
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

// CMultiBootyState::CheckPerfectBonus() (0x1c0f0): once the frame-ready gate fires,
// drive the bonus state's scroll phase (m_bonusState->m_5c): on the wrap value (-0x82)
// play the "BOOTY_PERFECT" cue on the draw-clock window; past 0x302 latch the done flag
// (m_8 |= 0x10000); otherwise advance the phase by 0xa. Returns 1.
RVA(0x0001c0f0, 0xd5)
i32 CMultiBootyState::CheckPerfectBonus() {
    if (!((CBattlezData*)BOOTY_REG->m_7c)->InBounds(-1)) {
        return 1;
    }
    CBootyBonusState* st = m_bonusState;
    i32 phase = st->m_5c;
    if (phase == (i32)0xffffff7e) {
        CBootyMusicHost* host = BOOTY_REG->m_30;
        i32 item = BOOTY_REG->m_11c;
        CBootyMusicHost::M28* m28 = host->m_28;
        if (m28->m_30 == 0) {
            void* found = 0;
            CMapStringToOb* map = (CMapStringToOb*)((char*)m28 + 0x10);
            map->Lookup("BOOTY_PERFECT", (CObject*&)found);
            if (found && g_61ab20 != 0) {
                CBootyFound* p = (CBootyFound*)found;
                if (g_6bf3c0 - (u32)p->m_14 >= (u32)p->m_18) {
                    p->m_14 = g_6bf3c0;
                    ((CSoundCueMgr*)p->m_10)->ConfigureItem(item, 0, 0, 0);
                }
            }
        }
    }
    if (phase >= 0x302) {
        m_bonusState->m_8 |= 0x10000;
        return 1;
    }
    m_bonusState->m_5c = phase + 0xa;
    return 1;
}

// CBootyState::Render (slot 5 / +0x14, 0x1c210): the per-frame bonus-state draw (1205B).
// Still a reconstruction target - stub body marks the slot.
// @confidence: med
// @source: string-xref
// @stub
RVA(0x0001c210, 0x4b5)
i32 CBootyState::Render() {
    return 0;
}

// CBootyState::Vslot06 (slot 6 / +0x18, 0x1ce10): boolify the class's slot-3
// active/ready virtual (Vfunc3) - return 1 iff ready, else 0.
RVA(0x0001ce10, 0xc)
i32 CBootyState::Vslot06() {
    return Vfunc3() != 0;
}

// CMultiBootyState::ReadyAndPaint() (0x1ce30): gate on the active/ready virtual (CState
// slot 3 / +0xc); when ready, run the per-frame Paint and return the normalized result;
// otherwise return the (zero) gate result.
RVA(0x0001ce30, 0x1d)
i32 CMultiBootyState::ReadyAndPaint() {
    if (Vfunc3() == 0) {
        return 0;
    }
    return Paint() != 0;
}

// CBootyState::Vslot0e (0x1d3e0, vtable slot 14) and Vslot11 (0x1d400, slot 17): the
// same tail-forward to the shared booty-grunt idle-animation builder (0x1ce60) as
// Vslot0c, but with the slot-14/17 (int,int,int) arg frame. Reloc-masked call.
RVA(0x0001d3e0, 0x8)
i32 CBootyState::Vslot0e(i32, i32, i32) {
    return BuildBootyGruntIdleAnimation();
}

RVA(0x0001d400, 0x8)
i32 CBootyState::Vslot11(i32, i32, i32) {
    return BuildBootyGruntIdleAnimation();
}

// CBootyState::Vslot0c (0x1d420, vtable slot 12): tail-forward to the shared booty-grunt
// idle-animation builder (0x1ce60). Homed out-of-line as the real virtual (matcher-5); the
// call is reloc-masked, so the shared-body owner name is code-neutral.
RVA(0x0001d420, 0x8)
i32 CBootyState::Vslot0c(i32, i32) {
    return BuildBootyGruntIdleAnimation();
}

// CBootyState::StateOnEnter (0x1d440): the booty state-enter driver - engine-label
// backlog stub (non-virtual; vtable-neutral).
// @confidence: med
// @source: decomp-xref
// @stub
RVA(0x0001d440, 0xd7d)
void CBootyState::StateOnEnter() {}

// CMultiBootyState::ReleaseResources() (slot 2 / +0x8, 0x1e520): free the leaf-registry
// pooled resource (if set), release the "BOOTY" set on the leaf registry, run a teardown
// on the owner's m_4->m_60 sub-object (~CMoviePlayer), then chain BaseCleanup.
// @early-stop
// near-exact (~98.5%): structure/offsets/calls all match; the sole non-reloc residual is
// the m_4 deref landing in eax vs retail's edx (single-register coin-flip).
RVA(0x0001e520, 0x3e)
void CMultiBootyState::ReleaseResources() {
    CViewPooledRes* r = ((CSoundRegistry*)m_c->m_28)->m_2c;
    if (r) {
        ((SoundStream*)r)->Stop();
    }
    ((CDDrawSubMgrLeafScan*)m_c->m_28)->RemoveKeysEqual_157c70("BOOTY", "_");
    ((CMoviePlayer*)((CBootyOwnerView*)m_4)->m_60)->~CMoviePlayer();
    ((CGameModeBase*)this)->BaseCleanup();
}

// CMultiBootyState::Vslot09() (slot 9 / +0x24, 0x1e570): on entry build the "multi"
// title page (fade + page) then, if the menu is live, push the "BOOTY_LOOP" cue into the
// player on the draw-clock window. Returns 1.
RVA(0x0001e570, 0xb4)
i32 CMultiBootyState::Vslot09(i32) {
    i32 ok = FadeInTitle("multi", 0, 0, 0, 0, 1);
    if (!ok) {
        return ok; // eax already 0 (the FadeInTitle result) - no xor/mov re-materialize
    }
    ((CDDrawSubMgrPages*)m_c->m_drawTarget)->Method_158ee0();
    BuildPage(0x50, 0x3e8, 0, 1);

    CBootyMusicHost* host = BOOTY_REG->m_30;
    i32 item = BOOTY_REG->m_11c;
    CBootyMusicHost::M28* m28 = host->m_28;
    if (m28->m_30 == 0) {
        void* found = 0;
        CMapStringToOb* map = (CMapStringToOb*)((char*)m28 + 0x10);
        map->Lookup("BOOTY_LOOP", (CObject*&)found);
        if (found && g_61ab20 != 0) {
            CBootyFound* p = (CBootyFound*)found;
            if (g_6bf3c0 - (u32)p->m_14 >= (u32)p->m_18) {
                p->m_14 = g_6bf3c0;
                ((CSoundCueMgr*)p->m_10)->ConfigureItem(item, 0, 0, 1);
            }
        }
    }
    return 1;
}

// CMultiBootyState::FrameSlot28 (slot 10 / +0x28, 0x1e660): identical BOOTY_LOOP
// voice-loop driver to CBootyState::FrameSlot28 (0x18e40) - the two sibling states
// share the ambient-loop spin.
// @early-stop
// same regalloc esi<->edi coin-flip wall as CBootyState::FrameSlot28 (0x18e40).
RVA(0x0001e660, 0x81)
i32 CMultiBootyState::FrameSlot28(i32) {
    CObject* obj = 0;
    CMapStringToOb* map = (CMapStringToOb*)((char*)m_c->m_28 + 0x10);
    map->Lookup("BOOTY_LOOP", obj);
    LeafCue* found = (LeafCue*)obj;
    if (found && ((DirectSoundMgr*)found->m_10)->IsPlaying()) {
        ((DirectSoundMgr*)found->m_10)->CloneAndPlay(0, 0x1f4, 1);
        while (((DirectSoundMgr*)found->m_10)->IsPlaying()) {
            if (m_c->m_28->m_2c != 0) {
                m_c->m_28->m_2c->PurgeVoiceList(-1);
            }
        }
    }
    return 1;
}

// CMultiBootyState::QueryGruntSlots() (0x1ecf0): scan the four per-player registry
// records (g_gameReg+0x174, stride 0x238); the first whose +0x4 is set but +0x0 is
// clear yields its +0x150 field; none -> 0. (`this`/ecx is unused.)
// @early-stop
// 1-instruction-order wall (~94%): the `je`/test/loop shape is byte-exact; the sole
// residual is the loop tail's `inc ecx` vs `add eax,0x238` order (a /O2 scheduling
// coin-flip; the natural for-loop form is kept).
RVA(0x0001ecf0, 0x2a)
i32 CMultiBootyState::QueryGruntSlots() {
    char* base = (char*)g_gameReg;
    i32 i = 0;
    char* rec = base + 0x174;
    for (; i < 4; i++) {
        if (*(i32*)(rec + 4) != 0 && *(i32*)rec == 0) {
            return *(i32*)(rec - 0x24);
        }
        rec += 0x238;
    }
    return 0;
}

// ===========================================================================
// CBattleStatsView::DrawBattleStats (0x1ed30; re-homed from the former drawbattlestats
// unit, waveP - TU_MIGRATION MOVE row `0x01ed30 DrawBattleStats@CBattleStatsView
// drawbattlestats -> 0x1c0f0 bootystateactivate`; called here as OnActivated). The
// in-game BATTLE-STATZ scoreboard renderer (sibling of DrawDebugStats): 6 numeric
// stat columns per active player, 7 category labels, per-player team-colour name, the
// title. The reused CString + colour-name temp give it the /GX frame. g_mgr / the view
// are engine classes reached by raw this+offset; every callee reloc-masked external;
// CopyRect hoisted through a data fn-ptr global.
// ===========================================================================
// DrawStatText (0x1f00 -> 0x1154b0): __cdecl(ctx, text, rect, y, flag, b, g, r, a9).
extern "C" void
DrawStatText(void* ctx, CString* text, RECT* rc, i32 y, i32 flag, i32 b, i32 g, i32 r, i32 a9);
// GetColorName (0x3e54): NRV CString* into `out`.
CString* GetColorName(CString* out);

// CopyRect USER32 import hoisted through a data fn-ptr global (retail loads it once
// into ebp and calls it ~13x).
DATA(0x002c44bc)
extern void(WINAPI* g_pCopyRect)(RECT* dst, const RECT* src); // 0x6c44bc

// The per-column source-rect tables (RECT[] in .data). Indexed by player/category.
DATA(0x001e9178)
extern RECT g_col1Rects[]; // 0x5e9178
DATA(0x001e91b8)
extern RECT g_col2Rects[]; // 0x5e91b8
DATA(0x001e91f8)
extern RECT g_col3Rects[]; // 0x5e91f8
DATA(0x001e9238)
extern RECT g_col4Rects[]; // 0x5e9238
DATA(0x001e9278)
extern RECT g_col5Rects[]; // 0x5e9278
DATA(0x001e92b8)
extern RECT g_col6Rects[]; // 0x5e92b8
DATA(0x001e92f8)
extern RECT g_colorRects[]; // 0x5e92f8 (4 team-colour rects)
DATA(0x001e9338)
extern RECT g_labelRects[]; // 0x5e9338 (7 category-label rects)

// The per-player stat block reached through g_mgr->m_7c; SumWinRow (0x1230) folds
// the win-row totals for a player.
DATA(0x0024556c)
extern CGameRegistry* g_mgr; // *0x64556c

class CBattleStatsView {
public:
    void DrawBattleStats(); // 0x1ed30

    char m_pad00[0xc];
    void* m_c; // +0x0c  draw context
};

static __inline i32 sumRun(CBattlezData* base, i32 off, i32 n) {
    i32* p = (i32*)((char*)base + off);
    i32 s = 0;
    i32 k;
    for (k = 0; k < n; k++) {
        s += p[k];
    }
    return s;
}

// @source: string-xref
// @early-stop
// induction-variable strength-reduction wall (~85%): the whole structure, all
// externs/strings, the /GX frame size and register roles match retail. The residual is
// the 6-column player loop: retail strength-reduces the per-column offsets into a
// specific induction-variable/register layout; the /O2 recompile derives an
// equivalent-but-differently-registered set cascading a scheduling/operand-byte drift.
// Documented regalloc wall, not source-steerable (cf. docs/patterns/
// loop-invariant-multiply-strength-reduce-vs-memreread.md).
RVA(0x0001ed30, 0x549)
void CBattleStatsView::DrawBattleStats() {
    CString s;
    RECT rc;
    void(WINAPI * copyRect)(RECT*, const RECT*) = g_pCopyRect;
    i32 i;
    i32 c;

    // Loop 1: 6 numeric stat columns per active player.
    for (i = 0; i < 4; i++) {
        if (*(i32*)((char*)g_mgr + 0x178 + i * 0x238) != 0) {
            s.Format("%d", sumRun((CBattlezData*)g_mgr->m_scoreHud, 0x348 + i * 0x10, 4));
            copyRect(&rc, &g_col1Rects[i]);
            DrawStatText(m_c, &s, &rc, 0x78, 1, 0xff, 0xff, 0, 1);

            s.Format("%d", sumRun((CBattlezData*)g_mgr->m_scoreHud, 0x2d8 + i * 0x1c, 7));
            copyRect(&rc, &g_col2Rects[i]);
            DrawStatText(m_c, &s, &rc, 0x78, 1, 0xff, 0xff, 0, 1);

            s.Format("%d", sumRun((CBattlezData*)g_mgr->m_scoreHud, 0x238 + i * 0x28, 10));
            copyRect(&rc, &g_col3Rects[i]);
            DrawStatText(m_c, &s, &rc, 0x78, 1, 0xff, 0xff, 0, 1);

            s.Format("%d", sumRun((CBattlezData*)g_mgr->m_scoreHud, 0xd8 + i * 0x58, 22));
            copyRect(&rc, &g_col4Rects[i]);
            DrawStatText(m_c, &s, &rc, 0x78, 1, 0xff, 0xff, 0, 1);

            s.Format("%d", *(i32*)((char*)(CBattlezData*)g_mgr->m_scoreHud + 0x48 + i * 4));
            copyRect(&rc, &g_col5Rects[i]);
            DrawStatText(m_c, &s, &rc, 0x78, 1, 0xff, 0xff, 0, 1);

            s.Format("%d", ((CBattlezData*)g_mgr->m_scoreHud)->SumWinRow(i));
            copyRect(&rc, &g_col6Rects[i]);
            DrawStatText(m_c, &s, &rc, 0x78, 1, 0xff, 0xff, 0, 1);
        }
    }

    // Loop 2: category labels.
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
        DrawStatText(m_c, &s, &rc, 0x78, 1, 0xff, 0xff, 0, 1);
    }

    // Colour loop: team-colour name per active player, drawn in that colour.
    for (i = 0; i < 4; i++) {
        if (*(i32*)((char*)g_mgr + 0x178 + i * 0x238) != 0) {
            i32 color;
            switch (*(i32*)((char*)g_mgr + 0x158 + i * 0x238)) {
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
            CString cn;
            s.Format("%s", (const char*)*GetColorName(&cn));
            copyRect(&rc, &g_colorRects[i]);
            DrawStatText(
                m_c,
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

    // Title.
    s.Format("BATTLE STATZ");
    rc.left = 0x96;
    rc.top = 0xf;
    rc.right = 0x280;
    rc.bottom = 0x73;
    DrawStatText(m_c, &s, &rc, 0x82, 1, 0xff, 0xff, 0, 1);
}

SIZE_UNKNOWN(StatArray);
SIZE_UNKNOWN(CBattleStatsView);

// ---------------------------------------------------------------------------
// CMultiBootyState::Render (slot 5, +0x14, 0x1f480) - the booty-countdown per-frame
// draw. Bail (ReportError 0x8006/0x459) when the frame surface is lost and the input
// poll did not consume the frame. Draw the battle-stats scoreboard once (the m_1b8
// latch flips 0x64 -> 0xc7), run the sprite worker's frame apply/present, format the
// remaining time as a HUD line (H:MM:SS or M:SS), flip the frame + blit the page onto
// the target, and purge finished sound voices. /GX (the CString temp).
RVA(0x0001f480, 0x1e9)
i32 CMultiBootyState::Render() {
    IDirectDrawSurface* frameSurf = m_c->m_drawTarget->m_10->m_2c->m_8;
    if (frameSurf == 0 || frameSurf->IsLost() != 0) {
        if (InputVirtual() == 0) {
            m_4->ReportError(0x8006, 0x459);
            return 0;
        }
    }
    if (m_1b8 == 0x64) {
        OnActivated(); // 0x1ed30 (== CBattleStatsView::DrawBattleStats)
        m_1b8 = 0xc7;
    }
    m_c->m_8->FrameBegin(1);
    m_c->m_8->FramePresent(m_c->m_drawTarget->m_14);

    u32 secs = g_gameReg->m_7c->m_elapsedMs / 1000; // signed /1000, then unsigned H:M:S
    CString s;
    RECT rc;
    SetRect(&rc, 8, 0x41, 0xcb, 0xae);
    if (secs / 3600 != 0) {
        s.Format("%d:%2.2d:%2.2d", secs / 3600, (secs / 60) % 60, secs % 60);
    } else {
        s.Format("%d:%2.2d", secs / 60, secs % 60);
    }
    ShowHudMessageAlt((HudMsgSink*)m_c, (i32)&s, (i32)&rc, 0x6e, 1, 0xff, 0xff, 0, 1);

    CDrawTarget* dt = m_c->m_drawTarget;
    dt->m_10->m_2c->Flip(0);
    dt->m_14->m_2c->BltFast(0, 0, dt->m_18->m_2c, &dt->m_18->m_1c, 0x10);
    if (m_c->m_28->m_2c != 0) {
        m_c->m_28->m_2c->PurgeVoiceList(-1); // SoundDevice base method (inherited)
    }
    return 1;
}

// ---------------------------------------------------------------------------
// CMultiBootyState::InputVirtual (slot 8, 0x1f6f0) - on state activation: chain the base
// image-load gate, hide the cursor, resolve+register the BOOTY/GRUNTZ/LEVEL "IMAGEZ" sets
// through the CSpriteFactoryHolder image registry (LoadNamespace +0x4c), fade in the "multi" title, run
// the post-activate hook, then kick the render worker apply + the page timer.
RVA(0x0001f6f0, 0x10b)
i32 CMultiBootyState::InputVirtual() {
    if (!BaseOnActivate()) {
        return 0;
    }

    while (ShowCursor(FALSE) >= 0)
        ;

    void* tree = SymTab2c()->ResolvePath("IMAGEZ");
    if (!tree) {
        return 0;
    }
    CImageRegistry* reg = m_c->m_10;
    if (reg->LoadNamespace(tree, "BOOTY", "_") == -1) {
        return 0;
    }

    tree = ((CSymTab*)m_gruntzBank)->ResolvePath("IMAGEZ");
    if (!tree) {
        return 0;
    }
    reg = m_c->m_10;
    if (reg->LoadNamespace(tree, "GRUNTZ", "_") == -1) {
        return 0;
    }

    tree = ((CSymTab*)m_levelBank)->ResolvePath("IMAGEZ");
    if (!tree) {
        return 0;
    }
    reg = m_c->m_10;
    if (reg->LoadNamespace(tree, "LEVEL", "_") == -1) {
        return 0;
    }

    if (!FadeInTitle("multi", 0, 0, 0, 0, 1)) {
        return 0;
    }

    OnActivated();
    ((CDDrawSubMgrPages*)m_c->m_drawTarget)->Method_158ee0();
    BuildPage(0x50, 0x3e8, 0, 1);
    return 1;
}

// CMultiBootyState::Vslot06 (slot 6 / +0x18, 0x1f850): boolify the class's slot-3
// active/ready virtual (Vfunc3) - return 1 iff ready, else 0.
RVA(0x0001f850, 0xc)
i32 CMultiBootyState::Vslot06() {
    return Vfunc3() != 0;
}

// ---------------------------------------------------------------------------
// CMultiBootyState::Vslot07 (slot 7 / +0x1c, 0x1f870): gate on the slot-3 ready
// virtual (Vfunc3); when ready run the non-virtual paint (Paint, 0xfac70) and return
// its normalized result, else 0. (Was the @orphan CGuardedDispatch1f870 view; the
// vtable slot-7 attribution -- find_holding(0x1f870) == CMultiBootyState:7 -- recovers
// its real identity, dissolving the view.)
// ---------------------------------------------------------------------------
RVA(0x0001f870, 0x1d)
i32 CMultiBootyState::Vslot07() {
    if (Vfunc3() == 0) {
        return 0;
    }
    return Paint() != 0;
}

// ---------------------------------------------------------------------------
// CMultiBootyState::PostCommandIfKey (0x1f8a0): if the one-shot battle-stats latch
// (m_1b8) reads 0xc7, post WM_COMMAND 0x8023 to the game window
// (g_gameReg->m_gameWnd->m_hwnd) via g_pPostMessageA; always return 1. __thiscall,
// no args. (Was the @identity-TODO PendingCmdKeyHost view; the slot-12/14/17
// forwarders below tail-call it with ecx = a CMultiBootyState `this`, and it reads
// m_1b8 (+0x1b8) -- that xref recovers it as this CMultiBootyState method.)
// ---------------------------------------------------------------------------
RVA(0x0001f8a0, 0x30)
i32 CMultiBootyState::PostCommandIfKey() {
    if (m_1b8 == 0xc7) {
        g_pPostMessageA(g_gameReg->m_gameWnd->m_hwnd, 0x111, 0x8023, 0);
    }
    return 1;
}

// CMultiBootyState slots 14/17/12 (0x1f8e0 / 0x1f900 / 0x1f920): each tail-forwards
// its (reloc-masked) call to PostCommandIfKey on `this`; slots 14/17 with the
// (int,int,int) arg frame (ret 0xc), slot 12 with (int,int) (ret 8).
RVA(0x0001f8e0, 0x8)
i32 CMultiBootyState::Vslot0e(i32, i32, i32) {
    return PostCommandIfKey();
}

RVA(0x0001f900, 0x8)
i32 CMultiBootyState::Vslot11(i32, i32, i32) {
    return PostCommandIfKey();
}

RVA(0x0001f920, 0x8)
i32 CMultiBootyState::Vslot0c(i32, i32) {
    return PostCommandIfKey();
}

// ---------------------------------------------------------------------------
// The EH-framed `??1` destructors (slot 0). Each re-stamps its own vtable, runs the
// slot-2 release (statically bound), re-stamps CState, chains BaseCleanup (compiler-
// emitted). With the CState ctor they anchor the CBootyState / CMultiBootyState vtable +
// inline-virtual (Update) emission in this TU. /GX EH frame.
// ---------------------------------------------------------------------------

// CBootyState::~CBootyState() (`??1`, 0x8d440): run the booty teardown then chain base.
RVA(0x0008d440, 0x55)
CBootyState::~CBootyState() {
    ReleaseResources();
}

// CMultiBootyState::~CMultiBootyState() (`??1`, 0x8d510): run the booty teardown then
// chain the base (the vtable re-stamps fold into the compiler-emitted body).
RVA(0x0008d510, 0x55)
CMultiBootyState::~CMultiBootyState() {
    ReleaseResources();
}
