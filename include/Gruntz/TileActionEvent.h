#ifndef GRUNTZ_TILEACTIONEVENT_H
#define GRUNTZ_TILEACTIONEVENT_H

#include <rva.h>

#include <Gruntz/SerialArchive.h> // the shared CSerialArchive stream (Read @+0x2c / Write @+0x30)

class CTileTriggerContainer; // owner container (back-stamped into m_14)

class CTileActionEvent {
public:
    // The constructor (0x112d80): zero the m_10 live flag. Was misread as a
    // "ResetFlag" method - its ONLY retail callers are the three new-sites
    // (AddToList3 / AddToList3Switch / Serialize op-7 in TileTriggerContainer.cpp),
    // each with the compiler's `alloc ? ctor(alloc) : 0` guarded-ctor shape, and it
    // returns this in eax exactly as a __thiscall ctor does.
    CTileActionEvent(); // 0x112d80
    // The inline dtor the container walkers inline before RezFree: clear the live
    // flag (no vtable, so no vptr stamp - retail's "clear +0x10, no stamp" delete).
    ~CTileActionEvent() {
        m_10 = 0;
    }

    // Set m_actionCode from the action code, then fold a duplicate-action lookup
    // against the per-player active flags / the level grid, returning 0 if the
    // action is a no-op duplicate else 1. 0x112da0.
    i32 SetActionCode(i32 code);

    // Run the action: translate (m_actionCode, arg) into the effective event code, spawn the
    // tile/brick effect, mark the per-player flags and re-dispatch. 0x112ee0.
    i32 Process(i32 arg);

    // Apply a tool/key (toolId 0x22..0x26) to the current action code: advance
    // m_actionCode to the next code per a per-tool transition table, reset the
    // 4-slot per-player flag array (m_playerFlags), mark this player's slot (or all
    // four when slot==5), then re-commit via SetActionCode. Returns 0 if the (tool,
    // m_actionCode) pair has no transition. 0x113420.
    i32 MorphByTool(i32 toolId, i32 playerSlot);

    // The serialize entry: __thiscall, 4 stack args (ar, mode, ...). Dispatches to
    // SerializeFields on mode 4 (write) / DeserializeFields on mode 7 (read).
    // 0x113f10.
    i32 Serialize(void* ar, i32 mode, i32 a3, i32 a4);

    // The mode-7 read-side counterpart of SerializeFields. External (its body is a
    // sibling function at 0x114040) -> modeled no-body so the call reloc-masks.
    i32 DeserializeFields(void* ar);

    // Stream the 9 record fields (m_actionCode..m_playerFlags[3], skipping m_14)
    // through ar's archive vtable slot +0x30. __thiscall (this = the record, ar =
    // the archive). 0x113f60.
    i32 SerializeFields(void* ar);

    i32 m_actionCode; // +0x00  action-type code (0x12d..0x149)
    i32 m_tileX;      // +0x04  tile X
    i32 m_tileY;      // +0x08  tile Y
    i32 m_c;          // +0x0c  cell key ((x<<8)|y; CTileTriggerContainer::FindByField0C match)
    i32 m_10;         // +0x10  live flag (ctor zeroes; AddToList3 sets 1; dtor clears)
    // +0x14  the owning CTileTriggerContainer (AddToList3/Serialize back-stamp it;
    // NOT serialized - which is exactly why the field skips the stream).
    CTileTriggerContainer* m_14;
    i32 m_playerFlags[4]; // +0x18..+0x24  per-player seen/active flags [0..3]
};
SIZE(CTileActionEvent, 0x28);

#endif // GRUNTZ_TILEACTIONEVENT_H
