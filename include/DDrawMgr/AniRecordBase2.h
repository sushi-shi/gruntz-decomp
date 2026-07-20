#ifndef GRUNTZ_DDRAWMGR_ANIRECORDBASE2_H
#define GRUNTZ_DDRAWMGR_ANIRECORDBASE2_H

#include <Ints.h>
#include <Wap32/Object.h> // CObject - the shared engine grand-base
#include <rva.h>

struct CAniRecordBase2 : public CObject {
    i32 m_04, m_08, m_0c; // +0x04..0x0f CObject-header fields (base-2 dtor resets them)
    i32 m_10;             // +0x10  owned work buffer (slot-7 FreeBuf frees it)

    CAniRecordBase2() {}

    // The map-worker ctor (inline): the 5 CDDrawWorkerMapSmall factory sites all build a
    // CAniRecordBase2 with the SAME 4-field seed (field04 = parent->+0x1c, field0c =
    // parent->m_0c, m_08/m_10 = 0). Modeled as a real ctor (not spelled-out stores / a
    // helper call) so cl schedules the vptr store 4th - after m_04/m_08/m_0c, before m_10 -
    // matching retail; see docs/patterns/ctor-vptr-interleave-vs-spelled-out-init.md.
    CAniRecordBase2(i32 field04, i32 field0c) {
        m_04 = field04;
        m_08 = 0;
        m_0c = field0c;
        m_10 = 0;
    }

    virtual ~CAniRecordBase2() OVERRIDE; // [1] 0x165dd0 (AniRecord.cpp); ??_G 0x165db0
    virtual void IsLoaded();        // [5] 0x165d90
    virtual void IsValidImage();         // [6] 0x001c08 (CWapObj-family marker slot)
    virtual void FreeBuf();              // [7] 0x168fb0 (bound as CAniRecordView::FreeBuf)
    virtual void GetClassId();        // [8] 0x165da0
    virtual void AllocBufCreate();       // [9] 0x168f20 (bound as CAniRecordView::AllocBufCreate)
    // Slots 10/11/12: the buffer (de)allocation virtuals; bodies are the
    // CAniRecordView non-virtual leaves bound at these RVAs (declared-only here,
    // reloc-masked). Call shapes: 2/2/3 args, callee-clean, i32-tested results. Named
    // by the CDDrawPtrCollections pool entrypoint each wraps (MakeB/MakeB2/MakeB3).
    virtual i32 AllocBufMakeB(i32 size, i32 flag);         // [10] 0x168ee0 (pool MakeB)
    virtual i32 AllocBufMakeB2(i32 size, i32 flag);        // [11] 0x168ea0 (pool MakeB2)
    virtual i32 AllocBufMakeB3(i32 a, i32 size, i32 flag); // [12] 0x168f60 (pool MakeB3)
    virtual i32 PushPalette();                             // [13] 0x168fd0

    // (The dtor's member teardown - free m_10 through the owner pool - is the
    // CAniRecordView-bound body 0x168fb0; the dtor reaches it directly through
    // the facet cast, so no forwarding shim / shadow decl lives here.)
};
SIZE(CAniRecordBase2, 0x14);       // standalone map-worker allocation size (`new` 0x14)
VTBL(CAniRecordBase2, 0x001f02d8); // ??_7 (14 slots)

#endif // GRUNTZ_DDRAWMGR_ANIRECORDBASE2_H
