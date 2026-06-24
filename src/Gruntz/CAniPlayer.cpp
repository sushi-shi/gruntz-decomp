// CAniPlayer.cpp - a timed cel-animation playback object (placeholder class;
// the RTTI name was not pinned). Two non-virtual __thiscall methods, ascending
// RVA:
//   0x0e7980  Init  (seed the player from a sequence + rect + cel key; 14 args,
//                    the caller passes the sequence as BOTH arg0 and arg1)
//   0x0e7b00  Tick  (timeGetTime-driven cel advance within [+0x4c,+0x50])
//
// The cel table is found by a CMap Lookup on (seq->m_10 + 0x10) keyed by the
// cel key; the engine map/render callees + g_gameReg + timeGetTime are
// reloc-masked. Field names are placeholders (only offsets/code bytes matter).
#include <Gruntz/CAniPlayer.h>

#include <rva.h>

#include <Win32.h> // timeGetTime

// The CMap embedded at (seq->m_10 + 0x10); Lookup(key, &out) -> bool, writing
// the AniCelTable* into out. __thiscall on the map; reloc-masked.
class AniCelMap {
public:
    i32 Lookup(i32 key, AniCelTable** out); // 0x1b8008
};

// One cel renders itself onto a surface context (g_gameReg->m_30->m_4->m_14) at
// (x,y). __thiscall on the cel; this is the shared RenderFrame @0x153790
// (external, reloc-masked - same callee as SBI_MenuItem/SBI_SideTab/etc.).
class AniCelDraw {
public:
    void RenderFrame(i32 surfaceCtx, i32 x, i32 y, i32 z); // 0x153790
};

// The active render target reached as g_gameReg->m_30->m_4->m_14.
struct AniGameMgr {
    char _pad00[4];
    void* m_4; // +0x04  -> render holder (+0x14 is the target)
};
struct AniRenderHolder {
    char _pad00[0x14];
    void* m_14; // +0x14  render target
};

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
        m_2c = seq;
        m_10 = a3;
        m_24 = seq2;
        m_28 = 0;
        m_04 = 1;
        m_rect[0] = r0;
        m_rect[1] = r1;
        m_rect[2] = r2;
        m_rect[3] = r3;
        m_0c = a2;
        if (key != 0) {
            AniCelTable* tbl = 0;
            ((AniCelMap*)((char*)seq2->m_10 + 0x10))->Lookup(key, &tbl);
            m_34 = tbl;
            if (tbl != 0) {
                m_3c = b2;
                m_44 = b3;
                m_48 = b4;
                m_50 = (b0 == -1) ? (b4 >= 0 ? tbl->m_64 : tbl->m_68) : b0;
                m_4c = (b1 == -1) ? (b4 >= 0 ? tbl->m_68 : tbl->m_64) : b1;
                m_38 = m_50;
                if (m_50 < tbl->m_64 || m_50 > tbl->m_68) {
                    m_30 = 0;
                    return 0;
                }
                m_30 = (AniCel*)tbl->m_14[m_50];
                return m_30 != 0;
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
    if (m_28 > 0) {
        AniCelTable* tbl = m_34;
        AniCel* cel;
        if (m_38 >= tbl->m_64 && m_38 <= tbl->m_68) {
            cel = (AniCel*)tbl->m_14[m_38];
        } else {
            cel = 0;
        }
        m_30 = cel;
        if (cel != 0) {
            AniGameMgr* mgr = (AniGameMgr*)g_gameReg->m_30;
            i32 surfaceCtx = (i32)((AniRenderHolder*)mgr->m_4)->m_14;
            ((AniCelDraw*)cel)
                ->RenderFrame(surfaceCtx, cel->m_18 + m_rect[0], cel->m_1c + m_rect[1], 0);
        }
        u32 now = timeGetTime();
        if (now - (u32)m_40 > (u32)m_3c) {
            m_38 += m_48;
            m_40 = timeGetTime();
        }
        if (m_48 > 0) {
            if (m_38 > m_4c) {
                if (m_44 != 0) {
                    m_38 = m_50;
                    return 1;
                }
                m_38 = m_4c;
                m_28--;
            }
        } else if (m_48 < 0) {
            if (m_38 < m_4c) {
                if (m_44 != 0) {
                    m_38 = m_50;
                    return 1;
                }
                m_38 = m_4c;
                m_28--;
            }
        } else {
            m_28--;
        }
    }
    return 1;
}
