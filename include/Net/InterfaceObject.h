#ifndef SRC_NET_INTERFACEOBJECT_H
#define SRC_NET_INTERFACEOBJECT_H

#include <rva.h>

#include <Mfc.h>

#include <Ints.h>
#include <Wap32/Object.h>

struct InterfaceObject : public CObject {
    GUID* m_guid;
    CString m_name;
    __POSITION* m_listPosition;

    InterfaceObject() {
        m_guid = NULL;
        m_listPosition = NULL;
    }
    virtual ~InterfaceObject() OVERRIDE;
    CString GetName();

    i32 IsIpxProvider();
    i32 IsTcpIpProvider();
    i32 IsModemProvider();
    i32 IsSerialProvider();
    i32 MatchesUnclassifiedProvider();
};
SIZE(0x10);

#endif // SRC_NET_INTERFACEOBJECT_H
