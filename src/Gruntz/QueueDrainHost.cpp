#include <rva.h>

#include <DDrawMgr/DDrawChildGroup.h>
#include <Gruntz/UserLogic.h>

#include <stddef.h>

RVA(0x00031250, 0x33)
CGameObject* CDDrawChildGroup::Drain() {
    for (;;) {
        if (m_scanCursor == NULL) {
            return 0;
        }
        CGameObject* data = static_cast<CGameObject*>(m_list.GetNext(m_scanCursor));
        if (data->GetClassId() == CLASSID_SERIALREF) {
            return data;
        }
    }
}
