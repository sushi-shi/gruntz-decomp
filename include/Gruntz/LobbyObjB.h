#ifndef GRUNTZ_GRUNTZ_CLOBBYOBJB_H
#define GRUNTZ_GRUNTZ_CLOBBYOBJB_H

// CLobbyObjB.h - the CMulti m_520 lobby object and its embedded 4x0x64 slot table.
//
// @identity-TODO / FAKE VIEWS (do not trust the class names here): these are NOT their
// own classes - they are placeholder views of the real net/session classes that live
// (as local structs) in src/Net/NetCmdSlot.cpp:
//   CLobbyObjB      == CLobbySync   (same +0x10 field + +0x20 4x0x64 slot table;
//                                    ~@0xb6220, Body_bf000 == CLobbySync::Reset @0xbf000)
//   CLobbySlot      == the 0x64-byte slot class (CLobbyChannel/CNetCmdSlot - the SAME
//                      element Multi.cpp types as `CNetCmdSlot* m_session->m_slots`;
//                      Body_c0bb0 == CNetCmdSlot::ResetAll @0xc0bb0)
//   CLobbySlotInner == MFC ::CInternetSession (the +0x20 EH member; ~@0x1b48c6 stamps
//                      ??_7CObject@@6B@)
//   CLobbySlotMgr   == the slot's +0xc inner manager
// FULL DISSOLUTION IS BLOCKED (downstream fold): the real CLobbySync / 0x64-slot classes
// are the DEFERRED cross-view CONFLATION that NetCmdSlot.cpp already flags - that 0x64
// slot wears THREE local views there (CCluster0c/CLobbyChannel/CNetSlotAux) and the
// CLobbySync<->CNetSession unification needs field-name re-matching. Dissolving this
// header requires FIRST unifying those into one real slot class in a shared <Net/> header,
// then retyping CMulti's +0x520/+0x320 dtors + m_session onto it. Until then these views
// stay (their Body_bf000/Body_c0bb0/~CLobbySlotInner calls reloc-mask, so unbound).
// Field names are placeholders; only the OFFSETS + emitted bytes are load-bearing.

#include <rva.h>
#include <Mfc.h> // CString (BuildHostName fills one)

// The +0xc inner manager of a slot; BuildHostName delegates the host-name copy to
// its 0x1f450 getter (out-of-line -> reloc-masked).
class CLobbySlotMgr {
public:
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
