#ifndef GRUNTZ_DDRAWMGR_ANIRECORDBASE2_H
#define GRUNTZ_DDRAWMGR_ANIRECORDBASE2_H

#include <Ints.h>
#include <Wap32/WapObj.h> // CWapObj : CObject - slots 5/6 (IsLoaded/IsReady default)
#include <rva.h>

class CDDrawSurfaceMgr; // the m_0c owner (the pool + draw-target root)
// struct, NOT class: CDDPalette is DEFINED as a struct (<DDrawMgr/DirectDrawMgr.h>) and
// every other fwd decl agrees. MSVC mangles the class-key of the first declaration the
// TU sees, so a stray `class` here made AniRecord.cpp emit PAVCDDPalette references
// against the PAU definitions - 6 FAKE relocs (assert_relocs), a link break.
struct CDDPalette; // the +0x10 owned work palette

struct CAniRecordBase2 : public CWapObj {
    i32 m_04, m_08;         // +0x04/+0x08 CObject-header fields (base-2 dtor resets them)
    CDDrawSurfaceMgr* m_0c; // +0x0c  the owning manager (same slot, and now the same
                            //        type, as CLoadable::m_ownerCtx)
    CDDPalette* m_buf;      // +0x10  the owned work palette (FreeBuf returns it to the pool)

    // The owner at its real type: the surface mgr whose m_ptrColl pool the
    // Alloc*/FreeBuf slots drive and whose m_drawTarget PushPalette walks.
    CDDrawSurfaceMgr* OwnerMgr() {
        return m_0c;
    }

    CAniRecordBase2() {}

    // The map-worker ctor (inline): the 5 CDDrawWorkerMapSmall factory sites all build a
    // CAniRecordBase2 with the SAME 4-field seed (field04 = parent->+0x1c, field0c =
    // parent->m_0c, m_08/m_10 = 0). Modeled as a real ctor (not spelled-out stores / a
    // helper call) so cl schedules the vptr store 4th - after m_04/m_08/m_0c, before m_10 -
    // matching retail; see docs/patterns/ctor-vptr-interleave-vs-spelled-out-init.md.
    CAniRecordBase2(i32 field04, class CDDrawSurfaceMgr* field0c) {
        m_04 = field04;
        m_08 = 0;
        m_0c = field0c;
        m_buf = 0;
    }

    // All slot bodies live in AniRecord.cpp (this class's own methods - the ex
    // "CAniRecordView-bound" homes were a mis-home; sema xref proves the bodies are
    // referenced ONLY as THIS vtable's slots).
    virtual ~CAniRecordBase2() OVERRIDE; // [1] 0x165dd0; ??_G 0x165db0
    virtual i32 IsLoaded() OVERRIDE;     // [5] 0x165d90 (m_buf != 0; overrides CWapObj)
    // slot 6 IsReady INHERITED from CWapObj (its `return 1` default @0xd5da0, reached
    // via the 0x001c08 thunk); not redeclared (that was a phantom own "IsValidImage").
    virtual void FreeBuf();   // [7] 0x168fb0  release m_buf into the owner pool
    virtual i32 GetClassId(); // [8] 0x165da0  CLASSID 0x15
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
