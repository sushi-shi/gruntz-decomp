#include <rva.h>

#include <Gruntz/DoNothing.h>

#include <Gruntz/DoNothingNormal.h>
#include <Gruntz/LogicTypeId.h>
#include <Gruntz/SerialArchive.h>
#include <Image/CImage.h>
#include <Ints.h>

VTBL(CDoNothingNormal, 0x001e859c);
VTBL(CDoNothing, 0x001e85f4);
RVA(0x0000f6d0, 0x47)
i32 CDoNothing::SerializeMove(CFileMemBase* ar, i32 tag, i32 c, CGameObject* d) {
    if (!CUserLogic::SerializeMove(ar, tag, c, d)) {
        return 0;
    }
    return Chain(ar, tag, c, d) != 0;
}

RVA_COMPGEN(0x0000f740, 0x1e, ??_GCDoNothing@@UAEPAXI@Z)
RVA_COMPGEN(0x0000f770, 0x44, ??1CDoNothing@@UAE@XZ)

RVA(0x0000f800, 0x47)
i32 CDoNothingNormal::SerializeMove(CFileMemBase* ar, i32 tag, i32 c, CGameObject* d) {
    if (!CUserLogic::SerializeMove(ar, tag, c, d)) {
        return 0;
    }
    return Chain(ar, tag, c, d) != 0;
}

RVA_COMPGEN(0x0000f870, 0x1e, ??_GCDoNothingNormal@@UAEPAXI@Z)
RVA_COMPGEN(0x0000f8a0, 0x44, ??1CDoNothingNormal@@UAE@XZ)

CDoNothingNormal* RealizeCDoNothingNormal();
CDoNothingNormal* RealizeCDoNothingNormal() {
    return new CDoNothingNormal();
}

// @early-stop
RVA(0x000ac1d0, 0x1a5)
CDoNothing::CDoNothing(CGameObject* obj) : CUserLogic(obj), CWapX(obj) {
    m_wwdObject->m_flags |= 1;
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
