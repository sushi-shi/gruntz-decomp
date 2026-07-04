#ifndef GRUNTZ_GRUNTZ_CLOBBYOBJB_H
#define GRUNTZ_GRUNTZ_CLOBBYOBJB_H

// CLobbyObjB.h - the CMulti m_520 lobby object and its embedded 4x0x64 slot table.
// Mis-attributed to CMulti by the dynamic-this tracer; these are their own classes
// (CMulti only holds m_520 as a pointer). Modeled standalone (not via CMulti.h's
// tiny placeholder decls) so the matched cmulti TU is untouched. Field names are
// placeholders; only the OFFSETS + emitted bytes are load-bearing. The body calls
// and the member teardown are external no-body fns -> their `call rel32` reloc-mask.

#include <rva.h>
#include <Mfc.h> // CString (BuildHostName fills one)

// The +0xc inner manager of a slot; BuildHostName delegates the host-name copy to
// its 0x1f450 getter (out-of-line -> reloc-masked).
class CLobbySlotMgr {
public:
    CString* GetHostName(CString* out); // 0x0001f450
};

// The slot's EH-destructible member at +0x20; its out-of-line dtor is the teardown
// the slot dtor runs at trylevel -1 (reloc-masked).
class CLobbySlotInner {
public:
    ~CLobbySlotInner(); // 0x001b48c6 (out-of-line)
    i32 m_0;
};

// A lobby slot (stride 0x64). ~CLobbySlot runs a body call then tears down the
// +0x20 member; BuildHostName fills `out` with the slot's host name.
class CLobbySlot {
public:
    CString* BuildHostName(CString* out); // 0x000bc3f0
    ~CLobbySlot();                        // 0x000b62a0
    void Body_c0bb0();                    // ~CLobbySlot body call (out-of-line)

    char m_pad00_0c[0xc];
    CLobbySlotMgr* m_mgr; // +0x0c  inner manager
    char m_pad10_20[0x20 - 0x10];
    CLobbySlotInner m_20; // +0x20  EH-destructible member
    char m_pad24_64[0x64 - 0x24];
};

// The CMulti m_520 lobby object. ~CLobbyObjB runs a body call then vector-destroys
// its embedded 4x0x64 slot table at +0x20.
class CLobbyObjB {
public:
    ~CLobbyObjB();     // 0x000b6220
    void Body_bf000(); // ~CLobbyObjB body call (out-of-line)

    char m_pad00_10[0x10];
    i32 m_10; // +0x10
    char m_pad14_20[0x20 - 0x14];
    CLobbySlot m_slots[4]; // +0x20  (4 x 0x64)
};

#endif // GRUNTZ_GRUNTZ_CLOBBYOBJB_H
