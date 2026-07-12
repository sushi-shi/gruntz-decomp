// SBI_ImageSetAni.cpp - CSBI_ImageSetAni::Serialize (0xe7cd0), the frameless slot-1
// serialize the ANI conveyor SBI leaf shares with CSBI_StatzTabArrow (both vtables'
// slot 1 = thunk 0x2829 -> 0xe7cd0). RE-ATTRIBUTED here from SBI_WarlordHead.cpp,
// where it was mis-named CSBI_WarlordHead::Serialize: the vtable proof (gruntz sema
// class) shows CSBI_ImageSetAni/CSBI_StatzTabArrow slot 1 = 0x2829 -> 0xe7cd0, while
// CSBI_WarlordHead slot 1 = thunk 0x3cd8 -> 0xeb970 (that real one now lives in
// SBI_WarlordHead.cpp). The six persistent ints m_3c..m_50 belong to this class
// (size 0x54), not warlord (which serializes only its single m_3c direction).
#define SBI_DTOR_CHAIN           // enable the inline base-dtor bodies (see StatusBarItem.h)
#define SBI_OWN_IMAGESETANI_DTOR // this TU owns the out-of-line ~CSBI_ImageSetAni (0x1047f0)
#include <rva.h>
#include <Mfc.h>
#include <Ints.h>
#include <Gruntz/SBI_ImageSetAni.h>
#include <Gruntz/GameRegistry.h> // canonical g_gameReg singleton (m_world liveness gate)
#include <Gruntz/ResMgr.h>       // canonical g_gameReg->m_world view (CResMgr + CDrawTarget)
#include <Gruntz/SbiConfig.h>    // canonical config-host family (Init's map lookup)
#include <Image/CImage.h>        // the resolved frame record (Tick's blit)

// The g_gameReg singleton (?g_gameReg@@3PAUWwdGameReg@@A @ VA 0x64556c).
DATA(0x0024556c)
extern "C" CGameRegistry* g_gameReg;

// ===========================================================================
// CSBI_ImageSetAni::Init (0xe7980, vtable slot 13): seed the item from a config
// host + rect + record key (14 args; the caller passes the host as BOTH arg0 and
// arg1). Ex CAniPlayer::Init (dossier #16: vtbl 0x1ead6c slot [13] thunk 0x3b48).
// @early-stop
// regalloc/scheduling wall (~65%): logic complete + verified. The 14-arg /O2
// body diverges only in the optimizer's internal choices - a `push edi` shrink-
// wrap retail elides (it keeps everything in eax/ecx/edx), the rect-store common
// base (retail `lea edx,[esi+0x14]` then [edx+N] vs our direct [esi+N]), and the
// field-store interleave order. None are source-steerable (see
// docs/patterns/shrink-wrapped-callee-save-push.md, zero-register-pinning.md).
RVA(0x000e7980, 0x109)
i32 CSBI_ImageSetAni::Init(
    i32 cfg,
    CSbiConfigHost* host,
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
    if (host != 0 && cfg != 0) {
        m_2c = cfg;
        m_10 = a3;
        m_24 = (i32)host;
        m_28 = 0;
        m_4 = 1;
        m_rect14.m_0 = r0;
        m_rect14.m_4 = r1;
        m_rect14.m_8 = r2;
        m_rect14.m_c = r3;
        m_c = a2;
        if (key != 0) {
            CSbiConfigRecord* tbl = 0;
            ((CMapStringToPtr*)&host->m_10->m_10map)->Lookup((const char*)key, (void*&)tbl);
            m_34 = (CSprite*)tbl;
            if (tbl != 0) {
                m_3c = b2;
                m_48 = b3;
                m_44 = b4;
                m_4c = (b0 == -1) ? (b4 >= 0 ? tbl->m_64 : tbl->m_68) : b0;
                m_50 = (b1 == -1) ? (b4 >= 0 ? tbl->m_68 : tbl->m_64) : b1;
                m_38 = m_4c;
                if (m_4c < tbl->m_64 || m_4c > tbl->m_68) {
                    m_30 = 0;
                    return 0;
                }
                m_30 = tbl->m_14[m_4c];
                return m_30 != 0;
            }
        }
    }
    return 0;
}

// ===========================================================================
// CSBI_ImageSetAni::Tick (0xe7b00, vtable slot 5): the timeGetTime-driven cel
// advance within [m_4c, m_50]. Ex CAniPlayer::Tick (dossier #16: vtbl 0x1ead6c
// slot [5] thunk 0x2dfb).
// @early-stop
// regalloc/tail-merge wall (~91%): logic complete + verified. Residual is the
// cel-fetch eax<->ecx register naming (retail pins tbl in ecx, frame in eax; we
// get the reverse) plus the wrap-tail merge (retail keeps the m_44==0 block's
// register decrement separate from the m_48==0 in-memory `dec [esi+0x28]`; our
// recompile tail-merges them). Both are the /O2 allocator/block-merger's choices,
// not source-steerable.
RVA(0x000e7b00, 0xe1)
i32 CSBI_ImageSetAni::Tick() {
    if (m_28 > 0) {
        CSbiConfigRecord* tbl = (CSbiConfigRecord*)m_34;
        CImage* cel;
        if (m_38 >= tbl->m_64 && m_38 <= tbl->m_68) {
            cel = (CImage*)tbl->m_14[m_38];
        } else {
            cel = 0;
        }
        m_30 = (i32)cel;
        if (cel != 0) {
            i32 surfaceCtx = (i32)((CResMgr*)g_gameReg->m_world)->m_drawTarget->m_14;
            cel->RenderFrame(
                (void*)surfaceCtx,
                (void*)(cel->m_anchorX + m_rect14.m_0),
                (void*)(cel->m_anchorY + m_rect14.m_4),
                (void*)0
            );
        }
        u32 now = timeGetTime();
        if (now - (u32)m_40 > (u32)m_3c) {
            m_38 += m_44;
            m_40 = timeGetTime();
        }
        if (m_44 > 0) {
            if (m_38 > m_50) {
                if (m_48 != 0) {
                    m_38 = m_4c;
                    return 1;
                }
                m_38 = m_50;
                m_28--;
            }
        } else if (m_44 < 0) {
            if (m_38 < m_50) {
                if (m_48 != 0) {
                    m_38 = m_4c;
                    return 1;
                }
                m_38 = m_50;
                m_28--;
            }
        } else {
            m_28--;
        }
    }
    return 1;
}

// ===========================================================================
// CSBI_ImageSetAni::SetRange (0xe7c30, vtable slot 14): re-arm the item with a new
// frame window without re-resolving the record: set start/end frames (a -1 means
// "derive from the record's range, ordered by the step sign"), the step, loop flag
// and interval (interval -1 = keep), reset the frame to the start, re-arm 2 play
// cycles and stamp the last-tick clock (via the cached g_pTimeGetTime, not the
// direct import). Ex CAniPlayer::SetRange (dossier #16: slot [14] thunk 0x3bde).
// @early-stop
// 88.2% - logic byte-exact; residual is the -1/record branch shape: retail hoists the
// m_34 load per else-arm and stores in each branch, cl computes the value into a
// register and does one store at the merge (explicit if/else made it worse). Final sweep.
extern "C" u32(WINAPI* g_pTimeGetTime)(); // 0x6c4650  cached timeGetTime fn ptr
RVA(0x000e7c30, 0x7d)
void CSBI_ImageSetAni::SetRange_0e7c30(i32 start, i32 end, i32 step, i32 loop, i32 interval) {
    if (start != -1) {
        m_4c = start;
    } else {
        m_4c = (step >= 0) ? ((CSbiConfigRecord*)m_34)->m_64 : ((CSbiConfigRecord*)m_34)->m_68;
    }
    if (end != -1) {
        m_50 = end;
    } else {
        m_50 = (step >= 0) ? ((CSbiConfigRecord*)m_34)->m_68 : ((CSbiConfigRecord*)m_34)->m_64;
    }
    if (interval != -1) {
        m_3c = interval;
    }
    m_44 = step;
    m_48 = loop;
    m_38 = m_4c;
    m_28 = 2;
    m_40 = g_pTimeGetTime();
}

// vtable slot 1 (0xe7cd0): save/load the six persistent ints (m_3c..m_50) through the
// stream's Read/WriteBytes, then chain the CSBI_ImageSet base serialize and normalize
// its result to a bool. mode 7 = load, mode 4 = save; any other mode just chains.
// Bails early when the stream is null or the active game manager (g_gameReg->m_world)
// is gone.
RVA(0x000e7cd0, 0xf8)
i32 CSBI_ImageSetAni::Serialize(CImageSetStream* s, i32 mode, i32 a3, i32 a4) {
    if (s == 0) {
        return 0;
    }
    if (g_gameReg->m_world == 0) {
        return 0;
    }
    switch (mode) {
        case 7:
            s->ReadBytes(&m_3c, 4);
            s->ReadBytes(&m_40, 4);
            s->ReadBytes(&m_44, 4);
            s->ReadBytes(&m_48, 4);
            s->ReadBytes(&m_4c, 4);
            s->ReadBytes(&m_50, 4);
            break;
        case 4:
            s->WriteBytes(&m_3c, 4);
            s->WriteBytes(&m_40, 4);
            s->WriteBytes(&m_44, 4);
            s->WriteBytes(&m_48, 4);
            s->WriteBytes(&m_4c, 4);
            s->WriteBytes(&m_50, 4);
            break;
    }
    return CSBI_ImageSet::Serialize(s, mode, a3, a4) != 0; // qualified = direct base call
}

// ---------------------------------------------------------------------------
// ~CSBI_ImageSetAni (0x1047f0): the /GX chain destructor - stamp
// ??_7CSBI_ImageSetAni, run the inherited ResetCounters (0xe7400), then MSVC folds
// the four inline base dtors in (ImageSet/Image/RectOnly/StatusBarItem - the
// SBI_DTOR_CHAIN device) behind the /GX SEH frame. The folded ImageSet level calls
// ResetCounters AGAIN, so retail shows two `call 0xe7400` here (@0x2c and @0x41).
// Collapsed from SBI_ImageSetAniEh.cpp (5-level case of
// docs/patterns/eh-dtor-multilevel-polymorphic-chain.md).
RVA(0x001047f0, 0x94)
CSBI_ImageSetAni::~CSBI_ImageSetAni() {
    ResetCounters();
}
