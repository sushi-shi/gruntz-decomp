#ifndef SRC_NET_INTERFACEOBJECT_H
#define SRC_NET_INTERFACEOBJECT_H

#include <Ints.h>
#include <Mfc.h>
#include <Wap32/Object.h>
#include <rva.h>

struct InterfaceObject : public CObject {
    GUID* m_guid;
    CString m_name;
    __POSITION* m_listPosition;

    InterfaceObject() {
        m_guid = 0;
        m_listPosition = 0;
    }
    virtual ~InterfaceObject() OVERRIDE;
    CString GetName();

    i32 IsInterface1();
    i32 IsInterface2();
    i32 IsInterface3();
    i32 IsInterface4();
    i32 IsInterface5();
};
SIZE(0x10);

#endif // SRC_NET_INTERFACEOBJECT_H
