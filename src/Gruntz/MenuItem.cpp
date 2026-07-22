#define GRUNTZ_MENUITEM_TU // own the 0x185510 Dispatch0c label (see MenuItem.h)
#include <rva.h>
#include <Gruntz/ChatBox.h>
#include <Gruntz/ChatBoxOwner.h>
#include <Image/CImage.h>
#include <DDrawMgr/DDrawWorker.h> // CDDrawWorker - the resolved m_sprite (frame strip Place indexes)

#include <Gruntz/MenuItem.h>
#include <Gruntz/MenuItem2.h>

#include <stdio.h> // engine sprintf (reloc-masked; CMenuItem2::Init)

VTBL(CMenuItem, 0x001f08c0);

VTBL(CMenuItem2, 0x001f08f8);

RVA(0x00184610, 0x20)
CString CMenuItem::GetField54() {
    return m_54;
}
RVA(0x00184630, 0x20)
CString CMenuItem::GetField58() {
    return m_58;
}
RVA(0x00184690, 0x91)
inline CMenuItem::~CMenuItem() {
    Dispatch0c();
}
RVA(0x00184730, 0x41)
void CMenuItem::Reset() {
    m_host = 0;
    m_template = 0;
    m_sprite = 0;
    m_owner = 0;
    m_listPos = 0;
    m_hitLeft = static_cast<i32>(0xeeeeeeee);
    m_fixedX = static_cast<i32>(0xeeeeeeee);
    m_navFwdName.Empty();
    m_navBackName.Empty();
    m_54.Empty();
    m_58.Empty();
}
RVA(0x001847a0, 0xa)
void CMenuItem2::SetFrame(i32 v) {
    m_70 = v;
}
RVA(0x001847e0, 0xa6)
CMenuItem2::~CMenuItem2() {
    Dispatch0c();
    // compiler stamps ??_7CMenuItem2@@6B@ at entry, then the base ~CMenuItem is
    // inlined here: it stamps ??_7CMenuItem@@6B@, runs its Dispatch0c hook, and
    // destroys m_58/m_54/m_50/m_4c/m_14/m_10 (reverse declaration order).
}
RVA(0x00185460, 0xa9)
i32 CMenuItem::Init(i32 a0, i32 a1, i32 a2, i32 a3, i32 a4, i32 a5) {
    CMenuItemTemplate* t = reinterpret_cast<CMenuItemTemplate*>(a0);
    if (!t) {
        return 0;
    }
    m_flags = a5;
    m_owner = t->m_0;
    m_host = t->m_4;
    m_template = t;
    m_name = reinterpret_cast<const char*>(a1);
    m_key = reinterpret_cast<const char*>(a4);
    m_cmdId = a3;
    m_1c = 0;
    m_cmdParam = 0;
    if (m_flags & 1) {
        m_state = 3;
    } else {
        m_state = 1;
    }
    if (!OnInit()) {
        CObject* slot = 0;
        // ?Lookup@CMapStringToOb@@QBEHPBDAAPAVCObject@@@Z @0x1b8008.  NOT a COMDAT fold
        // with CMapStringToPtr::Lookup - MSVC5 has no /OPT:ICF, and CMapStringToPtr's
        // Lookup is its own body at 0x1b8438 in a different .obj band.  The two classes
        // are code-identical, which is why every FID row there is AMBIG; the binary
        // names them itself (mfc_class 0x1b8008).
        m_owner->m_catalog->m_10.Lookup(reinterpret_cast<const char*>(a2), slot);
        m_sprite = slot;
        if (!slot) {
            return 0;
        }
    }
    return 1;
}
RVA(0x00185520, 0x2c)
i32 CMenuItem::GetFrameWidth() {
    CImageSet* s = static_cast<CImageSet*>(m_sprite);
    if (!s) {
        return 0;
    }
    CImage* f = s->GetAt(2);
    if (!f) {
        return 0;
    }
    return f->m_width;
}
RVA(0x00185550, 0x2c)
i32 CMenuItem::GetWidth() {
    CImageSet* s = static_cast<CImageSet*>(m_sprite);
    if (!s) {
        return 0;
    }
    CImage* f = s->GetAt(2);
    if (!f) {
        return 0;
    }
    return f->m_height;
}
RVA(0x00185580, 0x4a)
i32 CMenuItem::NotifyCmd() {
    i32 id = m_cmdId;
    if (!id) {
        return id;
    }
    HWND wnd = m_host->m_wnd;
    if (wnd) {
        PostMessageA(wnd, WM_COMMAND, id, m_cmdParam);
    }
    if (m_1c && wnd) {
        PostMessageA(wnd, WM_COMMAND, m_1c, 0);
    }
    return 1;
}
// place the item: pick the row out of the sub-page (m_28), have it lay
// itself out at the cached or argument coordinates, then cache the placed rect.
// @early-stop
// regalloc tie (~99.1%): the whole body is byte-aligned; the residual is which of
// the two callee-saved regs (ebx vs ebp) holds the m_44/m_48 coordinate pair --
// retail pins the cmp operand m_44 in ebp, the recompile in ebx. The two are
// interchangeable and the choice is not source-steerable (operand/decl reorderings
// all canonicalize to the same pick). Logic complete; deferred to the final sweep.
RVA(0x001855f0, 0x94)
i32 CMenuItem::Place(i32 ctx, i32 x, i32 y) {
    CDDrawWorker* page = static_cast<CDDrawWorker*>(m_sprite);
    if (!page) {
        return 0;
    }
    i32 py, px;
    if (m_fixedX != static_cast<i32>(0xeeeeeeee)) {
        py = m_fixedX;
        px = m_fixedY;
    } else {
        py = x;
        px = y;
    }
    i32 idx = m_state;
    CImage* row;
    if (idx >= page->m_minIndex && idx <= page->m_maxIndex) {
        row = static_cast<CImage*>(page->m_items.GetAt(idx));
    } else {
        row = 0;
    }
    if (!row) {
        return 0;
    }
    row->RenderFrame(
        reinterpret_cast<void*>(ctx),
        reinterpret_cast<void*>(py),
        reinterpret_cast<void*>(px),
        static_cast<void*>(0)
    );
    m_hitLeft = py - row->m_anchorX;
    m_hitRight = py + row->m_anchorX;
    m_hitTop = px - row->m_anchorY;
    m_hitBottom = px + row->m_anchorY;
    return 1;
}
RVA(0x00185690, 0x25)
i32 CMenuItem::Configure(void* notify) {
    if (notify) {
        m_host->ScrollRow0();
    }
    Disable(2);
    return 1;
}
RVA(0x001856d0, 0x25)
i32 CMenuItem::Trigger() {
    m_host->ScrollRow1();
    NotifyCmd();
    m_host->ReplaceNode(*reinterpret_cast<void**>(&m_key));
    return 1;
}
RVA(0x00185700, 0x4b)
i32 CMenuItem::Hit(i32 x, i32 y) {
    if (m_hitLeft == static_cast<i32>(0xeeeeeeee)) {
        return 0;
    }
    if (x < m_hitLeft) {
        return 0;
    }
    if (x > m_hitRight) {
        return 0;
    }
    if (y < m_hitTop) {
        return 0;
    }
    return y <= m_hitBottom;
}
// configure from a template + strings (chaining CMenuItem::Init), then
// resolve the three per-state sprites by "<key>_NORMAL/_SELECTED/_DISABLED".
// @early-stop
// scheduling artifact (~91.6%): every instruction is byte-identical (incl. the base
// Init forwarding, all three sprintf+Lookup blocks, stack layout) -- the only residual
// is where MSVC schedules the `sprite = 0` store: retail interleaves the `mov [out],0`
// after both Lookup arg-pushes, the recompile hoists it adjacent to the &out lea. Not
// source-steerable (no statement order forces a plain store past the call arg setup).
RVA(0x00185750, 0x123)
i32 CMenuItem2::Init(i32 a0, i32 a1, i32 a2, i32 a3, i32 a4, i32 a5) {
    if (!a0) {
        return 0;
    }
    if (!CMenuItem::Init(a0, a1, a2, a3, a4, a5)) {
        return 0;
    }
    m_frameIdx = 0;
    m_6c = 0;
    m_70 = 0x64;

    char name[0x80];
    CObject* sprite; // CMapStringToOb's value slot (Lookup @0x1b8008 takes CObject*&)

    sprintf(name, "%s_NORMAL", reinterpret_cast<const char*>(a2));
    sprite = 0;
    m_owner->m_catalog->m_10.Lookup(name, sprite);
    m_spriteNormal = static_cast<CImageSet*>(sprite);

    sprintf(name, "%s_SELECTED", reinterpret_cast<const char*>(a2));
    sprite = 0;
    m_owner->m_catalog->m_10.Lookup(name, sprite);
    m_spriteSelected = static_cast<CImageSet*>(sprite);

    sprintf(name, "%s_DISABLED", reinterpret_cast<const char*>(a2));
    sprite = 0;
    m_owner->m_catalog->m_10.Lookup(name, sprite);
    m_spriteDisabled = static_cast<CImageSet*>(sprite);

    return 1;
}
RVA(0x001858a0, 0x2b)
i32 CMenuItem2::Notify(void* arg) {
    u32 a = reinterpret_cast<u32>(arg);
    if (a >= static_cast<u32>(m_6c)) {
        m_6c = m_70;
        NextFrame();
        return 1;
    }
    m_6c = m_6c - a;
    return 1;
}
// the slot-9 Place override: draw the current animation frame at the placed (or
// argument) coordinates, then cache the resulting hit rect.
// @early-stop
// regalloc tie (~98.8%): body byte-aligned; the residual is which callee-saved reg
// (ebx vs ebp) holds the py/px coordinate pair -- retail pins py(m_44) in ebp, the
// recompile in ebx. Identical not-source-steerable tie as CMenuItem::Place (0x1855f0).
RVA(0x001858d0, 0x72)
i32 CMenuItem2::Place(i32 ctx, i32 x, i32 y) {
    i32 py, px;
    if (m_fixedX != static_cast<i32>(0xeeeeeeee)) {
        py = m_fixedX;
        px = m_fixedY;
    } else {
        py = x;
        px = y;
    }
    CImage* f = GetCurrentFrame();
    if (!f) {
        return 0;
    }
    f->RenderFrame(
        reinterpret_cast<void*>(ctx),
        reinterpret_cast<void*>(py),
        reinterpret_cast<void*>(px),
        0
    );
    m_hitLeft = py - f->m_anchorX;
    m_hitRight = py + f->m_anchorX;
    m_hitTop = px - f->m_anchorY;
    m_hitBottom = px + f->m_anchorY;
    return 1;
}
RVA(0x00185950, 0x1b)
CImageSet* CMenuItem2::GetCurrentSprite() {
    switch (m_state) {
        case 1:
            return m_spriteNormal;
        case 2:
            return m_spriteSelected;
        case 3:
            return m_spriteDisabled;
    }
    return 0;
}
// resolve the frame at the current cursor; if absent, rewind the cursor
// to the sprite's first index and try once more.
// @early-stop
// shrink-wrapped callee-save wall (~60.7%): body byte-identical to retail (both GetAt
// inlines, the eax-reuse on the retry) -- retail defers `push edi` past the early
// `if(!s) return 0` guard (saving only esi there), the recompile pushes edi upfront so
// every return epilogue differs by a pop. docs/patterns/shrink-wrapped-callee-save-push.md.
RVA(0x00185970, 0x4d)
CImage* CMenuItem2::GetCurrentFrame() {
    CImageSet* s = GetCurrentSprite();
    if (!s) {
        return 0;
    }
    CImage* f = s->GetAt(m_frameIdx);
    if (f) {
        return f;
    }
    m_frameIdx = s->m_minIndex;
    return s->GetAt(m_frameIdx);
}
RVA(0x001859c0, 0x4e)
i32 CMenuItem2::NextFrame() {
    if (!GetCurrentFrame()) {
        return 0;
    }
    m_frameIdx = m_frameIdx + 1;
    if (m_flags & 0x10000) {
        CImageSet* s = GetCurrentSprite();
        if (s) {
            if (m_frameIdx > s->m_maxIndex) {
                m_frameIdx = m_frameIdx - 1;
                return 1;
            }
        }
    }
    return GetCurrentFrame() != 0;
}
