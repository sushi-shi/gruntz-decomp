// MenuItem2.cpp - the animated three-state sprite menu item (C:\Proj\Gruntz).
//
// CMenuItem2 : CMenuItem (vtable 0x5f08f8). The visual overrides + frame-cursor
// helpers recovered from the 0x185750..0x185a10 cluster. Init chains the base
// CMenuItem::Init then resolves three CImageSet sprites by name out of the catalog;
// Draw renders the current animation frame and caches its hit rect; the three
// non-virtual helpers walk the frame cursor. No destructible local => frameless TU.
#include <rva.h>

#include <Gruntz/MenuItem2.h>

#include <stdio.h> // engine sprintf (reloc-masked)

// ===========================================================================

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
    m_68 = 0;
    m_6c = 0;
    m_70 = 0x64;

    char name[0x80];
    void* sprite;

    sprintf(name, "%s_NORMAL", (const char*)a2);
    sprite = 0;
    ((CMenuItemHostOwner*)m_4)->m_10->m_10.Lookup(name, sprite);
    m_5c = (CMenuSprite*)sprite;

    sprintf(name, "%s_SELECTED", (const char*)a2);
    sprite = 0;
    ((CMenuItemHostOwner*)m_4)->m_10->m_10.Lookup(name, sprite);
    m_60 = (CMenuSprite*)sprite;

    sprintf(name, "%s_DISABLED", (const char*)a2);
    sprite = 0;
    ((CMenuItemHostOwner*)m_4)->m_10->m_10.Lookup(name, sprite);
    m_64 = (CMenuSprite*)sprite;

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
    if (m_44 != (i32)0xeeeeeeee) {
        py = m_44;
        px = m_48;
    } else {
        py = x;
        px = y;
    }
    CImage* f = GetCurrentFrame();
    if (!f) {
        return 0;
    }
    f->RenderFrame((void*)ctx, (void*)py, (void*)px, 0);
    m_34 = py - f->m_18;
    m_3c = py + f->m_18;
    m_38 = px - f->m_1c;
    m_40 = px + f->m_1c;
    return 1;
}

// pick the sprite for the current visual state (m_24: 1/2/3).
RVA(0x00185950, 0x1b)
CMenuSprite* CMenuItem2::GetCurrentSprite() {
    switch (m_24) {
        case 1:
            return m_5c;
        case 2:
            return m_60;
        case 3:
            return m_64;
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
    CMenuSprite* s = GetCurrentSprite();
    if (!s) {
        return 0;
    }
    CImage* f = s->GetAt(m_68);
    if (f) {
        return f;
    }
    m_68 = s->m_64;
    return s->GetAt(m_68);
}

// advance the cursor one frame; when the looping flag (0x10000) is clear
// or the sprite still has a frame, report whether a frame remains.
RVA(0x001859c0, 0x4e)
i32 CMenuItem2::NextFrame() {
    if (!GetCurrentFrame()) {
        return 0;
    }
    m_68 = m_68 + 1;
    if (m_20 & 0x10000) {
        CMenuSprite* s = GetCurrentSprite();
        if (s) {
            if (m_68 > s->m_68) {
                m_68 = m_68 - 1;
                return 1;
            }
        }
    }
    return GetCurrentFrame() != 0;
}
