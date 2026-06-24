// TileActionEvent.h - the per-tile game-action event record (trace placeholder
// ClassUnknown_108; recovered from the 5-method __thiscall cluster at 0x112d80,
// 0x112da0, 0x112ee0, 0x113f10, 0x113f60).
//
// A small data record describing a game action that fires at a tile: m_0 holds an
// action-type code (the 0x12d..0x149 WAP game-event range), m_4/m_8 the tile X/Y,
// and a 4-element per-player "seen/active" flag word array at +0x18. The record is
// processed (m_0 translated, the tile effect spawned) by Process() (0x112ee0) and
// streamed field-by-field through a CMapArchive-style serializer (0x113f10 /
// 0x113f60), matching the archive vtable convention in MapLogic.h (slot +0x30 =
// write, slot +0x2c = read).
//
// No vtable: none of the 5 methods is referenced from a vftable and the ctor
// stamps none - these are plain __thiscall methods. Field names are placeholders;
// only offsets + code bytes are load-bearing.
#ifndef GRUNTZ_TILEACTIONEVENT_H
#define GRUNTZ_TILEACTIONEVENT_H

#include <rva.h>

// The CArchive-like serializer the record is streamed through. Modeled polymorphic
// (slot decls only, never defined -> no ??_7 emitted) so `ar->Write(buf,n)` lowers
// to `mov eax,[ar]; push n; push buf; mov ecx,ar; call [eax+0x30]`. Slot +0x30 is
// the only one this cluster's serializer drives (mode 4 = write). Mirror of
// MapLogic.h's CMapArchive.
struct CTileActionArchive {
    virtual void Slot00();
    virtual void Slot04();
    virtual void Slot08();
    virtual void Slot0C();
    virtual void Slot10();
    virtual void Slot14();
    virtual void Slot18();
    virtual void Slot1C();
    virtual void Slot20();
    virtual void Slot24();
    virtual void Slot28();
    virtual i32 Read(void* buf, i32 n);  // +0x2c
    virtual i32 Write(void* buf, i32 n); // +0x30
};

class CTileActionEvent {
public:
    // Reset the m_10 flag word to 0 (returns this). 0x112d80.
    CTileActionEvent* ResetFlag();

    // Set m_0 from the action code, then fold a duplicate-action lookup against the
    // per-player active flags / the level grid, returning 0 if the action is a
    // no-op duplicate else 1. 0x112da0.
    i32 SetActionCode(i32 code);

    // Run the action: translate (m_0, arg) into the effective event code, spawn the
    // tile/brick effect, mark the per-player flags and re-dispatch. 0x112ee0.
    i32 Process(i32 arg);

    // The serialize entry: __thiscall, 4 stack args (ar, mode, ...). Dispatches to
    // SerializeFields on mode 4 (write) / DeserializeFields on mode 7 (read).
    // 0x113f10.
    i32 Serialize(void* ar, i32 mode, i32 a3, i32 a4);

    // The mode-7 read-side counterpart of SerializeFields. External (its body is a
    // sibling function at 0x114040) -> modeled no-body so the call reloc-masks.
    i32 DeserializeFields(void* ar);

    // Stream the 9 record fields (m_0..m_24, skipping m_14) through ar's archive
    // vtable slot +0x30. __thiscall (this = the record, ar = the archive). 0x113f60.
    i32 SerializeFields(void* ar);

    i32 m_0;  // +0x00  action-type code (0x12d..0x149)
    i32 m_4;  // +0x04  tile X
    i32 m_8;  // +0x08  tile Y
    i32 m_c;  // +0x0c
    i32 m_10; // +0x10  flag word (ResetFlag zeroes it)
    i32 m_14; // +0x14  (NOT serialized)
    i32 m_18; // +0x18  per-player flag array [0..3] (m_18 = index 0)
    i32 m_1c; // +0x1c
    i32 m_20; // +0x20
    i32 m_24; // +0x24
};

#endif // GRUNTZ_TILEACTIONEVENT_H
