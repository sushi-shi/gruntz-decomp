#include <Gruntz/BootyMessages.h> // DrawStatText / g_bootyLetterCoords (ex .cpp externs)
#include <Dsndmgr/DirectSoundMgr.h>
#include <Rez/FrameClock.h> // frame-clock band (g_frameDelta/g_frameTime/g_killCueClock/g_engineFrameDelta)
#include <Gruntz/GameRegMfcPtr.h>
#include <Gruntz/SoundState.h> // g_sndEnabled/g_sndCueTag
#include <Bute/SymTab.h>
#include <Bute/SymParser.h>
#include <DDrawMgr/DDrawSubMgrPages.h>
#include <DDrawMgr/DDrawSubMgrLeafScan.h> // RemoveKeysEqual (Booty/MultiBooty ReleaseResources)
#include <DDrawMgr/DDrawWorkerRegistry.h> // RemoveKeysEqual (CBootyState::ReleaseResources)
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

void operator delete(void*);

DATA_SYMBOL(0x001e8fe8, 0x0, _g_bootyLetterCoords)

static const float kGlitterPhaseBias = -225.0f;  // was g_5e93b4 (fsub'd, hence negative)
static const double kDegToRad = 0.017453292;     // was g_5e93b8 (pi/180)
static const double kGlitterShrinkRate = 0.002;  // was g_5e93c0
static const double kGlitterStartRadius = 350.0; // was g_5e93c8

RVA(0x00018c90, 0x72)
void CBootyState::ReleaseResources() {
    SoundStream* r =
        m_world->m_soundRegistry->m_2c; // CSndHost::m_2c is already the real SoundStream*
    if (r) {
        r->Stop();
    }
    m_world->m_soundRegistry->RemoveKeysEqual("BOOTY", "_");
    m_world->m_soundRegistry->RemoveKeysEqual("GRUNTZ_WANDGRUNT", "_");
    m_world->m_imageRegistry->RemoveKeysEqual("BOOTY", "_");
    m_world->m_imageRegistry->RemoveKeysEqual("GRUNTZ_GOKARTGRUNT", "_");
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
    m_world->m_drawTarget->TransExit();
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
    m_world->m_soundRegistry->m_10.Lookup(
        "BOOTY_LOOP",
        obj
    ); // CSndHost::m_10 (::CMapStringToPtr @0x1b8438)
    LeafCue* found = static_cast<LeafCue*>(obj);
    if (found && (static_cast<DirectSoundMgr*>(found->m_10))->IsPlaying()) {
        (static_cast<DirectSoundMgr*>(found->m_10))->CloneAndPlay(0, 0x1f4, 1);
        while ((static_cast<DirectSoundMgr*>(found->m_10))->IsPlaying()) {
            if (m_world->m_soundRegistry->m_2c != 0) {
                m_world->m_soundRegistry->m_2c->PurgeVoiceList(-1);
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
    CWwdGameObjectA** slot = m_trailSprites;
    m_radius = 0xc8;
    m_letterIdx = (reg->m_scoreHud->m_count - 1) % 4; // +0x7c->+0x04: the active letter count
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
            i32* tbl = g_bootyLetterCoords + 1;    // walks: tbl[-1]=x, tbl[0]=y; advances by 2
            CWwdGameObjectA** ap = m_trailSprites; // walks the array by 1
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
        return;
    }

    i32 step = m_angleStep;
    i32 idx = m_letterIdx;
    double r =
        static_cast<float>(m_radius); // load (float)m_radius first; shared across sin/cos terms
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

    // Snap the leading sprites (0..m_letterIdx-1) to their static table coords (pointer walk).
    i32 i = 0;
    CWwdGameObjectA** arr1ec = m_trailSprites;
    if (idx > 0) {
        i32* tbl = g_bootyLetterCoords + 1; // ecx: tbl[-1]=x, tbl[0]=y
        CWwdGameObjectA** ap = arr1ec;      // eax
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
    // The trailing sprite + the i'th (== m_letterIdx) sprite get the computed scratch coords.
    m_cursorLetter->m_screenX = m_scratchX;
    m_cursorLetter->m_screenY = m_scratchY;
    arr1ec[i]->m_screenX = m_scratchX;
    arr1ec[i]->m_screenY = m_scratchY;

    MoveLettersByDir();

    if (m_radius == 0) {
        CWwdGameObjectA* e = arr1ec[i];
        if (e->m_sortKey != 1) {
            e->m_sortKey = 1;
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

RVA(0x0001c0f0, 0xd5)
i32 CBootyState::CheckPerfectBonus() {
    if (!g_gameReg->m_scoreHud->InBounds(-1)) { // FrameReady @0xfcd70 == CBattlezData::InBounds
        return 1;
    }
    CWwdGameObjectA* st = m_bootyPerfectSprite;
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

RVA(0x0001ce10, 0xc)
i32 CBootyState::Vslot06() {
    return IsActive() != 0;
}

RVA(0x0001ce30, 0x1d)
i32 CBootyState::Vslot07() {
    if (IsActive() == 0) {
        return 0;
    }
    return CState::Vslot07() != 0; // 0xfac70 (qualified base call, cast-free)
}

RVA(0x0001d3e0, 0x8)
i32 CBootyState::Vslot0e(i32, i32, i32) {
    return BuildBootyGruntIdleAnimation(); // 0x1ce60 (BzState:: - UNBOUND; needs BzState.h fold)
}

RVA(0x0001d400, 0x8)
i32 CBootyState::Vslot11(i32, i32, i32) {
    return BuildBootyGruntIdleAnimation(); // 0x1ce60 (BzState:: - UNBOUND; needs BzState.h fold)
}

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
    SoundStream* r =
        m_world->m_soundRegistry->m_2c; // CSndHost::m_2c is already the real SoundStream*
    if (r) {
        r->Stop();
    }
    m_world->m_soundRegistry->RemoveKeysEqual("BOOTY", "_");
    // m_4 (CState::m_4) IS the CGruntzMgr singleton; the sub-object it tears down here is
    // its +0x60 slot. GruntzMgr.h types that slot TimerObj* (m_timer) while this teardown
    // runs ~CMoviePlayer on it - a real substance divergence on ONE field, flagged (the
    // cast marks it) rather than forked into a second per-TU view of the manager.
    (reinterpret_cast<CMoviePlayer*>(m_mgr->m_cueSink))->~CMoviePlayer();
    CState::ReleaseResources(); // 0xfa150 (chain the base slot-2 teardown; direct)
}

RVA(0x0001e570, 0xb4)
i32 CMultiBootyState::Vslot09(i32) {
    i32 ok = FadeInTitle("multi", 0, 0, 0, 0, 1); // 0xfa1f0 (CState base method)
    if (!ok) {
        return ok; // eax already 0 (the FadeInTitle result) - no xor/mov re-materialize
    }
    m_world->m_drawTarget->TransExit();
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
    m_world->m_soundRegistry->m_10.Lookup(
        "BOOTY_LOOP",
        obj
    ); // CSndHost::m_10 (::CMapStringToPtr @0x1b8438)
    LeafCue* found = static_cast<LeafCue*>(obj);
    if (found && (static_cast<DirectSoundMgr*>(found->m_10))->IsPlaying()) {
        (static_cast<DirectSoundMgr*>(found->m_10))->CloneAndPlay(0, 0x1f4, 1);
        while ((static_cast<DirectSoundMgr*>(found->m_10))->IsPlaying()) {
            if (m_world->m_soundRegistry->m_2c != 0) {
                m_world->m_soundRegistry->m_2c->PurgeVoiceList(-1);
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

CString* GetColorName(CString* out);

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
        if (g_gameReg->m_options[i].m_joined != 0) {
            s.Format("%d", sumRun(g_gameReg->m_scoreHud, 0x348 + i * 0x10, 4));
            copyRect(&rc, &g_col1Rects[i]);
            DrawStatText(m_world, &s, &rc, 0x78, 1, 0xff, 0xff, 0, 1);

            s.Format("%d", sumRun(g_gameReg->m_scoreHud, 0x2d8 + i * 0x1c, 7));
            copyRect(&rc, &g_col2Rects[i]);
            DrawStatText(m_world, &s, &rc, 0x78, 1, 0xff, 0xff, 0, 1);

            s.Format("%d", sumRun(g_gameReg->m_scoreHud, 0x238 + i * 0x28, 10));
            copyRect(&rc, &g_col3Rects[i]);
            DrawStatText(m_world, &s, &rc, 0x78, 1, 0xff, 0xff, 0, 1);

            s.Format("%d", sumRun(g_gameReg->m_scoreHud, 0xd8 + i * 0x58, 22));
            copyRect(&rc, &g_col4Rects[i]);
            DrawStatText(m_world, &s, &rc, 0x78, 1, 0xff, 0xff, 0, 1);

            s.Format("%d", g_gameReg->m_scoreHud->m_counts[i]);
            copyRect(&rc, &g_col5Rects[i]);
            DrawStatText(m_world, &s, &rc, 0x78, 1, 0xff, 0xff, 0, 1);

            s.Format("%d", (g_gameReg->m_scoreHud)->SumWinRow(i));
            copyRect(&rc, &g_col6Rects[i]);
            DrawStatText(m_world, &s, &rc, 0x78, 1, 0xff, 0xff, 0, 1);
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
        DrawStatText(m_world, &s, &rc, 0x78, 1, 0xff, 0xff, 0, 1);
    }

    // Colour loop: team-colour name per active player, drawn in that colour.
    for (i = 0; i < 4; i++) {
        if (g_gameReg->m_options[i].m_joined != 0) {
            i32 color;
            switch (g_gameReg->m_options[i].m_008) {
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

    // Title.
    s.Format("BATTLE STATZ");
    rc.left = 0x96;
    rc.top = 0xf;
    rc.right = 0x280;
    rc.bottom = 0x73;
    DrawStatText(m_world, &s, &rc, 0x82, 1, 0xff, 0xff, 0, 1);
}

RVA(0x0001f480, 0x1e9)
i32 CMultiBootyState::Render() {
    IDirectDrawSurface* frameSurf = m_world->m_drawTarget->m_frontPair->m_surface->m_ddSurface;
    if (frameSurf == 0 || frameSurf->IsLost() != 0) {
        if (InputVirtual() == 0) {
            m_mgr->ReportError(0x8006, 0x459);
            return 0;
        }
    }
    if (m_1b8 == 0x64) {
        DrawBattleStats(); // 0x1ed30 (OnActivated slot; own method, cast-free)
        m_1b8 = 0xc7;
    }
    m_world->m_childGroup->TickKillCues(1);
    m_world->m_childGroup->WalkDispatch2C(m_world->m_drawTarget->m_backPair);

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
    ShowHudMessageAlt(
        reinterpret_cast<HudMsgSink*>(m_world),
        reinterpret_cast<i32>(&s),
        reinterpret_cast<i32>(&rc),
        0x6e,
        1,
        0xff,
        0xff,
        0,
        1
    );

    CDDrawSubMgrPages* dt = m_world->m_drawTarget;
    dt->m_frontPair->m_surface->Flip(0);
    dt->m_backPair->m_surface
        ->BltFast(0, 0, dt->m_overlayPair->m_surface, &dt->m_overlayPair->m_srcRect, 0x10);
    if (m_world->m_soundRegistry->m_2c != 0) {
        m_world->m_soundRegistry->m_2c->PurgeVoiceList(-1); // SoundDevice base method (inherited)
    }
    return 1;
}

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
    CImageRegistry* reg = m_world->m_imageRegistry;
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

    if (!FadeInTitle("multi", 0, 0, 0, 0, 1)) { // 0xfa1f0 (CState base method)
        return 0;
    }

    DrawBattleStats(); // 0x1ed30 (OnActivated slot; own method, cast-free)
    m_world->m_drawTarget->TransExit();
    RetireScene(0x50, 0x3e8, 0, 1); // 0xfa8f0 CState::RetireScene (inherited, cast-free)
    return 1;
}

RVA(0x0001f850, 0xc)
i32 CMultiBootyState::Vslot06() {
    return IsActive() != 0;
}

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

RVA(0x0008d440, 0x55)
CBootyState::~CBootyState() {
    ReleaseResources();
}

RVA(0x0008d510, 0x55)
CMultiBootyState::~CMultiBootyState() {
    ReleaseResources();
}
