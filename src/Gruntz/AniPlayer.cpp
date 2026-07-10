// AniPlayer.cpp - a timed cel-animation playback object (placeholder class;
// the RTTI name was not pinned). Two non-virtual __thiscall methods, ascending
// RVA:
//   0x0e7980  Init  (seed the player from a sequence + rect + cel key; 14 args,
//                    the caller passes the sequence as BOTH arg0 and arg1)
//   0x0e7b00  Tick  (timeGetTime-driven cel advance within [+0x4c,+0x50])
//
// The cel table is found by a CMap Lookup on (seq->m_10 + 0x10) keyed by the
// cel key; the engine map/render callees + g_gameReg + timeGetTime are
// reloc-masked. Field names are placeholders (only offsets/code bytes matter).
#include <Gruntz/AniPlayer.h>
#include <Image/CImage.h>
#include <Gruntz/SerialArchive.h> // CSerialArchive (Read @+0x2c / Write @+0x30)

#include <rva.h>

#include <Win32.h> // timeGetTime

// The cel-map holder's +0x10 map is the real MFC CMapStringToPtr (Lookup @0x1b8008,
// brought in via <Mfc.h>); no local view.

// The CMap embedded at (seq->m_10 + 0x10); Lookup(key, &out) -> bool, writing
// the AniCelTable* into out. __thiscall on the map; reloc-masked.
class AniCelMap {}; // MFC CMapStringToPtr (Lookup @0x1b8008); cast at the call

// The cel renders itself onto the active surface context via AniCel::RenderFrame
// (0x153790, the shared frame-worker blit reached by SBI_MenuItem/SBI_SideTab/etc.);
// modeled directly on AniCel (CAniPlayer.h) so no reinterpret cast is needed.

// The active render context is reached as g_gameReg->m_world->m_drawTarget->m_14:
// g_gameReg->m_world is the canonical CResMgr (ResMgr.h), whose m_drawTarget (+0x04) is the
// active draw surface and whose +0x14 (m_drawContext) is the surface context handed to
// RenderFrame - the same chain SBI_SideTab / SBI_MenuItem walk. Previously modeled as a
// per-TU AniGameMgr/AniRenderHolder view; now consolidated onto CResMgr (no cast).

// The running game clock the timed-play start is stamped from (DAT_00645588).
DATA(0x00245588)
extern "C" u32 g_645588;

// ===========================================================================
// CAniPlayer::Start (0x0e5ad0) - seed the player (forward all 14 args to Init);
// on success record the timed-play window (start clock @+0x58 = g_645588, duration
// @+0x60 = the play interval m_interval), then return 1. Returns 0 if Init fails.
// @early-stop
// ~77%: logic byte-correct (the 14-arg forward, the timer stamp). Residue is the
// arg-marshaling idiom: retail groups the four rect args (r0..r3) into a 16-byte
// stack block (`sub esp,0x10; mov [eax+N]`), which strongly implies Init/Start's
// real signature takes a by-VALUE 4-int rect struct there rather than four scalars -
// but Init (0xe7980) is modeled + banked with four scalar r-args, so changing it is a
// cross-function re-model deferred to the final sweep. Plus a swapped ecx/edx in the
// timer-stamp tail (regalloc). Not steerable without the Init re-model.
RVA(0x000e5ad0, 0x84)
i32 CAniPlayer::Start(
    AniSeq* seq,
    AniSeq* seq2,
    i32 a2,
    i32 a3,
    i32 r0,
    i32 r1,
    i32 r2,
    i32 r3,
    i32 key,
    i32 b0,
    i32 b1,
    i32 b2,
    i32 b3,
    i32 b4
) {
    if (Init(seq, seq2, a2, a3, r0, r1, r2, r3, key, b0, b1, b2, b3, b4) == 0) {
        return 0;
    }
    m_durationLo = m_interval;
    m_durationHi = 0;
    m_startTimeLo = g_645588;
    m_startTimeHi = 0;
    return 1;
}

// ===========================================================================
// CAniPlayer::TickToggle (0x0e5b90) - a timed frame flip: when the timed-play window
// (start clock @+0x58, i64) has elapsed against the running clock g_645588, flip the
// frame between the two range endpoints, restamp the window (duration = m_interval,
// start = now). Returns 1. The command param is ignored.
// @early-stop
// 92% - logic + control flow byte-exact; residual is the i64 window compare's
// register/store scheduling (m_frame/m_duration/m_startTime restamp) - the codegen
// entropy tail shared by the CAniPlayer timed-play family. Final sweep.
// ===========================================================================
RVA(0x000e5b90, 0x51)
i32 CAniPlayer::TickToggle_0e5b90(i32 param) {
    if ((__int64)g_645588 - *(__int64*)&m_startTimeLo >= *(__int64*)&m_durationLo) {
        m_frame = (m_frame == m_startFrame) ? m_endFrame : m_startFrame;
        m_durationLo = m_interval;
        m_durationHi = 0;
        m_startTimeLo = g_645588;
        m_startTimeHi = 0;
    }
    return 1;
}

// ===========================================================================
// CAniPlayer::RenderCel (0x0e5c10) - the render half of Tick: resolve the current cel
// from the cel table by frame (0 when out of range), record it, and - when present -
// blit it onto the active surface context at the rect base + cel offset. Returns 1.
// @early-stop
// 98.1% - logic + externs byte-exact; residual is the m_celTable/m_frame register
// pairing in the range test (same regalloc entropy tail as CAniPlayer::Tick). Final sweep.
// ===========================================================================
RVA(0x000e5c10, 0x54)
i32 CAniPlayer::RenderCel_0e5c10() {
    AniCelTable* tbl = m_celTable;
    AniCel* cel;
    if (m_frame >= tbl->m_firstFrame && m_frame <= tbl->m_lastFrame) {
        cel = (AniCel*)tbl->m_cels[m_frame];
    } else {
        cel = 0;
    }
    m_cel = cel;
    if (cel != 0) {
        i32 surfaceCtx = (i32)g_gameReg->m_world->m_drawTarget->m_14;
        ((CImage*)cel)
            ->RenderFrame(
                (void*)surfaceCtx,
                (void*)(cel->m_offsetX + m_rect[0]),
                (void*)(cel->m_offsetY + m_rect[1]),
                (void*)0
            );
    }
    return 1;
}

// ===========================================================================
// CAniPlayer::TickRenderCurrent (0x0e6dd0) - one play step that renders the CURRENT cel
// (no table re-lookup): while cycles remain, consume one and blit m_cel at the rect
// base + cel offset. Returns 1.
// @early-stop
// 74% - all operations byte-exact (verified via --diff); residual is pure register
// scheduling: retail keeps `this` in eax to load both m_rect halves up front and defers
// the g_gameReg->m_world->m_drawTarget surface-context chain to last, where cl loads the
// global mid-sequence. Neither local nor inlined surfaceCtx flips it (RenderFrame
// arg-eval-order/regalloc wall shared by the whole CAniPlayer render family). Final sweep.
// ===========================================================================
RVA(0x000e6dd0, 0x45)
i32 CAniPlayer::TickRenderCurrent_0e6dd0() {
    if (m_repeatCount > 0) {
        m_repeatCount--;
        AniCel* cel = m_cel;
        if (cel != 0) {
            ((CImage*)cel)
                ->RenderFrame(
                    (void*)(i32)g_gameReg->m_world->m_drawTarget->m_14,
                    (void*)(m_rect[0] + cel->m_offsetX),
                    (void*)(m_rect[1] + cel->m_offsetY),
                    (void*)0
                );
        }
    }
    return 1;
}

// ===========================================================================
// CAniPlayer::TickRenderFrame (0x0e7440) - one play step that re-resolves the cel from
// the table by frame and renders it: while cycles remain, consume one, look up the cel
// (0 when out of range), record it, and - when present - blit it. Returns 1.
// @early-stop
// 86.7% - logic + externs byte-exact; residual is the same RenderFrame surface-context
// chain regalloc as TickRenderCurrent_0e6dd0 plus the cel-table range-test register
// pairing. Final sweep.
// ===========================================================================
RVA(0x000e7440, 0x5e)
i32 CAniPlayer::TickRenderFrame_0e7440() {
    if (m_repeatCount > 0) {
        m_repeatCount--;
        AniCelTable* tbl = m_celTable;
        AniCel* cel;
        if (m_frame >= tbl->m_firstFrame && m_frame <= tbl->m_lastFrame) {
            cel = (AniCel*)tbl->m_cels[m_frame];
        } else {
            cel = 0;
        }
        m_cel = cel;
        if (cel != 0) {
            ((CImage*)cel)
                ->RenderFrame(
                    (void*)(i32)g_gameReg->m_world->m_drawTarget->m_14,
                    (void*)(cel->m_offsetX + m_rect[0]),
                    (void*)(cel->m_offsetY + m_rect[1]),
                    (void*)0
                );
        }
    }
    return 1;
}

// ===========================================================================
// CAniPlayer::Serialize (0x0e5c90) - bail on a null archive; chain the folded base-
// state serialize (0xe7cd0); then round-trip the timed-play window (start clock +
// duration, +0x58/+0x60) through the archive (mode 4 = Write @+0x30, mode 7 = Read
// @+0x2c). Returns 1.
// @early-stop
// ~88%: logic byte-correct. Residue is the base-serialize call reloc (the folded
// 0xe7cd0 body the delinker names CSBI_WarlordHead::Serialize, so my SerializeBase
// name is fuzzy against it) + minor field-address regalloc - the reloc-scoring
// artifact + regalloc family shared by the serialize cluster.
// ===========================================================================
RVA(0x000e5c90, 0x87)
i32 CAniPlayer::Serialize(CSerialArchive* arc, i32 mode, i32 a3, i32 a4) {
    if (arc == 0) {
        return 0;
    }
    if (SerializeBase(arc, mode, a3, a4) == 0) {
        return 0;
    }
    if (mode == 4) {
        arc->Write(&m_startTimeLo, 8);
        arc->Write(&m_durationLo, 8);
    } else if (mode == 7) {
        arc->Read(&m_startTimeLo, 8);
        arc->Read(&m_durationLo, 8);
    }
    return 1;
}

// ===========================================================================
// CAniPlayer::Init  (0x0e7980)
// ===========================================================================
// @early-stop
// regalloc/scheduling wall (~65%): logic complete + verified. The 14-arg /O2
// body diverges only in the optimizer's internal choices - a `push edi` shrink-
// wrap retail elides (it keeps everything in eax/ecx/edx), the rect-store common
// base (retail `lea edx,[esi+0x14]` then [edx+N] vs our direct [esi+N]), and the
// field-store interleave order. None are source-steerable (see
// docs/patterns/shrink-wrapped-callee-save-push.md, zero-register-pinning.md).
RVA(0x000e7980, 0x109)
i32 CAniPlayer::Init(
    AniSeq* seq,
    AniSeq* seq2,
    i32 a2,
    i32 a3,
    i32 r0,
    i32 r1,
    i32 r2,
    i32 r3,
    i32 key,
    i32 b0,
    i32 b1,
    i32 b2,
    i32 b3,
    i32 b4
) {
    if (seq2 != 0 && seq != 0) {
        m_seq = seq;
        m_10 = a3;
        m_seq2 = seq2;
        m_repeatCount = 0;
        m_active = 1;
        m_rect[0] = r0;
        m_rect[1] = r1;
        m_rect[2] = r2;
        m_rect[3] = r3;
        m_0c = a2;
        if (key != 0) {
            AniCelTable* tbl = 0;
            ((CMapStringToPtr*)((char*)seq2->m_celMapHolder + 0x10))
                ->Lookup((const char*)key, (void*&)tbl);
            m_celTable = tbl;
            if (tbl != 0) {
                m_interval = b2;
                m_loop = b3;
                m_step = b4;
                m_startFrame = (b0 == -1) ? (b4 >= 0 ? tbl->m_firstFrame : tbl->m_lastFrame) : b0;
                m_endFrame = (b1 == -1) ? (b4 >= 0 ? tbl->m_lastFrame : tbl->m_firstFrame) : b1;
                m_frame = m_startFrame;
                if (m_startFrame < tbl->m_firstFrame || m_startFrame > tbl->m_lastFrame) {
                    m_cel = 0;
                    return 0;
                }
                m_cel = (AniCel*)tbl->m_cels[m_startFrame];
                return m_cel != 0;
            }
        }
    }
    return 0;
}

// ===========================================================================
// CAniPlayer::Tick  (0x0e7b00)
// ===========================================================================
// @early-stop
// regalloc/tail-merge wall (~91%): logic complete + verified. Residual is the
// cel-fetch eax<->ecx register naming (retail pins tbl in ecx, frame in eax; we
// get the reverse) plus the wrap-tail merge (retail keeps the m_44==0 block's
// register decrement `ecx=m_28;dec;m_28=ecx` separate from the m_48==0 in-memory
// `dec [esi+0x28]`; our recompile tail-merges them). Both are the /O2 allocator/
// block-merger's choices, not source-steerable.
RVA(0x000e7b00, 0xe1)
i32 CAniPlayer::Tick() {
    if (m_repeatCount > 0) {
        AniCelTable* tbl = m_celTable;
        AniCel* cel;
        if (m_frame >= tbl->m_firstFrame && m_frame <= tbl->m_lastFrame) {
            cel = (AniCel*)tbl->m_cels[m_frame];
        } else {
            cel = 0;
        }
        m_cel = cel;
        if (cel != 0) {
            i32 surfaceCtx = (i32)g_gameReg->m_world->m_drawTarget->m_14;
            ((CImage*)cel)
                ->RenderFrame(
                    (void*)surfaceCtx,
                    (void*)(cel->m_offsetX + m_rect[0]),
                    (void*)(cel->m_offsetY + m_rect[1]),
                    (void*)0
                );
        }
        u32 now = timeGetTime();
        if (now - (u32)m_lastTime > (u32)m_interval) {
            m_frame += m_step;
            m_lastTime = timeGetTime();
        }
        if (m_step > 0) {
            if (m_frame > m_endFrame) {
                if (m_loop != 0) {
                    m_frame = m_startFrame;
                    return 1;
                }
                m_frame = m_endFrame;
                m_repeatCount--;
            }
        } else if (m_step < 0) {
            if (m_frame < m_endFrame) {
                if (m_loop != 0) {
                    m_frame = m_startFrame;
                    return 1;
                }
                m_frame = m_endFrame;
                m_repeatCount--;
            }
        } else {
            m_repeatCount--;
        }
    }
    return 1;
}

// ===========================================================================
// CAniPlayer::SetRange (0x0e7c30) - re-arm the player with a new frame window without
// re-resolving the sequence: set start/end frames (a -1 means "derive from the cel
// table's first/last, ordered by the step sign"), the step, loop flag and interval
// (interval -1 = keep), reset the frame to the start, re-arm 2 play cycles and stamp
// the last-tick clock (via the cached g_pTimeGetTime, not the direct import).
// @early-stop
// 88.2% - logic byte-exact; residual is the -1/celTable branch shape: retail hoists the
// m_celTable load per else-arm and stores in each branch, cl computes the value into a
// register and does one store at the merge (explicit if/else made it worse). Final sweep.
// ===========================================================================
extern "C" u32(WINAPI* g_pTimeGetTime)(); // 0x6c4650  cached timeGetTime fn ptr
RVA(0x000e7c30, 0x7d)
void CAniPlayer::SetRange_0e7c30(i32 start, i32 end, i32 step, i32 loop, i32 interval) {
    if (start != -1) {
        m_startFrame = start;
    } else {
        m_startFrame = (step >= 0) ? m_celTable->m_firstFrame : m_celTable->m_lastFrame;
    }
    if (end != -1) {
        m_endFrame = end;
    } else {
        m_endFrame = (step >= 0) ? m_celTable->m_lastFrame : m_celTable->m_firstFrame;
    }
    if (interval != -1) {
        m_interval = interval;
    }
    m_step = step;
    m_loop = loop;
    m_frame = m_startFrame;
    m_repeatCount = 2;
    m_lastTime = g_pTimeGetTime();
}

SIZE_UNKNOWN(AniCel);
SIZE_UNKNOWN(AniCelMap);
SIZE_UNKNOWN(AniCelTable);
SIZE_UNKNOWN(AniSeq);
SIZE_UNKNOWN(CAniPlayer);

// -------------------------------------------------------------------------
// 0x0e6020 (spatially re-homed from src/Stub/ApiCallers.cpp). Placeholder owner
// class unrecovered; kept as the 86% artifact stub.
// @early-stop
// stub artifact wins: the tiny stub's push/pop-4 + `ret 0x28` epilogue
// coincidentally aligns with the target's fail-tail, so objdiff (base-length
// normalized) scores it ~86%. A faithful full-body reconstruction (GameView
// ::Init, __thiscall(a0..a9): geometry stash + mgr-alloc + 3 bounded map
// lookups + SetRect) reaches only ~42%: target keeps 4 callee-saved regs and
// reuses the dead incoming-arg slots as SetRect/lookup scratch, while cl
// spills a fresh `sub esp,0x10` RECT frame + drops ebp - a uniform frame
// shift that mismatches every [esp+X] operand. Frame/regalloc wall.
struct StubOwner_e6020 {
    i32 winapi_0e6020_SetRect(i32, i32, i32, i32, i32, i32, i32, i32, i32, i32);
};
SIZE_UNKNOWN(StubOwner_e6020);
RVA(0x000e6020, 0x288)
i32 StubOwner_e6020::winapi_0e6020_SetRect(i32, i32, i32, i32, i32, i32, i32, i32, i32, i32) {
    return 0;
}
