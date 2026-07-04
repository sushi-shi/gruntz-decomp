// InterfaceObject.cpp - the DirectPlay group-list node (C:\Proj\NetMgr), the
// 0x10-byte node AddGroupNode (0x178360) operator-new's onto CNetMgr's +0x1c list
// (final vtbl 0x5f0748, base CObject dtor-vtbl 0x5e8cb4, a CString name at +0x8).
// The trace placeholder is "InterfaceObject". Two methods are reconstructed here:
//   ~InterfaceObject (0x179340) - the /GX node destructor.
//   GetName         (0x179300) - return the +0x8 name CString by value.
// Field names are placeholders; only offsets + code bytes are load-bearing.
#include <Net/InterfaceObject.h> // the ONE canonical InterfaceObject class (layout +
                                 // GetName/~dtor/IsInterface1-5); shared with NetMgr.cpp

// The base is the shared engine grand-base Wap::CObject (RTTI "CObject", 5-slot
// interface, grand-base dtor vtable @0x5e8cb4). vtable_hierarchy confirms this node's
// slots 0/2/3/4 are the inherited CObject ILT thunks (0x1bef01/0x0028ec/0x00106e/
// 0x004034) and slot 1 the destructor override - so InterfaceObject adds no new
// virtuals. Real polymorphic: cl folds the empty ~CObject grand-base re-stamp
// (reloc-masks 0x5e8cb4) LAST into the leaf dtor, after the CString member teardown.
// The factory (AddGroupNode, NetMgr.cpp) uses CNetGroupNode - a deliberately separate
// ctor-side model of this same node (see InterfaceObject.h).

// Own (most-derived) vtable @0x5f0748: cl auto-emits ??_7InterfaceObject (the dtor
// below defines the class's key virtual). Retrofit the retail datum name (was the
// anonymous Vtbl_1f0748 in UnknownVTables.h); reloc-masked, matching-neutral. Kept in
// this TU (the one that emits the vtable via the out-of-line dtor); the class layout +
// SIZE live in the shared header.
VTBL(InterfaceObject, 0x005f0748);

// InterfaceObject::GetName (0x179300) - copy-construct the +0x8 name CString into
// the caller's return slot and hand it back.
RVA(0x00179300, 0x20)
CString InterfaceObject::GetName() {
    return m_name;
}

// InterfaceObject::~InterfaceObject (0x179340) - real polymorphic now: cl emits
// the implicit ??_7InterfaceObject own-vptr stamp in the ENTRY state (stamp-first,
// == retail), zeroes m_4/m_c, then the +0x8 CString member dtor and the empty
// ~Wap::CObject (grand-base re-stamp) fold in last. The CString member's
// non-trivial dtor forces the /GX EH frame.
// (eh-dtor-implicit-vptr-stamp-first.md.)
RVA(0x00179340, 0x48)
InterfaceObject::~InterfaceObject() {
    m_4 = 0;
    m_c = 0;
}
