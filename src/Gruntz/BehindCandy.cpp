#include <rva.h>

#include <Gruntz/BehindCandy.h>

#include <Gruntz/LogicTypeId.h>
#include <Gruntz/SerialArchive.h>
#include <Image/CImage.h>

#include <stddef.h>

RVA(0x0000fb90, 0x47)
i32 CBehindCandy::SerializeMove(CFileMemBase* ar, SerialMode tag, LogicTypeId c, CGameObject* d) {
    if (!CUserLogic::SerializeMove(ar, tag, c, d)) {
        return 0;
    }
    return Chain(ar, tag, c, d) != 0;
}

RVA_COMPGEN(0x0000fc00, 0x1e, ??_GCBehindCandy@@UAEPAXI@Z)
RVA_COMPGEN(0x0000fc30, 0x44, ??1CBehindCandy@@UAE@XZ)

RVA(0x000ac3f0, 0x1b1)
CBehindCandy::CBehindCandy(CGameObject* obj) : CUserLogic(obj), CWapX(obj) {
    if (m_object->m_sortKey != 0) {
        m_object->m_sortKey = 0;
        m_object->m_flags |= 0x20000;
    }
    if (m_object->m_layer != NULL) {
        if (m_object->m_layer->m_width >= g_buteMgr.GetInt("World", "BigActHeight")
            || m_object->m_layer->m_height >= g_buteMgr.GetInt("World", "BigActHeight")) {
            if (m_object->m_animWorker != NULL) {
                m_object->m_animWorker->m_flags &= ~6;
                m_object->m_animWorker->m_flags |= 1;
                m_wwdObject->m_flags &= ~0x1000002;
                m_wwdObject->m_flags |= 0x800000;
            }
        }
    }
}
