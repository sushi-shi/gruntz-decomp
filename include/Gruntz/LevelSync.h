// LevelSync.h - CLevelSync (C:\Proj\Gruntz), the level serialize/sync walk, plus the
// referent shapes its one method drives.
//
// These were .cpp-local views inside the merged SBI_RectOnly.cpp. When CLevelSync::Sync
// was un-merged back to its own TU (LevelSync.cpp, 2026-07-13) the views came with it -
// into a header, not into the new .cpp: a type defined in a main-tree .cpp is a fake
// per-TU view, and homing a function out of a merged TU is exactly the point where that
// debt would otherwise be created.
#ifndef GRUNTZ_LEVELSYNC_H
#define GRUNTZ_LEVELSYNC_H

#include <Ints.h>
#include <rva.h>
#include <Gruntz/SerialArchive.h>

// The lazily-allocated CLevelSync +0x54c child + the vtable-slot-1 sub-object shape
// (ex LevelSync.cpp).
// An owned serializable sub-object: vtable slot 1 (+0x4) is its Serialize.
// Slot 0 is the virtual dtor: BOTH identity candidates for the objects the sync
// loop stores in these slots put their dtor at slot 0 - CGruntzCommand (whose
// slot 1 is the byte-identical `Serialize(CSerialArchive*, i32, i32, i32)`
// signature) and the CUserLogic leaf family - so the dtor naming holds whichever
// way the identity chase resolves. @identity-TODO: the slot-1 signature match
// makes CGruntzCommand the prime candidate (the m[] arrays would be the per-team
// queued-command slots); prove via the storing sites' ctor stamps before folding.
struct SyncSub {
    virtual ~SyncSub() {} // slot 0
    virtual i32 Serialize(CSerialArchive* s, i32 op, i32 p4, i32 p5) = 0; // slot 1 / +0x4
};

// The lazily-allocated +0x54c child (operator new(0x40) + ctor 0x401271).
class CLevelSync; // owner (defined below); m_3c back-links to it
struct CLevelSyncChild {
    char pad[0x3c];
    CLevelSync* m_3c; // +0x3c back-link to the owner (= `this` in Sync)
    CLevelSyncChild();
};

class CLevelSync {
public:
    i32 Sync(CSerialArchive* s, i32 op, i32 p4, i32 p5);

    // Reloc-masked engine helpers (this-methods unless noted):
    i32 PreWriteValidate(CSerialArchive* s);                  // 0x4016b8
    i32 PreReadValidate(CSerialArchive* s);                   // 0x402b53
    void SubResetA();                                         // 0x402b8a
    void SubResetB();                                         // 0x402d5b
    i32 ChildSync(CSerialArchive* s, i32 op, i32 p4, i32 p5); // 0x402306 (child __thiscall)
    void PostBlockFixup();                                    // 0x403a08
    void Finalize();                                          // 0x40125d

    i32 m[0x160];
};
SIZE_UNKNOWN(CLevelSync);
SIZE_UNKNOWN(CLevelSyncChild);
SIZE_UNKNOWN(SyncSub);

#endif // GRUNTZ_LEVELSYNC_H
