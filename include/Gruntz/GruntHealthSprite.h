// GruntHealthSprite.h - the grunt health-bar eyecandy sprite (C:\Proj\Gruntz).
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
// passed coordinates (m_cellX/m_cellY), round the passed health to a glyph slot
// (slot = 0x15 - (int)(health*0.2 + 0.5)), resolve that slot through the bound
// object's [m_64..m_68]-gated glyph table at +0x194 (the SAME gated-glyph-table
// archetype as CStatzGlyphMap in SBI_StatzTabGruntBar.h), publish the glyph and
// slot back into the object (+0x198 / +0x190), stash the health (m_health), return 1.
//
// Field names are placeholders; only OFFSETS + the inheritance chain are
// load-bearing.
#ifndef GRUNTZ_CGRUNTHEALTHSPRITE_H
#define GRUNTZ_CGRUNTHEALTHSPRITE_H

#include <rva.h>
// The health glyph resolve reads its bound game object (CUserLogic::m_10) through
// the SAME shared views the indicator sprites use: CGruntRenderable (the bound
// grunt renderable) and its +0x194 gated lookup table CGruntLayerHolder
// (table @+0x14, [m_64..m_68] index gate - the shared gated-lookup archetype,
// also seen as CStatzGlyphMap). No health-local duplicate view is kept.
#include <Gruntz/GruntIndicatorSprite.h> // CIndicatorActReg + g_healthActReg + CGruntRenderable/CGruntLayerHolder
#include <Gruntz/UserLogic.h> // CUserLogic base (CGruntHealthSprite : CUserLogic)
#include <Gruntz/SerialArchive.h> // shared CSerialArchive (Read +0x2c / Write +0x30)
#include <Gruntz/SerialObjRef.h>  // the +0x34 serialized-object-reference (Chain @0x8c00)

SIZE_UNKNOWN(CGruntHealthSprite);
class CGruntHealthSprite : public CUserLogic {
public:
    TILE_LOGIC_TAIL
public:
    static void InitActReg();   // 0x07ecf0 (construct g_healthActReg over [2000,2010])
    void RunAct(i32 id);        // 0x07ed70 (resolve the id's registered handler + dispatch it)
    static void RegisterActs(); // 0x07eed0 (register the class's activation handlers)

    i32 HealthUpdate(); // 0x07f180 (the registered per-frame handler; reconstructed in the .cpp)
    i32 SetHealthGlyph(i32 x, i32 y, i32 health);               // 0x07f0d0
    i32 Serialize(CSerialArchive* ar, i32 mode, i32 a3, i32 a4); // 0x07f270
    // slot 16 (new): the per-class stat-time getter (leaf overrides read the bound
    // grunt's stamina/wingz/toy timer); HealthUpdate dispatches it with the grunt entry.
    virtual i32 Vslot16(CGruntEntry* grunt);
    virtual ~CGruntHealthSprite() OVERRIDE; // 0x011fb0 (folds the CUserLogic teardown)

    // CUserLogic is 0x40; the leaf adds its own fields. SetHealthGlyph stashes the
    // two coordinates at +0x54/+0x58 and the health at +0x5c.
    char m_pad40[0x54 - 0x40];
    i32 m_cellX;  // +0x54  stashed grunt cell x
    i32 m_cellY;  // +0x58  stashed grunt cell y
    i32 m_health; // +0x5c  stashed health value
    i32 m_60;     // +0x60  screen-Y bias (HealthUpdate adds it to the grunt's screen y)
};
VTBL(CGruntHealthSprite, 0x001e7ba4);

// The class registry entry: its first dword receives the handler PMF (a 4-byte
// code pointer on this complete single-inheritance class). CGruntHealthSprite is
// complete above so the PMF stays 4 bytes (matching `mov [entry], offset thunk`).
typedef i32 (CGruntHealthSprite::*HealthActHandler)();
SIZE_UNKNOWN(CHealthActEntry);
struct CHealthActEntry {
    HealthActHandler m_fn;
};

#endif // GRUNTZ_CGRUNTHEALTHSPRITE_H
