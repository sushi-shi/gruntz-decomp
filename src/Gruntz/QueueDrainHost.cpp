#include <rva.h>
#include <DDrawMgr/DDrawChildGroup.h> // the real collection class (ex the CQueueDrainHost view)
#include <Gruntz/UserLogic.h>         // ::CGameObject (the payload; GetClassId is vtable slot 8)

RVA(0x00031250, 0x33)
CGameObject* CDDrawChildGroup::Drain() {
    for (;;) {
        if (m_scanCursor == 0) {
            return 0;
        }
        CGameObject* data = static_cast<CGameObject*>(m_list.GetNext(m_scanCursor));
        if (data->GetClassId() == CLASSID_SERIALREF) {
            return data;
        }
    }
}
