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

// CMenuItem2::SetFrame (0x1847a0, vtable slot 14): trivial frame-index setter.
// Folded from Stub/BoundaryUpper.cpp (B_1847a0::Set - ~??_7CMenuItem2@@6B@+0x38);
// declared in MenuItem2.h slot 14, m_70 already a CMenuItem2 field.
RVA(0x001847a0, 0xa)
void CMenuItem2::SetFrame(i32 v) {
    m_70 = v;
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
    void* sprite;

    sprintf(name, "%s_NORMAL", (const char*)a2);
    sprite = 0;
    m_owner->m_10->m_10.Lookup(name, sprite);
    m_spriteNormal = (CMenuSprite*)sprite;

    sprintf(name, "%s_SELECTED", (const char*)a2);
    sprite = 0;
    m_owner->m_10->m_10.Lookup(name, sprite);
    m_spriteSelected = (CMenuSprite*)sprite;

    sprintf(name, "%s_DISABLED", (const char*)a2);
    sprite = 0;
    m_owner->m_10->m_10.Lookup(name, sprite);
    m_spriteDisabled = (CMenuSprite*)sprite;

    return 1;
}

// slot 8 (0x1858a0): the frame-cursor countdown. Decrement the pending count by
// `arg`; when it runs out, reload it from m_70 and advance one animation frame.
RVA(0x001858a0, 0x2b)
i32 CMenuItem2::Notify(void* arg) {
    u32 a = (u32)arg;
    if (a >= (u32)m_6c) {
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
    if (m_fixedX != (i32)0xeeeeeeee) {
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
    f->RenderFrame((void*)ctx, (void*)py, (void*)px, 0);
    m_hitLeft = py - f->m_anchorX;
    m_hitRight = py + f->m_anchorX;
    m_hitTop = px - f->m_anchorY;
    m_hitBottom = px + f->m_anchorY;
    return 1;
}

// CMenuItem2::GetCurrentSprite (0x00185950) is now an inline member in the header.


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
    CImage* f = s->GetAt(m_frameIdx);
    if (f) {
        return f;
    }
    m_frameIdx = s->m_firstFrame;
    return s->GetAt(m_frameIdx);
}

// advance the cursor one frame; when the looping flag (0x10000) is clear
// or the sprite still has a frame, report whether a frame remains.
RVA(0x001859c0, 0x4e)
i32 CMenuItem2::NextFrame() {
    if (!GetCurrentFrame()) {
        return 0;
    }
    m_frameIdx = m_frameIdx + 1;
    if (m_flags & 0x10000) {
        CMenuSprite* s = GetCurrentSprite();
        if (s) {
            if (m_frameIdx > s->m_lastFrame) {
                m_frameIdx = m_frameIdx - 1;
                return 1;
            }
        }
    }
    return GetCurrentFrame() != 0;
}
