// BootyStateActivate.cpp - the CBootyState / CMultiBootyState per-frame activation
// slot bodies (C:\Proj\Gruntz), FOLDED onto the canonical GameMode.h siblings.
//
// Attribution is vtable-proven (see the retail state vtables):
//   0x18d30 = CBootyState      slot 9 (+0x24)  vtbl 0x5e9cec  (Vslot09)
//   0x1f6f0 = CMultiBootyState slot 8 (+0x20)  vtbl 0x5e9bdc  (InputVirtual / OnActivate2)
// The two are CONFIRMED-distinct siblings over CState (<Gruntz/GameMode.h>). The old
// per-TU CBootyState/CMultiBootyState views (with BootyAssetRoot @+0x0c, BootyRegistrar,
// BootyNamespace @+0x2c/+0x28/+0x30) are folded away here onto the CState + CDDrawSurfaceMgr facets:
//   - m_c (+0x0c)  == CDDrawSurfaceMgr (the shared render/resource context - see <Gruntz/View.h>).
//     The +0x04 loader is CDDrawSurfaceMgr's m_renderState->Flush
//     (0x158ee0); the +0x10 registrar is CDDrawSurfaceMgr's
//     m_imageRegistry->LoadNamespace (vtable slot +0x4c).
//   - the +0x2c/+0x28/+0x30 asset sources are CState::m_2c /
//     m_levelBank / m_gruntzBank (CResSource; LookupSet 0x13bae0).
// Only the g_gameReg world sound set below stays a local facet view - a separate
// object web from the +0x0c context (deferred canonical world-sound model). Only offsets
// / code bytes are load-bearing; every helper is a reloc-masked external.
#include <Dsndmgr/DirectSoundMgr.h>
#include <Gruntz/GameRegMfcPtr.h>
#include <Gruntz/SoundState.h> // g_sndEnabled/g_sndCueTag
#include <Bute/SymTab.h>
#include <Bute/SymParser.h>
#include <DDrawMgr/DDrawSubMgrPages.h>
#include <DDrawMgr/DDrawSubMgrLeafScan.h> // RemoveKeysEqual_157c70 (Booty/MultiBooty ReleaseResources)
#include <DDrawMgr/DDrawWorkerRegistry.h> // RemoveKeysEqual_155360 (CBootyState::ReleaseResources)
#include <DDrawMgr/DDSurface.h> // CMultiBootyState::Render: CDDSurface Flip/BltFast on the frame surfaces
#include <DDrawMgr/DDrawSurfacePair.h> // the CDDrawSubMgrPages pages (real class of m_10/m_14/m_18)
#include <Mfc.h>   // ShowCursor (reloc-masked); GameMode.h needs the afx umbrella
#include <ddraw.h> // CMultiBootyState::Render: real IDirectDrawSurface::IsLost dispatch (slot 24)
#include <math.h>  // sin/cos (StepGlitterAnim sine spiral)

#include <rva.h>
#include <Gruntz/BankMgr.h> // CResSource::LookupSet (CState::m_2c/m_levelBank/m_gruntzBank)
#include <Gruntz/GameMode.h> // canonical CBootyState/CMultiBootyState : CState + CDDrawSurfaceMgr facet
#include <Gruntz/GruntzMgr.h>         // CMultiBootyState::Render: CState::m_4 owner (ReportError)
#include <DDrawMgr/DDrawChildGroup.h> // CMultiBootyState::Render: m_c->m_childGroup frame-worker slots 9/10
#include <Gruntz/LeafCue.h>           // LeafCue (Booty/MultiBooty FrameSlot28 BOOTY_LOOP cue)
#include <Gruntz/UserLogic.h>   // CGameObject (glitter/letter sprites; StepGlitterAnim/BuildWarp)
#include <Gruntz/BattlezData.h> // CBattlezData::InBounds (CheckPerfectBonus frame-ready gate)
#include <Gruntz/WwdGameReg.h>  // WwdGameReg (g_gameReg; CheckPerfectBonus/Vslot09/QueryGruntSlots)
#include <Gruntz/GameRegistry.h> // CDDrawSurfaceMgr / CSndHost (CState::m_c draw+cue context)
#include <Io/MoviePlayer.h>      // CMoviePlayer (~; CMultiBootyState::ReleaseResources m_4->m_60)
// (FadeInTitle @0xfa1f0 and RetireScene @0xfa8f0 are now CState base methods via
//  <Gruntz/State.h>, called cast-free on the CBootyState/CMultiBootyState `this`.)
// NOTE: BzState::BuildBootyGruntIdleAnimation (0x1ce60) stays via GameMode.h's
// CBootyState decl (reloc-UNBOUND) - the proper bind needs <Gruntz/BzState.h>, blocked
// by the BzGameReg / *0x24556c view-conflation between this TU's local Booty* views and
// BzState.h's divergent Bz* shapes (a separate view-reconciliation task).

// This TU's shared .data literals (the 0x60ba44 / 0x60c5b8 runs, whose following
// bytes are this TU's "STATEZ_BOOTY" / "Cursez:" literals - owner-pool evidence).
// Lengths NULL-TERMINATOR-PROVEN from the retail bytes. s_assetKeyGame is the "GAME"
// asset-namespace root key GruntzMgr/Play/GameAssetNamespaces share; g_nameFmt is
// the "%s" display-name format CustomWorldDialog/PortalPath/GameApp share.
// (s_assetKeyGame @0x20ba44 / g_nameFmt @0x20c5b8 were FICTIONS -- invented names for
// cl's folded `??_C@_04IPHN@GAME@` / `??_C@_02DILL@?$CFs@` literal COMDATs.)

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
// The game-manager singleton (*0x24556c) is the RTTI-true CGruntzMgr (<Gruntz/
// GruntzMgr.h>), and every object this TU reaches through it already has a canonical
// class - so its objects map to the real classes, offset for offset:
//     BzGameReg / CGlitterMgr / CBootyGameReg  -> CGruntzMgr
//     BzGameWnd        (+0x04, HWND @+0x04)    -> CGameWnd     (WAP32::CGameMgr::m_gameWnd,
//                                                               <Wap32/Wap32.h>: m_hwnd @+0x04)
//     BootySndWorld / CBootyMusicHost /
//       CGlitterMgrM30 (all +0x30)             -> CWorldZ      (m_8 == CDDrawChildGroup* the
//                                                               glitter factory; m_28 == CSndHost*)
//     BootySndSet / CBootyMusicHost::M28       -> CSndHost     (<Gruntz/SoundCue.h>: the real
//                                                               ::CMapStringToPtr m_10 @+0x10 the
//                                                               BOOTY_* Lookup runs on, and the
//                                                               m_emitGate @+0x30 that gates it)
//     BootySndEntry / CBootyFound              -> LeafCue      (the Lookup's OWN result type -
//                                                               m_10 player / m_14 last-played /
//                                                               m_18 interval; the sibling
//                                                               FrameSlot28 in this very TU was
//                                                               already using LeafCue for the same
//                                                               map's "BOOTY_LOOP" hit)
//     BzGameClock / CGlitterMgrSet /
//       CBootyDrawObj  (all +0x7c)             -> CBattlezData (<Gruntz/BattlezData.h>: the +0x7c
//                                                               object by its own new-site proof;
//                                                               m_count @+0x04 is the glitter
//                                                               letter count, and FrameReady
//                                                               @0xfcd70 IS CBattlezData::InBounds)
//     CBootyOwnerView / CBootyM4Sub            -> CGruntzMgr   (CState::m_4; m_timer @+0x60)
// The booty countdown reads CBattlezData +0x10 as elapsed milliseconds where the battlez
// scoreboard reads it as a score accumulator: ONE field, two readers (the canonical name
// m_score is kept - the divergence is noted, not forked into a second class).
// USER32 PostMessageA reached through the game-owned IAT-style fn-ptr (ff 15 [ptr]);
// same global CGruntzMgr/Attract/Play bind. KeyHost::Check posts through it.
// extern "C" so the reloc emits _g_pPostMessageA - the canonical name bound @0x2c44c8
// (sbi_rectonly); the C++-mangled form ?::PostMessageA@@... never bound (silently dropped).
// Plain C++ extern: ?g_sndEnabled@@3HA is now the ONE name bound at 0x21ab20 (DEFINED in
// GruntzMgr.cpp, the owner TU). The old extern "C" spelling here carried a DATA pin that
// bound _g_sndEnabled and starved every C++-mangled reference in the tree.
extern "C" u32 g_killCueClock; // wrap-safe draw clock

// The scalar-deleting dtor's operator delete (declared so /GX tracks the EH state).
void operator delete(void*);

// The packed {x,y} spawn-coordinate table StepGlitterAnim indexes by m_letterIdx
// (x via [tbl], y via [tbl+4], stride 8). A GENUINE read-only global: bound to the RVA
// its name asserts (VA 0x5e8fe8 -> RVA 0x1e8fe8), so the delinker can resolve the
// references instead of leaving them dangling. Declared-only: the table's CONTENTS live
// in retail .rdata and are not reconstructed here (the reference is what matters).
DATA(0x001e8fe8)
extern "C" i32 g_bootyLetterCoords[];

// g_5e93b4/b8/c0/c8 are
// MSVC's FLOATING-POINT LITERAL POOL entries - the .rdata constants cl emits for the fp
// immediates in this very expression - which a previous pass mistook for game data and
// re-declared as extern "C" symbols that NOTHING defines (unresolvable at link, and
// invisible to objdiff because the references are reloc-masked). The devs wrote the
// literals; so do we. cl emits the identical pool entries and the identical fsub/fmul.
static const float kGlitterPhaseBias = -225.0f;  // was g_5e93b4 (fsub'd, hence negative)
static const double kDegToRad = 0.017453292;     // was g_5e93b8 (pi/180)
static const double kGlitterShrinkRate = 0.002;  // was g_5e93c0
static const double kGlitterStartRadius = 350.0; // was g_5e93c8

// (the CBootyBonusState view is GONE - there is no "bonus state object". +0x2f8 holds the
// BOOTY_PERFECT CGameObject sprite (CBootyState::m_bootyPerfectSprite), and the view's two
// fields were simply that sprite's own: m_8 == CGameObject::m_flags (+0x08), m_5c ==
// CGameObject::m_screenX (+0x5c). The "scroll phase" is literally its screen x.)

// ---------------------------------------------------------------------------
// CBootyState::ReleaseResources() (slot 2 / +0x8, 0x18c90): release the BOOTY resource
// set, then chain CState::ReleaseResources. The `m_c` view's leaf registry (m_28) holds a pooled
// resource (Free/Stop if set) and releases two named sound sets; the name registry
// (m_10) releases two named sprite sets. Also reached directly from ~CBootyState.
RVA(0x00018c90, 0x72)
void CBootyState::ReleaseResources() {
    SoundStream* r = m_c->m_soundRegistry->m_2c; // CSndHost::m_2c is already the real SoundStream*
    if (r) {
        r->Stop();
    }
    m_c->m_soundRegistry->RemoveKeysEqual_157c70("BOOTY", "_");
    m_c->m_soundRegistry->RemoveKeysEqual_157c70("GRUNTZ_WANDGRUNT", "_");
    m_c->m_imageRegistry->RemoveKeysEqual_155360("BOOTY", "_");
    m_c->m_imageRegistry->RemoveKeysEqual_155360("GRUNTZ_GOKARTGRUNT", "_");
    CState::ReleaseResources(); // 0xfa150 (chain the base slot-2 teardown; direct)
}

// ---------------------------------------------------------------------------
// CBootyState::Vslot09 (slot 9, 0x18d30) - the booty "bg" activation tick: hide the
// cursor, fade in the "bg" title, kick the render worker apply (m_c->m_drawTarget->Flush)
// + the booty page timer, and (when the BOOTY_LOOP ambient sound entry exists and is
// enabled by g_sndEnabled) re-trigger it on a rate-limited timer keyed off the
// g_killCueClock frame counter vs the entry's last-played stamp + interval.
// @early-stop
// regalloc wall (~95%): retail holds `set` (reg->m_world->m_soundRegistry) in eax and the play entry
// `res` live in eax with no reload; the /O2 recompile pins `set` in ecx and spills/reloads
// `res` at the Play call. Logic + all externs/strings named/folded.
RVA(0x00018d30, 0xcd)
i32 CBootyState::Vslot09(i32) {
    while (ShowCursor(FALSE) >= 0)
        ;
    if (!FadeInTitle("bg", 0, 0, 0, 0, 1)) { // 0xfa1f0 (CState base method)
        return 0;
    }
    m_c->m_drawTarget->Method_158ee0();
    RetireScene(0x50, 0x3e8, 0, 1); // 0xfa8f0 CState::RetireScene (inherited, cast-free)

    CGruntzMgr* reg = g_gameReg;
    CSndHost* set = reg->m_world->m_soundRegistry;
    i32 token = reg->m_soundVolume; // +0x11c (the ambient sound token this facet reads)
    if (set->m_emitGate == 0) {
        LeafCue* res = 0;
        set->m_10.Lookup("BOOTY_LOOP", reinterpret_cast<void*&>(res));
        if (res != 0 && g_sndEnabled != 0) {
            u32 now = g_killCueClock;
            if (now - static_cast<u32>(res->m_14) >= static_cast<u32>(res->m_18)) {
                res->m_14 = now;
                res->m_10->ConfigureItem(token, 0, 0, 1);
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
    void* obj = 0;
    m_c->m_soundRegistry->m_10.Lookup(
        "BOOTY_LOOP",
        obj
    ); // CSndHost::m_10 (::CMapStringToPtr @0x1b8438)
    LeafCue* found = static_cast<LeafCue*>(obj);
    if (found && (static_cast<DirectSoundMgr*>(found->m_10))->IsPlaying()) {
        (static_cast<DirectSoundMgr*>(found->m_10))->CloneAndPlay(0, 0x1f4, 1);
        while ((static_cast<DirectSoundMgr*>(found->m_10))->IsPlaying()) {
            if (m_c->m_soundRegistry->m_2c != 0) {
                m_c->m_soundRegistry->m_2c->PurgeVoiceList(-1);
            }
        }
    }
    return 1;
}

// CBootyState::BuildWarpStoneGlitterAnimation (0x19540): build 4 "DoNothing" warp-
// letter animations through the glitter factory (g_gameReg viewed as CGlitterMgr:
// m_world->m_childGroup), stash them in the +0x1ec sprite array, set/clear their active bit, then
// build the trailing "SimpleAnimation" glitter sprite.
// RE-HOMED from CMultiBootyState, which is a SIBLING of CBootyState (both derive from
// CState) - so it could never have been called, as it is, on a CBootyState's own `this`.
// Its ONLY caller is 0x18830 = CBootyState's vtable slot 1, via `mov ecx,esi`.
// @early-stop
// 88.1%: logic byte-faithful. Residual is the branchless-select codegen for the per-letter
// `(i != m_letterIdx) ? 1 : 3` kind + the per-iteration g_gameReg reload scheduling.
RVA(0x00019540, 0x12a)
i32 CBootyState::BuildWarpStoneGlitterAnimation() {
    CGruntzMgr* reg = g_gameReg;
    // The +0x1ec array is CBootyState::m_trailSprites - a real, typed member now, so the
    // `(CGameObject**)((char*)this + 0x1ec)` offset-reach is gone.
    CGameObject** slot = m_trailSprites;
    m_radius = 0xc8;
    m_letterIdx = (reg->m_scoreHud->m_count - 1) % 4; // +0x7c->+0x04: the active letter count
    m_angleStep = 0;
    m_scratchX = 0;
    m_1e8 = 0;
    for (i32 i = 0; i < 4; i++) {
        CGameObject* a = g_gameReg->m_world->m_childGroup
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
    CGameObject* g =
        g_gameReg->m_world->m_childGroup->CreateSprite(0, 0, 0, 4, "SimpleAnimation", 3);
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
            i32* tbl = g_bootyLetterCoords + 1; // walks: tbl[-1]=x, tbl[0]=y; advances by 2
            CGameObject** ap = m_trailSprites; // walks the array by 1
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
        m_cursorLetter->m_screenX = g_bootyLetterCoords[m_letterIdx * 2];
        m_cursorLetter->m_screenY = g_bootyLetterCoords[m_letterIdx * 2 + 1];
        return;
    }

    i32 step = m_angleStep;
    i32 idx = m_letterIdx;
    double r = static_cast<float>(m_radius); // load (float)m_radius first; shared across sin/cos terms
    double ang = (static_cast<float>(step) - kGlitterPhaseBias) * kDegToRad;
    m_scratchX = static_cast<i32>((sin(ang) * r + static_cast<float>(g_bootyLetterCoords[idx * 2])));
    m_1e8 = static_cast<i32>((cos(ang) * r + static_cast<float>(g_bootyLetterCoords[idx * 2 + 1])));
    m_angleStep = step + 5;
    m_radius =
        static_cast<i32>((kGlitterStartRadius - static_cast<float>((step + 5)) * kGlitterShrinkRate * kGlitterStartRadius));

    // Snap the leading sprites (0..m_letterIdx-1) to their static table coords (pointer walk).
    i32 i = 0;
    CGameObject** arr1ec = m_trailSprites;
    if (idx > 0) {
        i32* tbl = g_bootyLetterCoords + 1; // ecx: tbl[-1]=x, tbl[0]=y
        CGameObject** ap = arr1ec;          // eax
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
        CGameObject** p = m_sprintSprites;
        i32 n = 8;
        do {
            CGameObject* e = *p;
            p++;
            e->m_stateFlags |= 1;
        } while (--n);
        return;
    }
    CGameObject** p = m_sprintSprites;
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

// ---------------------------------------------------------------------------
// CBootyState::FormatHudText (0x1af70) - homed here from the former MenuState.cpp.
// It is a CBootyState method (not CMenuState), and 0x1af70 sits INSIDE this TU's
// own 0x18c90..0x1f928 .text block, between 0x19b90 and CheckPerfectBonus
// (0x1c0f0) - the same block the comment above attributes it to.
// ---------------------------------------------------------------------------
// FormatHudText's stats source IS the real CBattlezData (g_gameReg->m_scoreHud, the
// +0x7c HUD/score accumulator). The CHudStats view that used to sit here is GONE: its
// 13 GetC10..GetC40 "getters" were placeholder names for CBattlezData's own
// SumGroupField* methods - PHANTOMS (declared-only, no body, no rva) that no obj and no
// .LIB could ever define. Each was resolved from the binary by following the ILT thunk
// FormatHudText actually calls to its target rva, every one of which is an
// already-reconstructed, rva-bound CBattlezData method in the `battlezdata` unit:
//     GetC10 -> SumGroupField08 (0xfd2e0)   GetC38 -> SumGroupField34 (0xfd0b0)
//     GetC1c -> SumGroupField14 (0xfd290)   GetC24 -> SumGroupField1c (0xfd060)
//     GetC20 -> SumGroupField18 (0xfd240)   GetC40 -> SumGroupField3c (0xfd1f0)
//     GetC34 -> SumGroupField30 (0xfd010)   GetC2c -> SumGroupField24 (0xfd1a0)
//     GetC18 -> SumGroupField10 (0xfcfc0)   GetC3c -> SumGroupField38 (0xfd150)
//     GetC30 -> SumGroupField2c (0xfcf70)   GetC28 -> SumGroupField20 (0xfd100)
//     GetC14 -> SumGroupField0c (0xfcf20)
// The view's cached fields were the same object's: its m_c gate is CBattlezData's
// m_allDone (+0x0c) and its m_10 is m_score (+0x10).
// The (CBattlezData*) cast that used to sit here is GONE: CGameRegistry::m_scoreHud is
// TYPED now. The apparent conflict with Wormhole.cpp (which cast the SAME member to a
// CTeleMgrSub*) was never a conflict - CTeleMgrSub was a one-field view of THIS object,
// its m_28 being CBattlezData::m_28 (+0x28), the teleporter counter this very function
// reads back as its case-7 stat. Both casts were pointing at the same class all along.
// One HUD stat read, inlined per site as retail does (the typed g_gameReg->m_scoreHud
// CBattlezData reloaded at each use).
#define STAT(getter, field)                                                                        \
    ((m_initOnce != 0 && g_gameReg->m_scoreHud->m_allDone != 0) ? g_gameReg->m_scoreHud->getter()  \
                                                                : g_gameReg->m_scoreHud->field)

// CBootyState::FormatHudText(buf, sel) (0x1af70): the 960-byte HUD-text formatter - an
// 8-case switch that sprintf()s the game clock (MM:SS via the imul-by-0x10624dd3
// divide-by-1000 then /60), score, and "%d of %d" progress into `buf`. Every stat is
// read via STAT(getter, field). The default case writes "???".
// @early-stop
// jump-table-data scoring artifact (docs/patterns/jumptable-data-overlap.md): the
// 960-byte switch body is CODE-BYTE-EXACT (verified llvm-objdump -dr base vs retail:
// every stat sibling-guard block, the MM:SS unsigned /1000-then-/60 divide magic, the
// "%d of %d" clamp, the 13 stats-thiscall getters, and the sprintf pushes all match;
// the ~24 g_gameReg loads are the retail A1 moffs32 form). Residual ~2.5% is the
// inline .rdata jump table (8 case addresses) + the reloc-typed format-string DIR32
// operands, neither source-steerable. ~97.5%.
RVA(0x0001af70, 0x3c0)
void CBootyState::FormatHudText(CString* buf, i32 sel) {
    switch (sel) {
        case 0: {
            u32 secs = static_cast<u32>((STAT(SumGroupField08, m_score) / 1000));
            buf->Format("%d:%2.2d", secs / 60, secs % 60);
            return;
        }
        case 1:
            buf->Format("%d", STAT(SumGroupField14, m_1c));
            return;
        case 2:
            buf->Format("%d", STAT(SumGroupField18, m_20));
            return;
        case 3: {
            i32 total = STAT(SumGroupField30, m_34);
            i32 cap = STAT(SumGroupField30, m_34);
            i32 cur = STAT(SumGroupField10, m_weaponCount);
            if (cur >= cap) {
                cur = cap;
            }
            buf->Format("%d of %d", cur, total);
            return;
        }
        case 4: {
            i32 total = STAT(SumGroupField2c, m_30);
            i32 cap = STAT(SumGroupField2c, m_30);
            i32 cur = STAT(SumGroupField0c, m_toyzCount);
            if (cur >= cap) {
                cur = cap;
            }
            buf->Format("%d of %d", cur, total);
            return;
        }
        case 5: {
            i32 total = STAT(SumGroupField34, m_38);
            i32 cap = STAT(SumGroupField34, m_38);
            i32 cur = STAT(SumGroupField1c, m_powerupCount);
            if (cur >= cap) {
                cur = cap;
            }
            buf->Format("%d of %d", cur, total);
            return;
        }
        case 6: {
            i32 total = STAT(SumGroupField3c, m_40);
            i32 cap = STAT(SumGroupField3c, m_40);
            i32 cur = STAT(SumGroupField24, m_2c);
            if (cur >= cap) {
                cur = cap;
            }
            buf->Format("%d of %d", cur, total);
            return;
        }
        case 7: {
            i32 total = STAT(SumGroupField38, m_3c);
            i32 cap = STAT(SumGroupField38, m_3c);
            i32 cur = STAT(SumGroupField20, m_28);
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

// CBootyState::CheckPerfectBonus() (0x1c0f0): once the frame-ready gate fires, SCROLL the
// BOOTY_PERFECT sprite across the screen - it is not a "bonus state" with a "phase" at all.
// RE-HOMED from CMultiBootyState, which is allocation-proven 0x244 while this body reads
// [this+0x2f8] off its OWN `this` three times (`mov ebx,ecx` at 0x1c0f9) - 0xb4 bytes past
// that class's end. Its ONLY caller is CBootyState::Render (slot 5, 0x1c210), which invokes
// it with `mov ecx,esi` - its own `this`. CBootyState (0x320) holds +0x2f8. The same
// evidence attributed FormatHudText, LevelMsgHudDriver and the slot-1 build chain; this is the fourth.
//
// And the object AT +0x2f8 is just the sprite: the "bonus state (m_5c phase / m_8 flags)"
// view was CGameObject all along - m_5c is m_screenX, m_8 is m_flags. The proof is the
// sentinel: BuildBootyPerfectAnimation (0x1c070, also this class) creates the sprite at
// x = 0xffffff7e, and that is EXACTLY the -0x82 "wrap value" tested below. The sprite spawns
// off-screen left, this cues BOOTY_PERFECT on the frame it appears, then walks it right by
// 0xa a frame until x >= 0x302, where it latches the done bit. Returns 1.
RVA(0x0001c0f0, 0xd5)
i32 CBootyState::CheckPerfectBonus() {
    if (!g_gameReg->m_scoreHud->InBounds(-1)) { // FrameReady @0xfcd70 == CBattlezData::InBounds
        return 1;
    }
    CGameObject* st = m_bootyPerfectSprite;
    i32 phase = st->m_screenX;
    if (phase == static_cast<i32>(0xffffff7e)) {
        CDDrawSurfaceMgr* host = g_gameReg->m_world;
        i32 item = g_gameReg->m_soundVolume; // +0x11c (configured music item, this facet)
        CSndHost* m28 = host->m_soundRegistry;
        if (m28->m_emitGate == 0) {
            void* found = 0;
            m28->m_10.Lookup("BOOTY_PERFECT", found); // ::CMapStringToPtr::Lookup @0x1b8438
            if (found && g_sndEnabled != 0) {
                LeafCue* p = static_cast<LeafCue*>(found);
                if (g_killCueClock - static_cast<u32>(p->m_14) >= static_cast<u32>(p->m_18)) {
                    p->m_14 = g_killCueClock;
                    p->m_10->ConfigureItem(item, 0, 0, 0);
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
// active/ready virtual (IsActive) - return 1 iff ready, else 0.
RVA(0x0001ce10, 0xc)
i32 CBootyState::Vslot06() {
    return IsActive() != 0;
}

// CMultiBootyState::ReadyAndPaint() (0x1ce30): gate on the active/ready virtual (CState
// slot 3 / +0xc); when ready, run the per-frame Paint and return the normalized result;
// otherwise return the (zero) gate result.
RVA(0x0001ce30, 0x1d)
i32 CMultiBootyState::ReadyAndPaint() {
    if (IsActive() == 0) {
        return 0;
    }
    return CState::Vslot07() != 0; // 0xfac70 (qualified base call, cast-free)
}

// CBootyState::Vslot0e (0x1d3e0, vtable slot 14) and Vslot11 (0x1d400, slot 17): the
// same tail-forward to the shared booty-grunt idle-animation builder (0x1ce60) as
// Vslot0c, but with the slot-14/17 (int,int,int) arg frame. Reloc-masked call.
RVA(0x0001d3e0, 0x8)
i32 CBootyState::Vslot0e(i32, i32, i32) {
    return BuildBootyGruntIdleAnimation(); // 0x1ce60 (BzState:: - UNBOUND; needs BzState.h fold)
}

RVA(0x0001d400, 0x8)
i32 CBootyState::Vslot11(i32, i32, i32) {
    return BuildBootyGruntIdleAnimation(); // 0x1ce60 (BzState:: - UNBOUND; needs BzState.h fold)
}

// CBootyState::Vslot0c (0x1d420, vtable slot 12): tail-forward to the shared booty-grunt
// idle-animation builder (0x1ce60). Homed out-of-line as the real virtual (matcher-5); the
// call is reloc-masked, so the shared-body owner name is code-neutral.
RVA(0x0001d420, 0x8)
i32 CBootyState::Vslot0c(i32, i32) {
    return BuildBootyGruntIdleAnimation(); // 0x1ce60 (BzState:: - UNBOUND; needs BzState.h fold)
}

// CMultiBootyState::LoadGameAssetNamespaces (0x1d440): the multi-booty slot-1
// asset/state loader - engine-label backlog stub. Owner re-attributed (ex
// "CBootyState::StateOnEnter"): retail's ONLY reference to 0x1d440 is
// ??_7CMultiBootyState slot 1 (ILT 0x2900 -> 0x1d440; RTTI slot map), it appears
// in no other vtable and has no direct caller.
// @confidence: med
// @source: decomp-xref
// @stub
RVA(0x0001d440, 0xd7d)
i32 CMultiBootyState::LoadGameAssetNamespaces(i32, i32, i32) {
    return 0;
}

// CMultiBootyState::ReleaseResources() (slot 2 / +0x8, 0x1e520): free the leaf-registry
// pooled resource (if set), release the "BOOTY" set on the leaf registry, run a teardown
// on the owner's m_4->m_60 sub-object (~CMoviePlayer), then chain CState::ReleaseResources.
// @early-stop
// near-exact (~98.5%): structure/offsets/calls all match; the sole non-reloc residual is
// the m_4 deref landing in eax vs retail's edx (single-register coin-flip).
RVA(0x0001e520, 0x3e)
void CMultiBootyState::ReleaseResources() {
    SoundStream* r = m_c->m_soundRegistry->m_2c; // CSndHost::m_2c is already the real SoundStream*
    if (r) {
        r->Stop();
    }
    m_c->m_soundRegistry->RemoveKeysEqual_157c70("BOOTY", "_");
    // m_4 (CState::m_4) IS the CGruntzMgr singleton; the sub-object it tears down here is
    // its +0x60 slot. GruntzMgr.h types that slot TimerObj* (m_timer) while this teardown
    // runs ~CMoviePlayer on it - a real substance divergence on ONE field, flagged (the
    // cast marks it) rather than forked into a second per-TU view of the manager.
    (reinterpret_cast<CMoviePlayer*>(m_4->m_timer))->~CMoviePlayer();
    CState::ReleaseResources(); // 0xfa150 (chain the base slot-2 teardown; direct)
}

// CMultiBootyState::Vslot09() (slot 9 / +0x24, 0x1e570): on entry build the "multi"
// title page (fade + page) then, if the menu is live, push the "BOOTY_LOOP" cue into the
// player on the draw-clock window. Returns 1.
RVA(0x0001e570, 0xb4)
i32 CMultiBootyState::Vslot09(i32) {
    i32 ok = FadeInTitle("multi", 0, 0, 0, 0, 1); // 0xfa1f0 (CState base method)
    if (!ok) {
        return ok; // eax already 0 (the FadeInTitle result) - no xor/mov re-materialize
    }
    m_c->m_drawTarget->Method_158ee0();
    RetireScene(0x50, 0x3e8, 0, 1); // 0xfa8f0 CState::RetireScene (inherited, cast-free)

    CDDrawSurfaceMgr* host = g_gameReg->m_world;
    i32 item = g_gameReg->m_soundVolume; // +0x11c (configured music item, this facet)
    CSndHost* m28 = host->m_soundRegistry;
    if (m28->m_emitGate == 0) {
        void* found = 0;
        m28->m_10.Lookup("BOOTY_LOOP", found); // ::CMapStringToPtr::Lookup @0x1b8438
        if (found && g_sndEnabled != 0) {
            LeafCue* p = static_cast<LeafCue*>(found);
            if (g_killCueClock - static_cast<u32>(p->m_14) >= static_cast<u32>(p->m_18)) {
                p->m_14 = g_killCueClock;
                p->m_10->ConfigureItem(item, 0, 0, 1);
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
    void* obj = 0;
    m_c->m_soundRegistry->m_10.Lookup(
        "BOOTY_LOOP",
        obj
    ); // CSndHost::m_10 (::CMapStringToPtr @0x1b8438)
    LeafCue* found = static_cast<LeafCue*>(obj);
    if (found && (static_cast<DirectSoundMgr*>(found->m_10))->IsPlaying()) {
        (static_cast<DirectSoundMgr*>(found->m_10))->CloneAndPlay(0, 0x1f4, 1);
        while ((static_cast<DirectSoundMgr*>(found->m_10))->IsPlaying()) {
            if (m_c->m_soundRegistry->m_2c != 0) {
                m_c->m_soundRegistry->m_2c->PurgeVoiceList(-1);
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
    char* base = reinterpret_cast<char*>(g_gameReg);
    i32 i = 0;
    char* rec = base + 0x174;
    for (; i < 4; i++) {
        if (*reinterpret_cast<i32*>((rec + 4)) != 0 && *reinterpret_cast<i32*>(rec) == 0) {
            return *reinterpret_cast<i32*>((rec - 0x24));
        }
        rec += 0x238;
    }
    return 0;
}

// ===========================================================================
// CMultiBootyState::DrawBattleStats (0x1ed30; called here as OnActivated). The
// in-game BATTLE-STATZ scoreboard renderer (sibling of DrawDebugStats): 6 numeric
// stat columns per active player, 7 category labels, per-player team-colour name, the
// title. The reused CString + colour-name temp give it the /GX frame. The per-player
// records (+0x150, stride 0x238) are still reached by raw offset off the singleton (the
// established idiom); every callee is a reloc-masked external; CopyRect is hoisted
// through a data fn-ptr global.
// ===========================================================================
// DrawStatText (0x1f00 -> 0x1154b0): __cdecl(ctx, text, rect, y, flag, b, g, r, a9).
extern "C" void
DrawStatText(void* ctx, CString* text, RECT* rc, i32 y, i32 flag, i32 b, i32 g, i32 r, i32 a9);
// GetColorName (0x3e54): NRV CString* into `out`.
CString* GetColorName(CString* out);

// CopyRect USER32 import hoisted through a data fn-ptr global (retail loads it once
// into ebp and calls it ~13x).

// The per-column source-rect tables (RECT[] in .data). Indexed by player/category.
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

// The per-player stat block reached through the singleton's m_scoreHud (+0x7c);
// SumWinRow (0x1230) folds the win-row totals for a player.
//
// The CBattleStatsView view is GONE: its only field was `void* m_c` at +0x0c, which is
// CState::m_c (the CDDrawSurfaceMgr draw context) at the SAME offset, and both call
// sites (CMultiBootyState::Render @0x1f480 and ::InputVirtual @0x1f6f0, in this TU) invoke
// 0x1ed30 with `mov ecx,this` on their own CMultiBootyState `this` - no this-adjustment, no
// other caller anywhere (`sema xref 0x1ed30` finds only its ILT thunk). So DrawBattleStats
// is a CMultiBootyState method, declared on the canonical class in <Gruntz/GameMode.h>;
// the two `((CBattleStatsView*)this)->` cross-casts fall out.
// The former second binding of the singleton here (`CGameRegistry* g_mgr`, a SECOND
// DATA(0x24556c) under a second symbol name) is dropped with it - one address, one name,
// one type: the `extern "C" CGruntzMgr* g_gameReg` above.

static __inline i32 sumRun(CBattlezData* base, i32 off, i32 n) {
    i32* p = reinterpret_cast<i32*>((reinterpret_cast<char*>(base) + off));
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
void CMultiBootyState::DrawBattleStats() {
    CString s;
    RECT rc;
    BOOL(WINAPI * copyRect)(LPRECT, const RECT*) = ::CopyRect;
    i32 i;
    i32 c;

    // Loop 1: 6 numeric stat columns per active player.
    for (i = 0; i < 4; i++) {
        if (*reinterpret_cast<i32*>((reinterpret_cast<char*>(g_gameReg) + 0x178 + i * 0x238)) != 0) {
            s.Format("%d", sumRun(g_gameReg->m_scoreHud, 0x348 + i * 0x10, 4));
            copyRect(&rc, &g_col1Rects[i]);
            DrawStatText(m_c, &s, &rc, 0x78, 1, 0xff, 0xff, 0, 1);

            s.Format("%d", sumRun(g_gameReg->m_scoreHud, 0x2d8 + i * 0x1c, 7));
            copyRect(&rc, &g_col2Rects[i]);
            DrawStatText(m_c, &s, &rc, 0x78, 1, 0xff, 0xff, 0, 1);

            s.Format("%d", sumRun(g_gameReg->m_scoreHud, 0x238 + i * 0x28, 10));
            copyRect(&rc, &g_col3Rects[i]);
            DrawStatText(m_c, &s, &rc, 0x78, 1, 0xff, 0xff, 0, 1);

            s.Format("%d", sumRun(g_gameReg->m_scoreHud, 0xd8 + i * 0x58, 22));
            copyRect(&rc, &g_col4Rects[i]);
            DrawStatText(m_c, &s, &rc, 0x78, 1, 0xff, 0xff, 0, 1);

            s.Format("%d", *reinterpret_cast<i32*>((reinterpret_cast<char*>(g_gameReg->m_scoreHud) + 0x48 + i * 4)));
            copyRect(&rc, &g_col5Rects[i]);
            DrawStatText(m_c, &s, &rc, 0x78, 1, 0xff, 0xff, 0, 1);

            s.Format("%d", (g_gameReg->m_scoreHud)->SumWinRow(i));
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
        if (*reinterpret_cast<i32*>((reinterpret_cast<char*>(g_gameReg) + 0x178 + i * 0x238)) != 0) {
            i32 color;
            switch (*reinterpret_cast<i32*>((reinterpret_cast<char*>(g_gameReg) + 0x158 + i * 0x238))) {
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
            s.Format("%s", static_cast<const char*>(*GetColorName(&cn)));
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

// ---------------------------------------------------------------------------
// CMultiBootyState::Render (slot 5, +0x14, 0x1f480) - the booty-countdown per-frame
// draw. Bail (ReportError 0x8006/0x459) when the frame surface is lost and the input
// poll did not consume the frame. Draw the battle-stats scoreboard once (the m_1b8
// latch flips 0x64 -> 0xc7), run the sprite worker's frame apply/present, format the
// remaining time as a HUD line (H:MM:SS or M:SS), flip the frame + blit the page onto
// the target, and purge finished sound voices. /GX (the CString temp).
RVA(0x0001f480, 0x1e9)
i32 CMultiBootyState::Render() {
    IDirectDrawSurface* frameSurf = m_c->m_drawTarget->m_frontPair->m_surface->m_8;
    if (frameSurf == 0 || frameSurf->IsLost() != 0) {
        if (InputVirtual() == 0) {
            m_4->ReportError(0x8006, 0x459);
            return 0;
        }
    }
    if (m_1b8 == 0x64) {
        DrawBattleStats(); // 0x1ed30 (OnActivated slot; own method, cast-free)
        m_1b8 = 0xc7;
    }
    m_c->m_childGroup->TickKillCues_159a70(1);
    m_c->m_childGroup->WalkDispatch2C(m_c->m_drawTarget->m_backPair);

    // +0x7c->+0x10: the booty countdown's elapsed-millisecond source. The SAME field the
    // battlez scoreboard reads as its score accumulator (CBattlezData::m_score) - one
    // field, two readers; the canonical name is kept rather than forking the class.
    u32 secs = g_gameReg->m_scoreHud->m_score / 1000; // signed /1000, then unsigned H:M:S
    CString s;
    RECT rc;
    SetRect(&rc, 8, 0x41, 0xcb, 0xae);
    if (secs / 3600 != 0) {
        s.Format("%d:%2.2d:%2.2d", secs / 3600, (secs / 60) % 60, secs % 60);
    } else {
        s.Format("%d:%2.2d", secs / 60, secs % 60);
    }
    ShowHudMessageAlt(reinterpret_cast<HudMsgSink*>(m_c), reinterpret_cast<i32>(&s), reinterpret_cast<i32>(&rc), 0x6e, 1, 0xff, 0xff, 0, 1);

    CDDrawSubMgrPages* dt = m_c->m_drawTarget;
    dt->m_frontPair->m_surface->Flip(0);
    dt->m_backPair->m_surface
        ->BltFast(0, 0, dt->m_overlayPair->m_surface, &dt->m_overlayPair->m_srcRect, 0x10);
    if (m_c->m_soundRegistry->m_2c != 0) {
        m_c->m_soundRegistry->m_2c->PurgeVoiceList(-1); // SoundDevice base method (inherited)
    }
    return 1;
}

// ---------------------------------------------------------------------------
// CMultiBootyState::InputVirtual (slot 8, 0x1f6f0) - on state activation: chain the base
// image-load gate, hide the cursor, resolve+register the BOOTY/GRUNTZ/LEVEL "IMAGEZ" sets
// through the CDDrawSurfaceMgr image registry (LoadNamespace +0x4c), fade in the "multi" title, run
// the post-activate hook, then kick the render worker apply + the page timer.
RVA(0x0001f6f0, 0x10b)
i32 CMultiBootyState::InputVirtual() {
    if (!CState::InputVirtual()) { // 0xface0: the shared CState slot-8 base image-load gate
        return 0;
    }

    while (ShowCursor(FALSE) >= 0)
        ;

    void* tree = SymTab2c()->ResolvePath("IMAGEZ");
    if (!tree) {
        return 0;
    }
    CImageRegistry* reg = m_c->m_imageRegistry;
    if (reg->LoadNamespace(tree, "BOOTY", "_") == -1) {
        return 0;
    }

    tree = m_gruntzBank->ResolvePath("IMAGEZ");
    if (!tree) {
        return 0;
    }
    reg = m_c->m_imageRegistry;
    if (reg->LoadNamespace(tree, "GRUNTZ", "_") == -1) {
        return 0;
    }

    tree = m_levelBank->ResolvePath("IMAGEZ");
    if (!tree) {
        return 0;
    }
    reg = m_c->m_imageRegistry;
    if (reg->LoadNamespace(tree, "LEVEL", "_") == -1) {
        return 0;
    }

    if (!FadeInTitle("multi", 0, 0, 0, 0, 1)) { // 0xfa1f0 (CState base method)
        return 0;
    }

    DrawBattleStats(); // 0x1ed30 (OnActivated slot; own method, cast-free)
    m_c->m_drawTarget->Method_158ee0();
    RetireScene(0x50, 0x3e8, 0, 1); // 0xfa8f0 CState::RetireScene (inherited, cast-free)
    return 1;
}

// CMultiBootyState::Vslot06 (slot 6 / +0x18, 0x1f850): boolify the class's slot-3
// active/ready virtual (IsActive) - return 1 iff ready, else 0.
RVA(0x0001f850, 0xc)
i32 CMultiBootyState::Vslot06() {
    return IsActive() != 0;
}

// ---------------------------------------------------------------------------
// CMultiBootyState::Vslot07 (slot 7 / +0x1c, 0x1f870): gate on the slot-3 ready
// virtual (IsActive); when ready run the non-virtual paint (Paint, 0xfac70) and return
// its normalized result, else 0. (Was the @orphan CGuardedDispatch1f870 view; the
// vtable slot-7 attribution -- find_holding(0x1f870) == CMultiBootyState:7 -- recovers
// its real identity.)
// ---------------------------------------------------------------------------
RVA(0x0001f870, 0x1d)
i32 CMultiBootyState::Vslot07() {
    if (IsActive() == 0) {
        return 0;
    }
    return CState::Vslot07() != 0; // 0xfac70 (qualified base call, cast-free)
}

// ---------------------------------------------------------------------------
// CMultiBootyState::PostCommandIfKey (0x1f8a0): if the one-shot battle-stats latch
// (m_1b8) reads 0xc7, post WM_COMMAND 0x8023 to the game window
// (g_gameReg->m_gameWnd->m_hwnd) via ::PostMessageA; always return 1. __thiscall,
// no args. (Was the @identity-TODO PendingCmdKeyHost view; the slot-12/14/17
// forwarders below tail-call it with ecx = a CMultiBootyState `this`, and it reads
// m_1b8 (+0x1b8) -- that xref recovers it as this CMultiBootyState method.)
// ---------------------------------------------------------------------------
RVA(0x0001f8a0, 0x30)
i32 CMultiBootyState::PostCommandIfKey() {
    if (m_1b8 == 0xc7) {
        ::PostMessageA(g_gameReg->m_gameWnd->m_hwnd, 0x111, 0x8023, 0);
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
// slot-2 release (statically bound), re-stamps CState, chains CState::ReleaseResources (compiler-
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
