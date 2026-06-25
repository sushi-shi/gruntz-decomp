// CGruntPowerupSprite.h - the "grunt has a powerup" indicator sprite, a
// CUserLogic-derived game object (vftables 0x5e705c / 0x5e70b4). The 0x44 dtor
// folds the bare CUserLogic teardown (leaf-dtor archetype); SetCell stashes the
// grunt cell + powerup id and binds the bute sprite; Update tracks the grunt.
//
// Field names are placeholders; only the OFFSETS + code bytes are load-bearing.
#ifndef GRUNTZ_CGRUNTPOWERUPSPRITE_H
#define GRUNTZ_CGRUNTPOWERUPSPRITE_H

#include <rva.h>

#include <Gruntz/GruntIndicatorSprite.h> // shared registry/entry/renderable types

class CGruntPowerupSprite : public CUserLogic {
public:
    ~CGruntPowerupSprite(); // 0x012370 (folds the CUserLogic teardown)

    i32 SetCell(i32 x, i32 y, i32 powerup); // 0x080380
    i32 Update();                           // 0x080410

    i32 m_40; // +0x40
    char m_pad44[0x54 - 0x44];
    i32 m_54; // +0x54  grunt cell x
    i32 m_58; // +0x58  grunt cell y
    i32 m_5c; // +0x5c  powerup id
};

#endif // GRUNTZ_CGRUNTPOWERUPSPRITE_H
