// TileTriggerWiring.h - the by-value parameter blocks the tile-trigger container's
// AddLogic factory (0x116610) and its two forwarders (AddLogicDefaults 0x1163b0 /
// AddLogicFromRecord 0x1164a0, both CTileTriggerContainer methods) marshal into the
// per-id logic leaf. AddLogicDefaults passes six zeroed blocks; AddLogicFromRecord
// pulls them from a source tile record. (The old CTileTriggerWiring "class" here was
// a view of the container - the three AddLogic* methods operate on the container's
// own m_list1/m_list2/m_70, so they are CTileTriggerContainer methods; the class was
// dissolved onto CTileTriggerContainer.)
//
// Only the offsets / call shape are load-bearing; field names are placeholders.
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

#endif // GRUNTZ_TILETRIGGERWIRING_H
