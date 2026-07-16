// AniRecordBase2.h - CAniRecordBase2, the 0x14-byte secondary/base facet of the
// 'ANI' frame-record family (14-slot vtable ??_7CAniRecordBase2 @0x1f02d8,
// = 0x1f02c0+0x18; slot-1 dtor pair ??_G 0x165db0 / ??1 0x165dd0 in AniRecord.cpp).
//
// This is ALSO the standalone keyed "map worker" object the CDDrawWorkerMapSmall
// factories allocate (`new` 0x14 bytes, seed m_04..m_10, dispatch slots 10/11/12) -
// the former DDrawWorkerMapSmall.h view pair CDDrawMapWorker (RELOC_VTBL alias) +
// CDDrawMapWorkerObj was this ONE class under two fake names; both are dissolved
// onto this canonical definition (the factory new-sites stamp 0x5f02d8 as the
// object's own primary vtable - one vtable, one class).
//
// HELD (vtable_hierarchy correction): slot 6 = 0x001c08 is the CWapObj-family
// marker, so the true chain is CObject -> CWapObj -> CAniRecordBase2; kept on the
// direct CObject base pending CWapObj modeling (do NOT re-flatten elsewhere).
//
// Field names are placeholders (m_<hexoffset>); only the OFFSETS + emitted code
// bytes are load-bearing (campaign doctrine).
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
    virtual void Slot05_165d90();        // [5] 0x165d90
    virtual void IsValidImage();         // [6] 0x001c08 (CWapObj-family marker slot)
    virtual void Slot07_168fb0();        // [7] 0x168fb0 (FreeBuf, bound as CAniRecordView method)
    virtual void Slot08_165da0();        // [8] 0x165da0
    virtual void Slot09_168f20();        // [9] 0x168f20
    // Slots 10/11/12: the buffer (de)allocation virtuals; bodies are the
    // CAniRecordView non-virtual leaves bound at these RVAs (declared-only here,
    // reloc-masked). Call shapes: 2/2/3 args, callee-clean, i32-tested results.
    virtual i32 Alloc168ee0(i32 size, i32 flag);        // [10] 0x168ee0
    virtual i32 Alloc168ea0(i32 size, i32 flag);        // [11] 0x168ea0
    virtual i32 Alloc168f60(i32 a, i32 size, i32 flag); // [12] 0x168f60
    virtual i32 Slot13_168fd0();                        // [13] 0x168fd0

    // (The dtor's member teardown - free m_10 through the owner pool - is the
    // CAniRecordView-bound body 0x168fb0; the dtor reaches it directly through
    // the facet cast, so no forwarding shim / shadow decl lives here.)
};
SIZE(CAniRecordBase2, 0x14);       // standalone map-worker allocation size (`new` 0x14)
VTBL(CAniRecordBase2, 0x001f02d8); // ??_7 (14 slots)

#endif // GRUNTZ_DDRAWMGR_ANIRECORDBASE2_H
