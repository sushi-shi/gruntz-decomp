// CGruntToySprite.h - the "grunt has a toy" indicator sprite, a CUserLogic-
// derived game object (vftables 0x5e705c / 0x5e70b4). The 0x44 dtor folds the
// bare CUserLogic teardown (leaf-dtor archetype); SetCell stashes the grunt cell
// and clears a flag bit; Update tracks the grunt's screen position + layer.
//
// Field names are placeholders; only the OFFSETS + code bytes are load-bearing.
#ifndef GRUNTZ_CGRUNTTOYSPRITE_H
#define GRUNTZ_CGRUNTTOYSPRITE_H

#include <rva.h>

#include <Gruntz/GruntIndicatorSprite.h> // shared registry/entry/renderable types

SIZE_UNKNOWN(CGruntToySprite);
class CGruntToySprite : public CUserLogic {
public:
    ~CGruntToySprite() OVERRIDE; // 0x0122b0 (folds the CUserLogic teardown)

    static void InitActReg();   // 0x07f540 (construct g_toyActReg over [2000,2010])
    static void RegisterActs(); // 0x07f720 (register the class's activation handlers)

    i32 SetCell(i32 x, i32 y); // 0x07f920
    i32 Update();              // 0x07f960

    i32 m_geoId; // +0x40  geometry-id cache slot (indicator-sprite family; unset by this leaf's ApplyLookupSprite ctor)
    char m_pad44[0x54 - 0x44];
    i32 m_cellX;     // +0x54  grunt cell x
    i32 m_cellY;     // +0x58  grunt cell y
    i32 m_lastLayer; // +0x5c  last-seen layer index (Update tracks layer change)
};

// The class registry entry: its first dword receives the Update handler PMF (a
// 4-byte code pointer on this complete single-inheritance class).
typedef i32 (CGruntToySprite::*ToyActHandler)();
SIZE_UNKNOWN(CToyActEntry);
struct CToyActEntry {
    ToyActHandler m_fn;
};

#endif // GRUNTZ_CGRUNTTOYSPRITE_H
