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
SIZE_UNKNOWN(CTrigParam);

// The +0x7c sub-object of the source tile record, holding two more CTrigParam blocks.
struct CTrigRecordSub {
    char _00[0xf0];
    CTrigParam m_f0;  // +0xf0
    CTrigParam m_100; // +0x100
};
SIZE_UNKNOWN(CTrigRecordSub);

// The source tile record AddLogicFromRecord marshals into the AddLogic factory: five
// ids (m_4/m_164/m_168 + the two caller args) and six CTrigParam blocks (m_64, three
// consecutive at m_134/m_144/m_154, and m_7c->m_f0/m_100). Identity unrecovered
// (the per-tile trigger-source record); placeholder view - only the offsets matter.
struct CTrigSourceRecord {
    char _00[0x04];
    i32 m_4; // +0x04
    char _08[0x64 - 0x08];
    CTrigParam m_64; // +0x64
    char _74[0x7c - 0x74];
    CTrigRecordSub* m_7c; // +0x7c
    char _80[0x118 - 0x80];
    i32 m_118; // +0x118
    char _11c[0x120 - 0x11c];
    i32 m_120; // +0x120
    i32 m_124; // +0x124
    i32 m_128; // +0x128
    char _12c[0x134 - 0x12c];
    CTrigParam m_134; // +0x134
    CTrigParam m_144; // +0x144
    CTrigParam m_154; // +0x154
    i32 m_164;        // +0x164
    i32 m_168;        // +0x168
};
SIZE_UNKNOWN(CTrigSourceRecord);

class CTileTriggerWiring {
public:
    // 0x116610: the full factory (this, five ids, six param blocks, four ids).
    // Declared-only (no body) - both AddLogic* forwarders' CALLs to it stay
    // reloc-UNBOUND. @reloc-TODO: @rva-symbol cannot bind an undefined external,
    // so this only binds once 0x116610's body is reconstructed in this TU's gap
    // (0x1165b6..0x116a40). It is an 812-byte /GX factory (a 0x15..0x1a jump-table
    // type switch that news the per-type 0x9c CTileTriggerLogic leaf, vptr-stamps
    // ??_7CTileTriggerLogic@@6B@, and shares an operator-new EH tail) - a full
    // reconstruction, deferred (a >512 B EH/jump-table body, not a reloc-pass edit).
    void AddLogic(
        i32 type,
        i32 a2,
        i32 a3,
        i32 a4,
        i32 a5,
        CTrigParam p1,
        CTrigParam p2,
        CTrigParam p3,
        CTrigParam p4,
        CTrigParam p5,
        CTrigParam p6,
        i32 a6,
        i32 a7,
        i32 a8,
        i32 a9
    );

    // 0x1163b0: forward with six default (zeroed) parameter blocks.
    void AddLogicDefaults(i32 type, i32 a2, i32 a3, i32 a4, i32 a5, i32 a6, i32 a7, i32 a8, i32 a9);

    // 0x1164a0: forward with the five ids + six CTrigParam blocks pulled from a source
    // tile record (rec) instead of zeroed.
    void AddLogicFromRecord(i32 type, i32 a2, CTrigSourceRecord* rec);
};
SIZE_UNKNOWN(CTileTriggerWiring);

#endif // GRUNTZ_TILETRIGGERWIRING_H
