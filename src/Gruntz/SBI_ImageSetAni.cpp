#define SBI_DTOR_CHAIN           // enable the inline base-dtor bodies (see StatusBarItem.h)
#define SBI_OWN_IMAGESETANI_DTOR // this TU owns the out-of-line ~CSBI_ImageSetAni (0x1047f0)
#include <rva.h>
#include <Gruntz/GameRegMfcPtr.h> // g_gameReg at its REAL type (CGruntzMgr)
#include <Gruntz/GruntzMgr.h>
#include <Io/FileMem.h> // CFileMemBase - the CFileMemBase stream (Read/Write dispatch)
#include <Mfc.h>
#include <Ints.h>
#include <Gruntz/SBI_ImageSetAni.h>
#include <Gruntz/GameRegistry.h> // canonical g_gameReg singleton (m_world liveness gate)
#include <DDrawMgr/DDrawSurfaceMgr.h>
#include <DDrawMgr/DDrawWorkerRegistry.h> // m_imageRegistry (full def)
#include <Gruntz/Sprite.h>                // CDDrawWorker (fold: ex via ResMgr.h)
#include <DDrawMgr/DDrawSubMgrPages.h> // the m_drawTarget pages (fold: ex ResMgr.h CDrawTarget)       // canonical g_gameReg->m_world view (CDDrawSurfaceMgr + CDDrawSubMgrPages)
#include <Gruntz/SbiConfig.h>          // canonical config-host family (Init's map lookup)
#include <Image/CImage.h>              // the resolved frame record (Render's blit)

VTBL(CSBI_ImageSetAni, 0x001ead6c); // vtable_names -> code (RTTI game class; was in SbiDtorChain.h)
// ===========================================================================
// CSBI_ImageSetAni::Init (0xe7980, vtable slot 13): seed the item from a config
// host + rect + record key (14 args; the caller passes the host as BOTH arg0 and
// arg1). Ex CAniPlayer::Init (dossier #16: vtbl 0x1ead6c slot [13] thunk 0x3b48).
// 62.9 -> 88.5 (2026-08-01). The old "regalloc wall" note was wrong on all three
// counts: the `push edi` came from the TERNARY window assignments (they made cl batch
// b2/b3/b4 into three live registers at once), the `lea edx,[esi+0x14]` rect base is
// the WHOLE-STRUCT `m_rect14 = rc` copy, and the member offsets themselves were wrong
// (m_step/m_loop and m_frameStart/m_frameEnd were swapped in the header - see there).
// @early-stop
// Residual: cl cross-jumps the key/record `return 0` epilogues onto one shared block
// where retail keeps two inline copies, plus eax<->edx naming in the cel fetch.
RVA(0x000e7980, 0x109)
i32 CSBI_ImageSetAni::Init(
    CStatusBarMgr* owner,
    CDDrawSurfaceMgr* host,
    i32 cmd,
    i32 tab,
    RECT rc,
    const char* key,
    i32 b0,
    i32 b1,
    i32 b2,
    i32 b3,
    i32 b4
) {
    // FOUR separate early-return guards. Nesting the body under `if (host && owner)
    // { if (key) { if (tbl) {...} } }` cross-jumps every exit onto ONE epilogue at the
    // bottom; retail keeps a private `xor eax,eax; pop esi; ret 0x38` per guard
    // (81.7 -> 88.5).
    if (host == 0) {
        return 0;
    }
    if (owner == 0) {
        return 0;
    }
    m_2c = owner;
    m_tab = tab;
    m_24 = host;
    m_28 = 0;
    m_enabled = 1;
    // WHOLE-struct assignment - retail's `lea edx,[esi+0x14]` + [edx+N] block.
    m_rect14 = rc;
    m_cmd = cmd;
    if (key == 0) {
        return 0;
    }
    CObject* found = 0;
    host->m_imageRegistry->m_10map.Lookup(key, found);
    CDDrawWorker* tbl = static_cast<CDDrawWorker*>(found);
    m_34 = tbl;
    if (tbl == 0) {
        return 0;
    }
    m_interval = b2;
    m_loop = b3;
    m_step = b4;
    // EXPLICIT if/else, not a ternary (69.3 -> 81.7, config/axes/sbi-imagesetani-init.json):
    // the ternary makes cl batch all three window loads up front, which needs a
    // fourth register and forces a `push edi` retail does not have.
    if (b0 == -1) {
        m_frameStart = (b4 >= 0) ? tbl->m_minIndex : tbl->m_maxIndex;
    } else {
        m_frameStart = b0;
    }
    if (b1 == -1) {
        m_frameEnd = (b4 >= 0) ? tbl->m_maxIndex : tbl->m_minIndex;
    } else {
        m_frameEnd = b1;
    }
    m_38 = m_frameStart;
    // ONE trailing `return cel != 0` that cl tail-DUPLICATES into both arms:
    // retail's out-of-range arm is `xor ecx,ecx; xor eax,eax; test ecx,ecx;
    // mov [esi+0x30],ecx; setne al` - it re-runs the same setne on the known
    // zero, which a per-arm `return 0` cannot produce.
    CImage* cel;
    if (m_frameStart < tbl->m_minIndex || m_frameStart > tbl->m_maxIndex) {
        cel = 0;
    } else {
        cel = static_cast<CImage*>(tbl->m_items.GetAt(m_frameStart));
    }
    m_frame = cel;
    return cel != 0;
}

RVA(0x000e7ae0, 0x8)
i32 CSBI_ImageSetAni::Refresh(i32) {
    return 1;
}

// ===========================================================================
// CSBI_ImageSetAni::Tick (0xe7b00, vtable slot 5): the timeGetTime-driven cel
// advance within [m_4c, m_50]. Ex CAniPlayer::Tick (dossier #16: vtbl 0x1ead6c
// slot [5] thunk 0x2dfb).
// @early-stop
// tail-merge + regalloc wall (~91.4). Branch sequences AGREE (mnemonics AND symbolic
// targets); base is 210 B against retail's 225 because cl merges two wrap exits retail
// keeps apart - one of which uses a REGISTER decrement, the other `dec [esi+0x28]`.
// Plus the cel-fetch eax<->ecx naming. 27-cell matrix
// (config/axes/sbi-imagesetani-render.json: cel-fetch spelling x anchor-add operand
// order x explicit register decrement) is FLAT - baseline is the product optimum.
RVA(0x000e7b00, 0xe1)
i32 CSBI_ImageSetAni::Render() {
    if (m_28 > 0) {
        CDDrawWorker* tbl = m_34;
        CImage* cel;
        if (m_38 >= tbl->m_minIndex && m_38 <= tbl->m_maxIndex) {
            cel = static_cast<CImage*>(tbl->m_items.GetAt(m_38));
        } else {
            cel = 0;
        }
        m_frame = cel;
        if (cel != 0) {
            CDDrawSurfacePair* surfaceCtx = g_gameReg->m_world->m_drawTarget->m_backPair;
            cel->RenderFrame(
                surfaceCtx,
                cel->m_anchorX + m_rect14.left,
                cel->m_anchorY + m_rect14.top,
                0
            );
        }
        u32 now = timeGetTime();
        if (now - static_cast<u32>(m_lastTime) > static_cast<u32>(m_interval)) {
            m_38 += m_step;
            m_lastTime = timeGetTime();
        }
        if (m_step > 0) {
            if (m_38 > m_frameEnd) {
                if (m_loop != 0) {
                    m_38 = m_frameStart;
                    return 1;
                }
                m_38 = m_frameEnd;
                m_28--;
            }
        } else if (m_step < 0) {
            if (m_38 < m_frameEnd) {
                if (m_loop != 0) {
                    m_38 = m_frameStart;
                    return 1;
                }
                m_38 = m_frameEnd;
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
// cycles and stamp the last-tick clock (via the cached ::timeGetTime, not the
// direct import). Ex CAniPlayer::SetRange (dossier #16: slot [14] thunk 0x3bde).
// 88.2 -> 100.00 EXACT (2026-08-01). The old note said "explicit if/else made it
// worse" - it was tested against the WRONG member offsets (m_frameStart/m_frameEnd
// were swapped in the header). With the offsets fixed, the `== -1`-first polarity
// plus per-arm stores is byte-exact.
RVA(0x000e7c30, 0x7d)
void CSBI_ImageSetAni::SetRange(i32 start, i32 end, i32 step, i32 loop, i32 interval) {
    // `== -1` FIRST (retail `cmp eax,0xffffffff; jne <store arg>`), and EXPLICIT
    // if/else arms, each with its own store - retail writes m_frameStart three times
    // (`mov [esi+0x50],edx` in both record arms and `mov [esi+0x50],eax` for the
    // literal), which a ternary collapses to one store at the merge.
    if (start == -1) {
        if (step >= 0) {
            m_frameStart = m_34->m_minIndex;
        } else {
            m_frameStart = m_34->m_maxIndex;
        }
    } else {
        m_frameStart = start;
    }
    if (end == -1) {
        if (step >= 0) {
            m_frameEnd = m_34->m_maxIndex;
        } else {
            m_frameEnd = m_34->m_minIndex;
        }
    } else {
        m_frameEnd = end;
    }
    if (interval != -1) {
        m_interval = interval;
    }
    m_step = step;
    m_loop = loop;
    m_38 = m_frameStart;
    m_28 = 2;
    m_lastTime = ::timeGetTime();
}

RVA(0x000e7cd0, 0xf8)
i32 CSBI_ImageSetAni::SerializeFields(CFileMemBase* s, i32 mode, i32 typeId, i32 pObj) {
    if (s == 0) {
        return 0;
    }
    if (g_gameReg->m_world == 0) {
        return 0;
    }
    switch (mode) {
        // ASCENDING OFFSET ORDER (+0x3c,+0x40,+0x44,+0x48,+0x4c,+0x50) - retail's
        // `lea eax,[edi+0x3c]` .. `lea eax,[edi+0x50]` walk. The names moved when the
        // m_step/m_loop and m_frameStart/m_frameEnd pairs were un-swapped in the header;
        // the WIRE ORDER did not.
        case 7:
            s->Read(&m_interval, 4);
            s->Read(&m_lastTime, 4);
            s->Read(&m_loop, 4);
            s->Read(&m_step, 4);
            s->Read(&m_frameEnd, 4);
            s->Read(&m_frameStart, 4);
            break;
        case 4:
            s->Write(&m_interval, 4);
            s->Write(&m_lastTime, 4);
            s->Write(&m_loop, 4);
            s->Write(&m_step, 4);
            s->Write(&m_frameEnd, 4);
            s->Write(&m_frameStart, 4);
            break;
    }
    return CSBI_ImageSet::SerializeFields(s, mode, typeId, pObj)
           != 0; // qualified = direct base call
}

RVA_COMPGEN(0x001047c0, 0x1e, ??_GCSBI_ImageSetAni@@UAEPAXI@Z)
RVA(0x001047f0, 0x94)
CSBI_ImageSetAni::~CSBI_ImageSetAni() {
    Reset();
}
