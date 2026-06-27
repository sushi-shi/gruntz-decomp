// TileTriggerWiring.h - a tile-trigger logic container's "build with default
// params" forwarder (C:\Proj\Gruntz; the 0x1163xx region, called from
// WireTileSwitchLogic @0x6c130). AddLogicDefaults (0x1163b0) is a thin __thiscall
// forwarder onto the full factory AddLogic (0x116610, a 0x15..0x1a type switch
// that news the per-type 0x9c-byte logic object): it forwards its five leading
// ids + four trailing ids and materializes the six zeroed 16-byte parameter
// blocks the factory wants in between.
//
// The factory + its parameter blocks are reloc-masked externs; field/method names
// are placeholders, only the call shape is load-bearing (campaign doctrine).
#ifndef GRUNTZ_TILETRIGGERWIRING_H
#define GRUNTZ_TILETRIGGERWIRING_H

#include <Ints.h>
#include <rva.h>

// One 16-byte parameter block the factory takes by value (six of them, all default
// zero on the AddLogicDefaults path).
struct CTrigParam {
    i32 m0, m4, m8, mc;
    CTrigParam() : m0(0), m4(0), m8(0), mc(0) {} // VC5 won't value-init -> zero by ctor
};

class CTileTriggerWiring {
public:
    // 0x116610: the full factory (this, five ids, six param blocks, four ids).
    // Reloc-masked (no body).
    void AddLogic(i32 type, i32 a2, i32 a3, i32 a4, i32 a5, CTrigParam p1, CTrigParam p2,
                  CTrigParam p3, CTrigParam p4, CTrigParam p5, CTrigParam p6, i32 a6, i32 a7,
                  i32 a8, i32 a9);

    // 0x1163b0: forward with six default (zeroed) parameter blocks.
    void AddLogicDefaults(i32 type, i32 a2, i32 a3, i32 a4, i32 a5, i32 a6, i32 a7, i32 a8,
                          i32 a9);
};

#endif // GRUNTZ_TILETRIGGERWIRING_H
