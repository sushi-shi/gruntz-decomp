#ifndef SRC_NET_INTERFACEOBJECT_H
#define SRC_NET_INTERFACEOBJECT_H

#include <Ints.h>
#include <Mfc.h>          // real MFC CString (the +0x8 name member)
#include <Wap32/Object.h> // Wap::CObject - the shared engine grand-base (vtbl 0x5e8cb4)
#include <rva.h>

// InterfaceObject - the DirectPlay service-provider group-list node (C:\Proj\NetMgr):
// the 0x10-byte payload CNetMgr::AddGroupNode operator-new's onto the +0x1c group
// CObList. Derives the shared engine grand-base Wap::CObject; whole-object vtable
// 0x5f0748 (owned/emitted by InterfaceObject.cpp's VTBL). RTTI-proven (the COL type
// descriptor is `.?AVInterfaceObject@@` per vtable_hierarchy). Its five GUID predicates
// (IsInterface1-5, external __thiscall thunks) classify the provider class
// (IPX/TcpIp/Modem/Serial/...) that CNetMgr::Find / DetectConnectionConfig select.
//
// Declared `struct` (default-public) so CNetMgr::Find's `InterfaceObject*` return
// mangles PAU - matching the retail ?Find@CNetMgr@@QAEPAUInterfaceObject@@H@Z symbol.
//
// NB: AddGroupNode's ctor-side model (CNetGroupNode, local to NetMgr.cpp) is a
// DELIBERATELY separate class with an inline empty dtor - it IS the same retail object
// but keeps new/delete construction inlined into AddGroupNode, whereas this class'
// out-of-line ~InterfaceObject (0x179340) matches the standalone node destructor. A
// required matching split, not a modeling miss.
struct InterfaceObject : public Wap::CObject {
    i32 m_4;        // +0x04  the service-provider GUID (stored raw)
    CString m_name; // +0x08  the provider name
    i32 m_c;        // +0x0c  cached AddTail position

    virtual ~InterfaceObject() OVERRIDE; // slot 1 (own dtor; 0x179340)
    CString GetName();                   // 0x179300

    // The five GUID predicates (external __thiscall thunks; reloc-masked). Each tests
    // the node's stored GUID against one service-provider connection class.
    i32 IsInterface1(); // 0x1794b0
    i32 IsInterface2(); // 0x1794e0
    i32 IsInterface3(); // 0x179510
    i32 IsInterface4(); // 0x179540
    i32 IsInterface5(); // 0x179570
};
SIZE(InterfaceObject, 0x10); // vptr@0 + m_4 + m_name CString + m_c

#endif // SRC_NET_INTERFACEOBJECT_H
