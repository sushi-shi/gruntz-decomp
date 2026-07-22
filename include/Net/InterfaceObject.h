#ifndef SRC_NET_INTERFACEOBJECT_H
#define SRC_NET_INTERFACEOBJECT_H

#include <Ints.h>
#include <Mfc.h>          // real MFC CString (the +0x8 name member)
#include <Wap32/Object.h> // CObject - the shared engine grand-base (vtbl 0x5e8cb4)
#include <rva.h>

struct InterfaceObject : public CObject {
    GUID* m_guid;      // +0x04  the service-provider GUID (LPGUID, stored raw)
    CString m_name; // +0x08  the provider name
    __POSITION* m_listPosition; // +0x0c  cached AddTail position

    // Inline ctor: base CObject vptr stamp + m_name CString ctor + own vptr stamp,
    // then zero the GUID / cached position. Inlined into CNetMgr::AddGroupNode's
    // `new InterfaceObject()` (NetMgr.cpp) - the only construction site.
    InterfaceObject() {
        m_guid = 0;
        m_listPosition = 0;
    }
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
SIZE(0x10); // vptr@0 + m_4 + m_name CString + m_c

#endif // SRC_NET_INTERFACEOBJECT_H
