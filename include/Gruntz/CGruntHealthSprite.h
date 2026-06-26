// CGruntHealthSprite.h - the grunt health-bar eyecandy sprite (C:\Proj\Gruntz).
//
// CGruntHealthSprite : CUserLogic - a tile-logic leaf in the same game-object
// hierarchy as CSimpleAnimation (proven by its dtor @0x011fb0 stamping the
// CUserLogic vftable 0x5e705c then the CUserBase vftable 0x5e70b4, tearing down
// the +0x18 link via the embedded ~EngStr at 0x16d2a0 - byte-identical in shape
// to the established leaf-dtor archetype). The leaf adds no destructible members
// beyond CUserLogic, so its dtor folds the bare CUserLogic teardown (the /GX
// leaf-dtor archetype).
//
// SetHealthGlyph (0x07f0d0) is the per-bump health-glyph resolver: stash the two
// passed coordinates (m_54/m_58), round the passed health to a glyph slot
// (slot = 0x15 - (int)(health*0.2 + 0.5)), resolve that slot through the bound
// object's [m_64..m_68]-gated glyph table at +0x194 (the SAME gated-glyph-table
// archetype as CStatzGlyphMap in SBI_StatzTabGruntBar.h), publish the glyph and
// slot back into the object (+0x198 / +0x190), stash the health (m_5c), return 1.
//
// Field names are placeholders; only OFFSETS + the inheritance chain are
// load-bearing.
#ifndef GRUNTZ_CGRUNTHEALTHSPRITE_H
#define GRUNTZ_CGRUNTHEALTHSPRITE_H

#include <rva.h>
#include <Gruntz/GruntIndicatorSprite.h> // CIndicatorActReg + g_healthActReg
#include <Gruntz/UserLogic.h>            // CUserLogic base (CGruntHealthSprite : CUserLogic)

// The [m_64..m_68]-gated glyph table the bound object holds at +0x194 (the SAME
// shape as CStatzGlyphMap: glyph table at +0x14, lo/hi index gate at +0x64/+0x68).
struct CHealthGlyphMap {
    char m_pad0[0x14];
    i32* m_14; // +0x14  glyph table
    char m_pad18[0x64 - 0x18];
    i32 m_64; // +0x64  glyph-index range lo gate
    i32 m_68; // +0x68  glyph-index range hi gate
};

// The bound object (CUserLogic::m_10) view this method touches: the resolved-slot
// mirror (+0x190), the gated glyph map (+0x194) and the resolved glyph (+0x198).
// A local view (distinct fields from the shared CGameObject) so the deep offset
// reads stay self-contained.
struct CHealthSpriteObj {
    char m_pad0[0x190];
    i32 m_190;              // +0x190  resolved slot mirror
    CHealthGlyphMap* m_194; // +0x194  gated glyph table
    i32 m_198;              // +0x198  resolved glyph mirror
};

class CGruntHealthSprite : public CUserLogic {
public:
    static void InitActReg();   // 0x07ecf0 (construct g_healthActReg over [2000,2010])
    static void RegisterActs(); // 0x07eed0 (register the class's activation handlers)

    void HealthUpdate(); // 0x07f180 (the registered per-frame handler; defined elsewhere)
    i32 SetHealthGlyph(i32 x, i32 y, i32 health); // 0x07f0d0
    ~CGruntHealthSprite();                        // 0x011fb0 (folds the CUserLogic teardown)

    // CUserLogic is 0x40; the leaf adds its own fields. SetHealthGlyph stashes the
    // two coordinates at +0x54/+0x58 and the health at +0x5c.
    char m_pad40[0x54 - 0x40];
    i32 m_54; // +0x54  stashed x
    i32 m_58; // +0x58  stashed y
    i32 m_5c; // +0x5c  stashed health
};

// The class registry entry: its first dword receives the handler PMF (a 4-byte
// code pointer on this complete single-inheritance class). CGruntHealthSprite is
// complete above so the PMF stays 4 bytes (matching `mov [entry], offset thunk`).
typedef void (CGruntHealthSprite::*HealthActHandler)();
struct CHealthActEntry {
    HealthActHandler m_fn;
};

#endif // GRUNTZ_CGRUNTHEALTHSPRITE_H
