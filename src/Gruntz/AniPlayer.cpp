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

#include <rva.h>

#include <Win32.h> // timeGetTime

// The cel-map holder's +0x10 map is an MFC CMapStringToPtr (Lookup @0x1b8008); minimal local
// decl (Win32 TU, no <Mfc.h>); const to match the MFC QBE mangling; links from MFC.
SIZE_UNKNOWN(CMapStringToPtr);
class CMapStringToPtr {
public:
    i32 Lookup(const char* key, void*& rValue) const; // 0x1b8008
};

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

SIZE_UNKNOWN(AniCel);
SIZE_UNKNOWN(AniCelMap);
SIZE_UNKNOWN(AniCelTable);
SIZE_UNKNOWN(AniSeq);
SIZE_UNKNOWN(CAniPlayer);
