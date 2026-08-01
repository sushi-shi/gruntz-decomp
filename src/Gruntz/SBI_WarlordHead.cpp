#define SBI_DTOR_CHAIN // enable the inline base-dtor bodies (see StatusBarItem.h)
#include <rva.h>
#include <Gruntz/GameRegMfcPtr.h> // g_gameReg at its REAL type (CGruntzMgr)
#include <Gruntz/GruntzMgr.h>
#include <Io/FileMem.h> // CFileMemBase - the CFileMemBase stream (Read/Write dispatch)
#include <Mfc.h>
#include <Ints.h>
#include <Gruntz/SBI_WarlordHead.h>
#include <Image/ImageSet.h> // canonical CDDrawWorker (the m_34 config record; ex CWhConfig view)
#include <DDrawMgr/DDrawShadeBlit.h> // full CImage::m_owned (CDDrawShadeBlit) for the +0x1c latch
#include <Gruntz/GameRegistry.h>     // canonical g_gameReg singleton + CDDrawSurfaceMgr m_world
#include <DDrawMgr/DDrawSurfaceMgr.h>
#include <Gruntz/Sprite.h>             // CDDrawWorker (fold: ex via ResMgr.h)
#include <DDrawMgr/DDrawSubMgrPages.h> // the m_drawTarget pages (fold: ex ResMgr.h CDrawTarget)           // CDDrawSubMgrPages (m_world->m_drawTarget->m_backPair)

VTBL(CSBI_WarlordHead, 0x001ead24); // vtable_names -> code (RTTI game class)
// vtable slot 11 (0xeb6b0): forward all 11 setup args to the ImageSet base setup
// (the four rect ints fold into one by-value aggregate so MSVC stages the 0x10 temp
// on the caller stack exactly as retail does); on success latch the initial state
// (SetState(0)) and return 1, else return the base's result (0).
RVA(0x000eb6b0, 0x67)
i32 CSBI_WarlordHead::SetupImage(
    CStatusBarMgr* owner,
    CDDrawSurfaceMgr* host,
    i32 cmd,
    i32 tab,
    RECT rc,
    const char* key,
    i32 frame,
    i32 extra
) {
    if (CSBI_ImageSet::SetupImage(owner, host, cmd, tab, rc, key, frame, extra) == 0) {
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
RVA(0x000eb740, 0xb3)
i32 CSBI_WarlordHead::ShowFrames(i32 show, CShadeTable* palDescr) {
    CDDrawWorker* cfg = m_34;
    if (cfg == 0) {
        return 0;
    }

    CImage* f = (cfg->m_minIndex <= 1 && cfg->m_maxIndex >= 1)
                    ? static_cast<CImage*>(cfg->m_items.GetAt(1))
                    : 0;
    if (f == 0) {
        return 0;
    }
    if (f->m_owned) {
        f->m_owned->Select(show, 0);
    }
    if (palDescr && f->m_owned) {
        f->m_owned->m_palDescr = palDescr;
    }

    f = (cfg->m_minIndex <= 2 && cfg->m_maxIndex >= 2) ? static_cast<CImage*>(cfg->m_items.GetAt(2))
                                                       : 0;
    if (f == 0) {
        return 0;
    }
    if (f->m_owned) {
        f->m_owned->Select(show, 0);
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
// at the base origin plus the frame record's own m_rect14.top/m_1c offset.
// @early-stop
RVA(0x000eb880, 0xbd)
i32 CSBI_WarlordHead::Render() {
    if (m_28 > 0) {
        m_28--;
        CDDrawSurfacePair* target = g_gameReg->m_world->m_drawTarget->m_backPair;

        CDDrawWorker* cfg = m_34;
        CImage* f;
        if (m_direction == 1) {
            f = (cfg->m_minIndex > 3 || cfg->m_maxIndex < 3)
                    ? 0
                    : static_cast<CImage*>(cfg->m_items.GetAt(3));
        } else {
            f = (cfg->m_minIndex > 4 || cfg->m_maxIndex < 4)
                    ? 0
                    : static_cast<CImage*>(cfg->m_items.GetAt(4));
        }
        if (f) {
            f->RenderFrame(target, m_rect14.left + f->m_anchorX, m_rect14.top + f->m_anchorY, 0);
        }

        cfg = m_34;
        i32 idx = m_38;
        CImage* g = (idx < cfg->m_minIndex || idx > cfg->m_maxIndex)
                        ? 0
                        : static_cast<CImage*>(cfg->m_items.GetAt(idx));
        m_frame = g;
        if (g) {
            g->RenderFrame(target, m_rect14.left + g->m_anchorX, m_rect14.top + g->m_anchorY, 0);
        }
    }
    return 1;
}

// vtable slot 1 (0xeb970): save/load the head's single persistent direction (m_3c)
// through the stream's Write/ReadBytes, then chain the CSBI_ImageSet base serialize
// (0xe74f0) and normalize its result to a bool. mode 4 = save, mode 7 = load; any
// other mode just chains. Bails early when the stream is null or the active game
// manager (g_gameReg->m_world) is gone. Re-homed from src/Stub/BoundaryLowerMethods.cpp
// (was the Ceb970 placeholder view); vtable slot 1 (thunk 0x3cd8) proves the owner.
RVA(0x000eb970, 0x72)
i32 CSBI_WarlordHead::SerializeFields(CFileMemBase* s, i32 mode, i32 typeId, i32 pObj) {
    if (s == 0) {
        return 0;
    }
    if (g_gameReg->m_world == 0) {
        return 0;
    }
    switch (mode) {
        case 7:
            s->Read(&m_direction, 4);
            break;
        case 4:
            s->Write(&m_direction, 4);
            break;
    }
    return CSBI_ImageSet::SerializeFields(s, mode, typeId, pObj)
           != 0; // qualified = direct base call
}

RVA_COMPGEN(0x001049d0, 0x1e, ??_GCSBI_WarlordHead@@UAEPAXI@Z)
RVA(0x00104a00, 0x94)
CSBI_WarlordHead::~CSBI_WarlordHead() {
    Reset();
}
