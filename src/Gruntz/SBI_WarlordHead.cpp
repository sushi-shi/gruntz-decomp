#define SBI_DTOR_CHAIN // enable the inline base-dtor bodies (see StatusBarItem.h)
#include <rva.h>
#include <Gruntz/GameRegMfcPtr.h> // g_gameReg at its REAL type (CGruntzMgr)
#include <Gruntz/GruntzMgr.h>
#include <Io/FileMem.h> // CFileMemBase - the CSerialArchive stream (Read/Write dispatch)
#include <Mfc.h>
#include <Ints.h>
#include <Gruntz/SBI_WarlordHead.h>
#include <Image/ImageSet.h> // canonical CImageSet (the m_34 config record; ex CWhConfig view)
#include <DDrawMgr/DDrawShadeBlit.h> // full CImage::m_owned (CDDrawShadeBlit) for the +0x1c latch
#include <Gruntz/GameRegistry.h>     // canonical g_gameReg singleton + CDDrawSurfaceMgr m_world
#include <DDrawMgr/DDrawSurfaceMgr.h>
#include <Gruntz/Sprite.h>             // CSprite (fold: ex via ResMgr.h)
#include <DDrawMgr/DDrawSubMgrPages.h> // the m_drawTarget pages (fold: ex ResMgr.h CDrawTarget)           // CDDrawSubMgrPages (m_world->m_drawTarget->m_backPair)

VTBL(CSBI_WarlordHead, 0x001ead24); // vtable_names -> code (RTTI game class)
// vtable slot 11 (0xeb6b0): forward all 11 setup args to the ImageSet base setup
// (the four rect ints fold into one by-value aggregate so MSVC stages the 0x10 temp
// on the caller stack exactly as retail does); on success latch the initial state
// (SetState(0)) and return 1, else return the base's result (0).
// @early-stop
// ~46% (thin-forwarding-wrapper regalloc wall, docs/patterns/serialize-wrapper-reg-
// forward.md): the call sequence, the 0x10 temp build, the SetState(0) tail and the
// `test eax;jne;ret 0x2c` control flow are all byte-correct, BUT retail forwards the
// 11 args using only `push esi` (caller-saved eax/ecx/edx scratch to copy a9..a11
// stack->stack), while MSVC5 here spills two extra callee-saved regs (`push ebx`/
// `push edi`) to stage the trailing args across the by-value-aggregate stores. No
// struct-construction spelling (named local, inline ctor temp, field order) flips
// the reg choice. Plus the reloc-masked BaseSetupImage/SetState rel32. Deferred to
// the final sweep (whole-hierarchy model).
RVA(0x000eb6b0, 0x67)
i32 CSBI_WarlordHead::SetupImage(
    CStatusBarMgr* owner,
    CDDrawSurfaceMgr* host,
    i32 a3,
    i32 a4,
    SbRect rc,
    const char* key,
    i32 a10,
    i32 a11
) {
    if (CSBI_ImageSet::SetupImage(owner, host, a3, a4, rc, key, a10, a11) == 0) {
        return 0;
    }
    SetState(0);
    return 1;
}

// 0xeb740: drive the show/hide of the two anchor frames (frame-table slots 1 and
// 2). For each slot, range-gate the index against the config record's m_64/m_68;
// if the frame exists, fire its sprite handle's show/hide notifier and (when a
// non-zero arg2 is supplied) latch arg2 into the handle's m_1c.
// @early-stop
// ~92.3% (reloc-residual plateau): code bytes byte-identical to retail; the two
// `call WhShowItem` (0x14dd90) rel32 are reloc-masked against a differently-named
// symbol (docs/patterns/reloc-typing-vptr-global.md). Exact once it co-names.
RVA(0x000eb740, 0xb3)
i32 CSBI_WarlordHead::ShowFrames(i32 show, ShadeDescr* palDescr) {
    CImageSet* cfg = m_34;
    if (cfg == 0) {
        return 0;
    }

    CImage* f = (cfg->m_minIndex <= 1 && cfg->m_maxIndex >= 1) ? static_cast<CImage*>(cfg->m_items.GetAt(1)) : 0;
    if (f == 0) {
        return 0;
    }
    if (f->m_owned) {
        WhShowItem(show, 0);
    }
    if (palDescr && f->m_owned) {
        f->m_owned->m_palDescr = palDescr;
    }

    f = (cfg->m_minIndex <= 2 && cfg->m_maxIndex >= 2) ? static_cast<CImage*>(cfg->m_items.GetAt(2)) : 0;
    if (f == 0) {
        return 0;
    }
    if (f->m_owned) {
        WhShowItem(show, 0);
    }
    if (palDescr && f->m_owned) {
        f->m_owned->m_palDescr = palDescr;
    }
    return 1;
}

RVA(0x000eb830, 0x31)
i32 CSBI_WarlordHead::SetState(i32 dir) {
    if (dir == 0 || dir == 1) {
        m_direction = dir;
        m_38 = 1;
        return 1;
    }
    m_direction = dir;
    m_38 = 2;
    return 1;
}

// vtable slot 5 (0xeb880): the per-frame render. Idle (return 1) while the frame
// countdown is non-positive; otherwise tick it down, pull the surface context from
// the active drawable, and blit two frames: the direction frame (table slot 3 or 4
// per m_3c) and the indexed frame (table slot m_38, latched into m_30). Each draws
// at the base origin plus the frame record's own m_rect14.m_4/m_1c offset.
// @early-stop
// ~87.4% (reloc-residual plateau): code bytes byte-identical to retail; the two
// `call RenderFrame` (0x153790) rel32 + the g_gameReg DIR32 are reloc-masked against
// differently-named symbols (docs/patterns/reloc-typing-vptr-global.md). Same
// plateau as CSBI_SideTab::Render (87.8%).
RVA(0x000eb880, 0xbd)
i32 CSBI_WarlordHead::Render() {
    if (m_28 <= 0) {
        return 1;
    }
    m_28--;
    i32 ctx = reinterpret_cast<i32>(g_gameReg->m_world->m_drawTarget->m_backPair);

    CImageSet* cfg = m_34;
    CImage* f;
    if (m_direction == 1) {
        f = (cfg->m_minIndex > 3 || cfg->m_maxIndex < 3) ? 0 : static_cast<CImage*>(cfg->m_items.GetAt(3));
    } else {
        f = (cfg->m_minIndex > 4 || cfg->m_maxIndex < 4) ? 0 : static_cast<CImage*>(cfg->m_items.GetAt(4));
    }
    if (f) {
        f->RenderFrame(
            reinterpret_cast<void*>(ctx),
            reinterpret_cast<void*>((m_rect14.m_0 + f->m_anchorX)),
            reinterpret_cast<void*>((m_rect14.m_4 + f->m_anchorY)),
            0
        );
    }

    cfg = m_34;
    i32 idx = m_38;
    CImage* g = (idx < cfg->m_minIndex || idx > cfg->m_maxIndex) ? 0 : static_cast<CImage*>(cfg->m_items.GetAt(idx));
    m_frame = g;
    if (g) {
        g->RenderFrame(
            reinterpret_cast<void*>(ctx),
            reinterpret_cast<void*>((m_rect14.m_0 + g->m_anchorX)),
            reinterpret_cast<void*>((m_rect14.m_4 + g->m_anchorY)),
            0
        );
    }
    return 1;
}

// vtable slot 1 (0xeb970): save/load the head's single persistent direction (m_3c)
// through the stream's Write/ReadBytes, then chain the CSBI_ImageSet base serialize
// (0xe74f0) and normalize its result to a bool. mode 4 = save, mode 7 = load; any
// other mode just chains. Bails early when the stream is null or the active game
// manager (g_gameReg->m_world) is gone. Re-homed from src/Stub/BoundaryLowerMethods.cpp
// (was the Ceb970 placeholder view); vtable slot 1 (thunk 0x3cd8) proves the owner.
// @early-stop
// block-layout wall: the mode==4 Write branch lands inline (jne-skip) but retail
// floats it to the tail (forward je). The m_3c transfer, the base-chain call and the
// neg/sbb/neg bool are byte-faithful.
RVA(0x000eb970, 0x72)
i32 CSBI_WarlordHead::SerializeFields(CImageSetStream* s, i32 mode, i32 a3, i32 a4) {
    if (s == 0) {
        return 0;
    }
    if (g_gameReg->m_world == 0) {
        return 0;
    }
    if (mode == 4) {
        s->Write(&m_direction, 4);
    } else if (mode == 7) {
        s->Read(&m_direction, 4);
    }
    return CSBI_ImageSet::SerializeFields(s, mode, a3, a4) != 0; // qualified = direct base call
}

RVA(0x00104a00, 0x94)
CSBI_WarlordHead::~CSBI_WarlordHead() {
    ResetCounters();
}
