// InterfaceObject.cpp - the DirectPlay group-list node (C:\Proj\NetMgr), the
// 0x10-byte node AddGroupNode (0x178360) operator-new's onto CNetMgr's +0x1c list
// (final vtbl 0x5f0748, base CObject dtor-vtbl 0x5e8cb4, a CString name at +0x8).
// The trace placeholder is "InterfaceObject". Two methods are reconstructed here:
//   ~InterfaceObject (0x179340) - the /GX node destructor.
//   GetName         (0x179300) - return the +0x8 name CString by value.
// Field names are placeholders; only offsets + code bytes are load-bearing.
#include <Mfc.h> // real MFC CString (NAFXCW copy-ctor 0x1b9ba3 / dtor 0x1b9cde, reloc-masked)
#include <Ints.h>
#include <Wap32/CObject.h> // Wap::CObject - the shared engine grand-base (vtbl 0x5e8cb4)
#include <rva.h>

// The base is the shared engine grand-base Wap::CObject (RTTI "CObject", 5-slot
// interface, grand-base dtor vtable @0x5e8cb4). vtable_hierarchy confirms this node's
// slots 0/2/3/4 are the inherited CObject ILT thunks (0x1bef01/0x0028ec/0x00106e/
// 0x004034) and slot 1 the destructor override - so InterfaceObject adds no new
// virtuals. Real polymorphic: cl folds the empty ~CObject grand-base re-stamp
// (reloc-masks 0x5e8cb4) LAST into the leaf dtor, after the CString member teardown.
// The factory (AddGroupNode, NetMgr.cpp) is real polymorphic (`new CNetGroupNode`):
// cl auto-stamps the own vtable (0x5f0748) in the ctor, no manual stamp anywhere.

// Own (most-derived) vtable @0x5f0748: cl auto-emits ??_7InterfaceObject (the dtor
// below defines the class's key virtual). Retrofit the retail datum name (was the
// anonymous Vtbl_1f0748 in UnknownVTables.h); reloc-masked, matching-neutral.
VTBL(InterfaceObject, 0x005f0748);
class InterfaceObject : public Wap::CObject {
public:
    i32 m_4;        // +0x04
    CString m_name; // +0x08
    i32 m_c;        // +0x0c
    virtual ~InterfaceObject();
    CString GetName();
};
// Exact size: AddGroupNode (NetMgr.cpp 0x178360) operator-new's a 0x10-byte node
// (vptr@0 + m_4 + m_name CString + m_c).
SIZE(InterfaceObject, 0x10);

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
