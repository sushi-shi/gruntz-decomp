// GruntDataRecord.h - a fixed-size (0x68 B) game-data record serialized through a
// binary writer in the big grunt-data Serialize (FUN_00453f90 @0x53f90, in the
// LoadVehicleGruntSprites/LoadGruntAbilityTuning cluster). The enclosing Serialize
// walks an array of these records (stride 0x68) and calls SerializeStrings on each.
//
// The record opens with five owned name strings (char*[5] at +0x00..+0x10) that
// SerializeStrings writes out as fixed 0x80-byte zero-padded fields, followed by
// three 0x10-byte blocks (+0x14/+0x24/+0x34) and one 0x20-byte block (+0x48).
// Only the OFFSETS + the emitted code bytes are load-bearing (campaign doctrine);
// the record's real class name is unrecovered (the enclosing Serialize is unnamed,
// no RTTI), so the fields keep placeholder names.
#ifndef SRC_GRUNTZ_GRUNTDATARECORD_H
#define SRC_GRUNTZ_GRUNTDATARECORD_H

#include <Ints.h>

// The binary writer the record serializes through: an MFC-CArchive-like object
// whose virtual Write(const void* buf, u32 len) sits at vtable slot 12 (+0x30).
// Modeled polymorphic so `ar->Write(buf,len)` emits the retail __thiscall virtual
// dispatch `mov edx,[ar]; mov ecx,ar; call [edx+0x30]` (writer in ecx, two args
// pushed, callee cleanup). The 12 preceding slots are placeholder virtuals; their
// bodies live in NAFXCW / elsewhere so the call reloc-masks.
struct DataWriter {
    virtual void Slot0();
    virtual void Slot1();
    virtual void Slot2();
    virtual void Slot3();
    virtual void Slot4();
    virtual void Slot5();
    virtual void Slot6();
    virtual void Slot7();
    virtual void Slot8();
    virtual void Slot9();
    virtual void Slot10();
    virtual void Slot11();
    virtual void Write(const void* buf, u32 len); // +0x30  slot 12
};

// The 0x68-byte record. SerializeStrings is the only matched method.
struct GruntDataRecord {
    char* m_str[5]; // +0x00..+0x10  five owned name strings
    char m_14[0x10]; // +0x14
    char m_24[0x10]; // +0x24
    char m_34[0x10]; // +0x34
    char m_44[0x4];  // +0x44
    char m_48[0x20]; // +0x48..+0x67 (record stride 0x68)

    // Write the five names (as fixed 0x80 fields) + the four fixed blocks through
    // `ar`; returns 0 if `ar` is null, else 1. (0x56da0, __thiscall, 1 stdcall arg.)
    i32 SerializeStrings(DataWriter* ar);
};

#endif // SRC_GRUNTZ_GRUNTDATARECORD_H
