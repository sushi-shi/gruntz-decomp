#include <rva.h>

#include <Net/InterfaceObject.h>

#include <stddef.h>

VTBL(InterfaceObject, 0x001f0748);

RVA(0x00179300, 0x20)
CString InterfaceObject::GetName() {
    return m_name;
}

RVA_COMPGEN(0x00179320, 0x1e, ??_GInterfaceObject@@UAEPAXI@Z)
RVA(0x00179340, 0x48)
InterfaceObject::~InterfaceObject() {
    m_guid = NULL;
    m_listPosition = NULL;
}
