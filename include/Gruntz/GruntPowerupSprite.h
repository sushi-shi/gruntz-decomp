// GruntPowerupSprite.h - the "grunt has a powerup" indicator sprite, a
// CUserLogic-derived game object (vftables 0x5e705c / 0x5e70b4). The 0x44 dtor
// folds the bare CUserLogic teardown (leaf-dtor archetype); SetCell stashes the
// grunt cell + powerup id and binds the bute sprite; Update tracks the grunt.
//
// Field names are placeholders; only the OFFSETS + code bytes are load-bearing.
#ifndef GRUNTZ_CGRUNTPOWERUPSPRITE_H
#define GRUNTZ_CGRUNTPOWERUPSPRITE_H

#include <rva.h>

#include <Gruntz/GruntIndicatorSprite.h> // shared registry/entry/renderable types
#include <Gruntz/SerialArchive.h>        // shared CSerialArchive (Read +0x2c / Write +0x30)

class CGruntPowerupSprite : public CUserLogic {
public:
    TILE_LOGIC_TAIL
public:
    virtual ~CGruntPowerupSprite() OVERRIDE; // 0x012370 (folds the CUserLogic teardown)

    static void InitActReg();   // 0x07ffa0 (construct g_powerupActReg over [2000,2010])
    void RunAct(i32 id);        // 0x080020 (resolve the id's registered handler + dispatch it)
    static void RegisterActs(); // 0x080180 (register the class's activation handlers)

    i32 SetCell(i32 x, i32 y, i32 powerup); // 0x080380
    i32 Update();                           // 0x080410
    // 0x080490: the serialize override - chain CUserLogic::SerializeChain + the +0x34
    // sub-object, then round-trip m_cellX/m_cellY (8 B) and m_powerupId (4 B). On read (mode 7) it
    // re-resolves the powerup's bute-set record from g_gameReg->m_78 into the bound
    // renderable (m_10). (__thiscall: ret 0x10.)
    i32 Serialize(CSerialArchive* ar, i32 mode, i32 a3, i32 a4);

    i32 m_geoId; // +0x40  cached bound-object geometry id (ctor: m_38->m_geoId)
    char m_pad44[0x54 - 0x44];
    i32 m_cellX;     // +0x54  grunt cell x
    i32 m_cellY;     // +0x58  grunt cell y
    i32 m_powerupId; // +0x5c  powerup id
};

// The class registry entry: its first dword receives the Update handler PMF (a
// 4-byte code pointer on this complete single-inheritance class).
typedef i32 (CGruntPowerupSprite::*PowerupActHandler)();
struct CPowerupActEntry {
    PowerupActHandler m_fn;
};

#endif // GRUNTZ_CGRUNTPOWERUPSPRITE_H
