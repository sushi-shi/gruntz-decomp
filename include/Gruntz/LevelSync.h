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
// slot 1 is the byte-identical `Serialize(CFileMemBase*, i32, i32, i32)`
// signature) and the CUserLogic leaf family - so the dtor naming holds whichever
// way the identity chase resolves. @identity-TODO: the slot-1 signature match
// makes CGruntzCommand the prime candidate (the m[] arrays would be the per-team
// queued-command slots); prove via the storing sites' ctor stamps before folding.
// VTBL_ABSENT: abstract 2-slot base of the sync-slot objects; only the concrete
// class is constructed. @identity-TODO (above): CGruntzCommand is the prime
// candidate - prove via the storing sites' ctor stamps, then fold.
VTBL_ABSENT(SyncSub);
struct SyncSub {
    virtual ~SyncSub() {} // slot 0
    virtual i32 Serialize(CFileMemBase* s, i32 op, i32 p4, i32 p5) = 0; // slot 1 / +0x4
};
SIZE_UNKNOWN();

class CLevelSync; // owner (defined below); m_3c back-links to it
struct CLevelSyncChild {
    char pad[0x3c];
    CLevelSync* m_3c; // +0x3c back-link to the owner (= `this` in Sync)
    CLevelSyncChild();
};
SIZE_UNKNOWN();

class CLevelSync {
public:
    i32 Sync(CFileMemBase* s, i32 op, i32 p4, i32 p5);

    // Reloc-masked engine helpers (this-methods unless noted):
    i32 PreWriteValidate(CFileMemBase* s);                  // 0x4016b8
    i32 PreReadValidate(CFileMemBase* s);                   // 0x402b53
    void SubResetA();                                         // 0x402b8a
    void SubResetB();                                         // 0x402d5b
    i32 ChildSync(CFileMemBase* s, i32 op, i32 p4, i32 p5); // 0x402306 (child __thiscall)
    void PostBlockFixup();                                    // 0x403a08
    void Finalize();                                          // 0x40125d

    i32 m[0x160];
};
SIZE_UNKNOWN();

#endif // GRUNTZ_LEVELSYNC_H
