#include <Net/InterfaceObject.h> // the ONE canonical InterfaceObject class (layout +
#include <rva.h>

VTBL(InterfaceObject, 0x001f0748);

RVA(0x00179300, 0x20)
CString InterfaceObject::GetName() {
    return m_name;
}

RVA(0x00179340, 0x48)
InterfaceObject::~InterfaceObject() {
    m_guid = 0;
    m_listPosition = 0;
}
