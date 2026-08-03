#include <rva.h>

#include <Gruntz/EyeCandy.h>

#include <Gruntz/LogicTypeId.h>
#include <Gruntz/SerialArchive.h>
#include <Image/CImage.h>

RVA(0x0000fcc0, 0x47)
i32 CEyeCandy::SerializeMove(CFileMemBase* ar, SerialMode tag, LogicTypeId c, CGameObject* d) {
    if (!CUserLogic::SerializeMove(ar, tag, c, d)) {
        return 0;
    }
    return Chain(ar, tag, c, d) != 0;
}

RVA_COMPGEN(0x0000fd30, 0x1e, ??_GCEyeCandy@@UAEPAXI@Z)
RVA_COMPGEN(0x0000fd60, 0x44, ??1CEyeCandy@@UAE@XZ)

RVA(0x000ac620, 0x1cf)
CEyeCandy::CEyeCandy(CGameObject* obj) : CUserLogic(obj), CWapX(obj) {
    CWwdGameObjectA* o = m_object;
    if (o->m_sortKey == 0 && o->m_layer != 0) {
        i32 v = o->m_layer->m_anchorY + o->m_screenY + 0x186a0;
        if (o->m_sortKey != v) {
            o->m_sortKey = v;
            o->m_flags |= 0x20000;
        }
    }
    CImage* aux = m_object->m_layer;
    if (aux != 0) {
        if (aux->m_width >= g_buteMgr.GetInt("World", "BigActHeight")
            || m_object->m_layer->m_height >= g_buteMgr.GetInt("World", "BigActHeight")) {
            if (m_object->m_animWorker != 0) {
                m_object->m_animWorker->m_flags &= ~6;
                m_object->m_animWorker->m_flags |= 1;
                m_wwdObject->m_flags &= ~0x1000002;
                m_wwdObject->m_flags |= 0x800000;
            }
        }
    }
}

VTBL(CEyeCandy, 0x001e843c);
