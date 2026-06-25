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

class CGruntToySprite : public CUserLogic {
public:
    ~CGruntToySprite(); // 0x0122b0 (folds the CUserLogic teardown)

    i32 SetCell(i32 x, i32 y); // 0x07f920
    i32 Update();              // 0x07f960

    i32 m_40; // +0x40
    char m_pad44[0x54 - 0x44];
    i32 m_54; // +0x54  grunt cell x
    i32 m_58; // +0x58  grunt cell y
    i32 m_5c; // +0x5c  last-seen layer index (Update tracks layer change)
};

#endif // GRUNTZ_CGRUNTTOYSPRITE_H
