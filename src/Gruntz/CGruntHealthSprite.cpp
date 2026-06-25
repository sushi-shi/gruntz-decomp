// CGruntHealthSprite.cpp - the grunt health-bar eyecandy sprite (C:\Proj\Gruntz).
//
// Two trace-discovered CGruntHealthSprite methods, defined in ascending
// retail-RVA order:
//   ~CGruntHealthSprite @0x011fb0 - the /GX leaf dtor (folds the CUserLogic teardown).
//   SetHealthGlyph      @0x07f0d0 - the per-bump health-glyph resolver (ret 0xc).
//
// CGruntHealthSprite : CUserLogic (the base hierarchy comes from <Gruntz/UserLogic.h>).
// Only offsets / code bytes are load-bearing; names are placeholders for the
// recovered engine identities.
#include <Gruntz/CGruntHealthSprite.h>

// CGruntHealthSprite::~CGruntHealthSprite @0x011fb0 - the leaf adds no
// destructible members beyond CUserLogic, so its dtor folds the bare CUserLogic
// teardown: store the CUserLogic vptr (0x5e705c), inline-destruct the +0x18 link
// (the embedded ~EngStr call 0x16d2a0), store the CUserBase vptr (0x5e70b4). The
// destructible link forces the /GX EH frame. Byte-identical in shape to the
// established leaf dtors; the empty body is enough for cl.
RVA(0x00011fb0, 0x44)
CGruntHealthSprite::~CGruntHealthSprite() {}

// CGruntHealthSprite::SetHealthGlyph @0x07f0d0 - stash the two passed coordinates
// (m_54/m_58), round the passed health to a glyph slot (slot = 0x15 -
// (int)(health*0.2 + 0.5); the *0.2+0.5 round emits fild/fmul[0.2]/fadd[0.5]/
// __ftol), resolve that slot through the bound object's [m_64..m_68]-gated glyph
// table at +0x194, publish the glyph (+0x198) and slot (+0x190) back into the
// object, stash the health (m_5c), return 1.
RVA(0x0007f0d0, 0x6e)
i32 CGruntHealthSprite::SetHealthGlyph(i32 x, i32 y, i32 health) {
    m_54 = x;
    m_58 = y;
    i32 slot = 0x15 - (i32)((double)health * 0.2 + 0.5);
    CHealthSpriteObj* obj = (CHealthSpriteObj*)m_10;
    CHealthGlyphMap* map = obj->m_194;
    if (map) {
        i32 glyph;
        if (slot >= map->m_64 && slot <= map->m_68) {
            glyph = map->m_14[slot];
        } else {
            glyph = 0;
        }
        obj->m_198 = glyph;
        obj->m_190 = slot;
    }
    m_5c = health;
    return 1;
}
