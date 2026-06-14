#ifndef GAME_CGRUNTZAPP_H
#define GAME_CGRUNTZAPP_H

/*
 * CGruntzApp — the Gruntz application object (CGameApp subclass).
 * .?AVCGruntzApp@@  (size 0x254, same as the base; CGruntzApp adds no fields)
 *
 * Layout ported from tomalla (@approx tomalla 1.0.1.77; base layout). Has graduated
 * into src/Gruntz/GruntzApp.cpp; this comprehension restatement exists only so the
 * class lays out standalone under `gruntz structs` (`gruntz structs` prefers the src/
 * definition on overlap). Neither matched method touches an instance field, so no
 * CGruntzApp-specific members are modeled.
 */

#include "../wap32/cgameapp.h"   // WAP32::CGameApp base (0x254)

class CGruntzApp : public WAP32::CGameApp
{
public:
    CGruntzApp();
    virtual ~CGruntzApp();   // vector deleting destructor
};                           // 0x254 (no fields beyond the base)

#endif /* GAME_CGRUNTZAPP_H */
