#include <Mfc.h>
#include <Gruntz/GameRegMfcPtr.h>
#include <Gruntz/GruntzMgr.h>
#include <Gruntz/ActNameRegistry.h>
#include <Wap32/ZVec.h>
#include <Gruntz/ActReg.h>
#include <Gruntz/SingleFrameMessage.h>
#include <Gruntz/WwdGameReg.h>
#include <Gruntz/SerialArchive.h>
#include <rva.h>
#include <rva.h>
#include <Wap32/ZVec.h>
#include <Gruntz/SerialArchive.h>

template<> DATA(0x00245ef0)
CActReg CActRegPool<CSingleFrameMessage>::s_table(2000, 2010);

RVA(0x0000f5a0, 0x47)
i32 CSingleFrameMessage::SerializeMove(CFileMemBase* ar, i32 tag, i32 c, CGameObject* d) {
    if (!CUserLogic::SerializeMove(ar, tag, c, d)) {
        return 0;
    }
    return Chain(ar, tag, c, d) != 0;
}

RVA_COMPGEN(0x0000f610, 0x1e, ??_GCSingleFrameMessage@@UAEPAXI@Z)
RVA_COMPGEN(0x0000f640, 0x44, ??1CSingleFrameMessage@@UAE@XZ)

RVA(0x000ab310, 0x18d)
CSingleFrameMessage::CSingleFrameMessage(CGameObject* obj) : CUserLogic(obj), CWapX(obj) {
    m_prevAnimSetNode = m_objAux->m_actKey;
    m_objAux->m_actKey = ActFindId("A");
    m_object->ApplyLookupSprite("GAME_MESSAGEZ", m_wwdObject->m_id);
    RECT bounds;
    RECT r;
    CopyRect(&r, g_gameReg->GetRect(&bounds));
    m_object->m_screenX = r.left + (r.right - r.left) / 2;
    m_object->m_screenY = r.top + (r.bottom - r.top) / 2;
}

VTBL(CSingleFrameMessage, 0x001e864c);

RVA(0x000ab5b0, 0x102)
void CSingleFrameMessage::FireActivation(i32 id) {
    CActHandler* e = (CActRegPool<CSingleFrameMessage>::s_table.ResolveEntry(id));
    if ((*e) != 0) {
        (this->*(*((CActRegPool<CSingleFrameMessage>::s_table.ResolveEntry(id)))))();
    }
}

RVA(0x000ab710, 0x18d)
void CSingleFrameMessage::RegisterActs() {
    i32 id = ActFindId("A");
    if (id == 0) {
        ActInsertId("A", g_typeCounter);
        id = g_typeCounter;
        CString* slot = ActNameLookup(g_typeCounter);
        i32 n = g_typeColl.m_grown;
        CString* list = ActNameSlots();
        while (n-- != 0) {
            if (list != 0) {
                list->CString::~CString();
            }
            list++;
        }
        *slot = "A";
        g_typeCounter++;
    }
    (*((CActRegPool<CSingleFrameMessage>::s_table.ResolveEntry(id)))) =
        static_cast<i32 (CUserLogic::*)()>(&CSingleFrameMessage::AdvanceAnim);
}

RVA(0x000ab910, 0x12)
i32 CSingleFrameMessage::AdvanceAnim() {
    m_wwdObject->m_flags |= 0x10000;
    return 0;
}
