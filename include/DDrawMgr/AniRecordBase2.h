#ifndef GRUNTZ_DDRAWMGR_ANIRECORDBASE2_H
#define GRUNTZ_DDRAWMGR_ANIRECORDBASE2_H

#include <Ints.h>
#include <Gruntz/Loadable.h> // CLoadable : CWapObj : CObject - the real 9-slot base
#include <rva.h>

class CDDrawSurfaceMgr; // the m_0c owner (the pool + draw-target root)
// struct, NOT class: CDDPalette is DEFINED as a struct (<DDrawMgr/DirectDrawMgr.h>) and
// every other fwd decl agrees. MSVC mangles the class-key of the first declaration the
// TU sees, so a stray `class` here made AniRecord.cpp emit PAVCDDPalette references
// against the PAU definitions - 6 FAKE relocs (assert_relocs), a link break.
struct CDDPalette; // the +0x10 owned work palette

// BASE CORRECTED 2026-07-28: `: CWapObj` -> `: CLoadable`, the same dtor proof that
// re-based AnimWorkerObj. ~CAniRecordBase2 @0x165dd0 ends `mov [esi+4],-1 /
// mov [esi+8],0 / mov [esi+0xc],0 / mov [esi],0x5e8cb4` (0x165e04-0x165e19) - that is
// ~CLoadable's body, in its declaration order, then the grand-base restamp. The slot
// scheme matches too: [5] IsLoaded, [6] inherited IsReady, [7] the family's Unload
// hook, [8] GetClassId, and only [9]..[13] new. m_04/m_08/m_0c ARE m_id/m_flags/
// m_ownerCtx (the factories seed m_id from parent->+0x1c and m_ownerCtx from the
// parent's own owner word).
struct CAniRecordBase2 : public CLoadable {
    CDDPalette* m_buf; // +0x10  the owned work palette (FreeBuf returns it to the pool)

    CAniRecordBase2() {}

    // The map-worker ctor (inline): the 5 CDDrawWorkerMapSmall factory sites all build a
    // CAniRecordBase2 with the SAME 4-field seed (field04 = parent->+0x1c, field0c =
    // parent->m_0c, m_08/m_10 = 0). Modeled as a real ctor (not spelled-out stores / a
    // helper call) so cl schedules the vptr store 4th - after m_04/m_08/m_0c, before m_10 -
    // matching retail; see docs/patterns/ctor-vptr-interleave-vs-spelled-out-init.md.
    CAniRecordBase2(i32 field04, class CDDrawSurfaceMgr* owner) {
        m_id = field04;
        m_flags = 0;
        m_ownerCtx = owner;
        m_buf = 0;
    }

    // All slot bodies live in AniRecord.cpp (this class's own methods - the ex
    // "CAniRecordView-bound" homes were a mis-home; sema xref proves the bodies are
    // referenced ONLY as THIS vtable's slots).
    virtual ~CAniRecordBase2() OVERRIDE; // [1] 0x165dd0; ??_G 0x165db0
    virtual i32 IsLoaded() OVERRIDE;     // [5] 0x165d90 (m_buf != 0; overrides CLoadable)
    // slot 6 IsReady INHERITED from CWapObj (its `return 1` default @0xd5da0, reached
    // via the 0x001c08 thunk); not redeclared (that was a phantom own "IsValidImage").
    // [7] IS the family's Unload slot: release m_buf back into the owner pool. Named
    // FreeBuf before the rebase; the dtor calls it directly (cl devirtualizes).
    virtual void Unload() OVERRIDE;    // [7] 0x168fb0
    virtual i32 GetClassId() OVERRIDE; // [8] 0x165da0  CLASSID 0x15
    // Slots 9-12: the buffer (de)allocation virtuals - each wraps one
    // CDDrawPtrCollections pool entrypoint (Create/MakeB/MakeB2/MakeB3) with the
    // 0x44 palette kind + the optional system-palette capture.
    // Slot 9/10/11/12 arg1 types are NOT a single polymorphic arg - each slot wraps a
    // DIFFERENT pool entrypoint, so each has its own first-arg type: [9] a handle
    // (Create takes an int), [10]/[12] an in-memory palette blob (MakeB's loader
    // @0x1474d0 reads 256 RGB triples out of it), [11] a FILE PATH (MakeB2's loader,
    // CDDPalette::LoadFromFile @0x147410, opens with `strrchr(a1,'.')`).
    virtual i32 AllocBufCreate(i32 handle, i32 flag);           // [9]  0x168f20
    virtual i32 AllocBufMakeB(void* data, i32 flag);            // [10] 0x168ee0
    virtual i32 AllocBufMakeB2(char* path, i32 flag);           // [11] 0x168ea0
    virtual i32 AllocBufMakeB3(void* data, i32 size, i32 flag); // [12] 0x168f60
    virtual i32 PushPalette();                                  // [13] 0x168fd0
};
SIZE(0x14); // standalone map-worker allocation size (`new` 0x14)

#endif // GRUNTZ_DDRAWMGR_ANIRECORDBASE2_H
