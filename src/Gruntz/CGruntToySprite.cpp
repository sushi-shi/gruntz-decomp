// CGruntToySprite.cpp - the "grunt has a toy" indicator sprite (C:\Proj\Gruntz).
// A CUserLogic-derived game object; methods in ascending retail-RVA order:
//   ~CGruntToySprite  @0x0122b0 - the /GX leaf dtor (CUserLogic teardown).
//   SetCell           @0x07f920 - stash the (x,y) grunt cell, clear m_38 bit 0.
//   Update            @0x07f960 - track the grunt's screen pos + layer.
//
// The 0x44 is a DESTRUCTOR (stamps CUserLogic 0x5e705c then CUserBase 0x5e70b4,
// tears down the +0x18 link via ~EngStr @0x16d2a0), NOT a ctor - identical in
// shape to ~CTimeBomb @0x012a70.
#include <Gruntz/CGruntToySprite.h>

// ~CGruntToySprite @0x0122b0 - the CUserLogic-folded /GX leaf dtor (see header).
RVA(0x000122b0, 0x44)
CGruntToySprite::~CGruntToySprite() {}

// SetCell @0x07f920 - stash the (x,y) grunt cell, clear bit 0 of the +0x38 game
// object's +0x40 flags, return 1.
RVA(0x0007f920, 0x21)
i32 CGruntToySprite::SetCell(i32 x, i32 y) {
    m_54 = x;
    m_58 = y;
    m_38->m_40 &= ~1;
    return 1;
}

// Update @0x07f960 - resolve the grunt for cell (m_54,m_58); when present, if its
// layer index changed re-clamp it through the level layer table into the bound
// renderable's layer fields, then copy the grunt's screen position (y biased by
// -0x20) into the bound renderable. Returns 0.
RVA(0x0007f960, 0x85)
i32 CGruntToySprite::Update() {
    CGruntEntry* e = ((CGruntEntry**)(g_gameReg->m_68 + 0x1c))[m_54 * 15 + m_58];
    if (e == 0) {
        return 0;
    }
    i32 layer = e->m_198;
    if (m_5c != layer) {
        CGruntRenderable* r = (CGruntRenderable*)m_10;
        m_5c = layer;
        CGruntLayerHolder* h = r->m_194;
        if (h != 0) {
            i32 mapped;
            if (layer >= h->m_64 && layer <= h->m_68) {
                mapped = h->m_14[layer];
            } else {
                mapped = 0;
            }
            r->m_198 = mapped;
            r->m_190 = layer;
        }
    }
    m_10->m_5c = e->m_10->m_5c;
    m_10->m_60 = e->m_10->m_60 - 0x20;
    return 0;
}
