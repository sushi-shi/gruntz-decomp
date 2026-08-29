#include <rva.h>

#include <Mfc.h>

#include <Bute/ButeTree.h>
#include <Bute/PTreeNode.h>
#include <Gruntz/SerialArchive.h>
#include <Gruntz/TypeCollRuntime.h>
#include <Gruntz/TypeKeyColl.h>
#include <Gruntz/UserLogic.h>
#include <Io/FileMem.h>
#include <Wap32/ZVec.h>
#include <Wwd/WwdGameObjectFamily.h>

#include <strstrea.h>

inline CTypeCollRuntime::CTypeCollRuntime()

    : _zdvec(sizeof(CString), 0x7d0, 0x7da, ZVecNoScratch()) {
    CString* slot = Slots();
    if (slot != NULL) {
        i32 cnt = m_grown;
        while (cnt-- != 0) {
            if (slot != NULL) {
                slot->CString::CString();
            }
            ++slot;
        }
    }
}

inline CTypeCollRuntime::~CTypeCollRuntime() {
    CString* item = Elem(m_lo);
    if (item != NULL) {
        i32 count = m_hi - m_lo + 1;
        while (count-- != 0) {
            item->CString::~CString();
            ++item;
        }
    }
}

RVA_DYNINIT(0x0016e720, 0xa, g_typeColl)
RVA_DYNINIT(0x0016e730, 0x51, g_typeColl)
RVA_DYNINIT(0x0016e790, 0xe, g_typeColl)
RVA_DYNINIT(0x0016e7a0, 0x48, g_typeColl)
DATA(0x002bf650)
CTypeCollRuntime g_typeColl;

RVA_DYNINIT(0x0016e690, 0xa, g_buteTree)
RVA_DYNINIT(0x0016e6a0, 0x26, g_buteTree)
RVA_DYNINIT(0x0016e6d0, 0xe, g_buteTree)
RVA_DYNINIT(0x0016e6e0, 0x3e, g_buteTree)
DATA(0x002bf620)
CButeTree g_buteTree(&ButeTreeNopFree, zPtrColl::PASSIVE);

RVA(0x0016e7f0, 0x1cf)
i32 CUserLogic::SerializeDispatch(
    CFileMemBase* ar,
    SerialMode mode,
    LogicTypeId typeId,
    CGameObject* object
) {
    if (ar == NULL) {
        return 0;
    }
    switch (mode) {
        case SERIAL_LOAD: {

            i32 len;
            ar->Read(&len, sizeof(len));
            char* buf = new char[len];
            ar->Read(buf, len);
            istrstream accum(buf, len);
            accum >> m_actBits;
            delete[] buf;
            ar->Read(&m_gatedCallbackCode, sizeof(m_gatedCallbackCode));
            ar->Read(&m_reserved2c, sizeof(m_reserved2c));
            ar->Read(&g_logicTypesRegistered, sizeof(g_logicTypesRegistered));
            ar->Read(&m_previousAnimationActId, sizeof(m_previousAnimationActId));
            m_logicObject = object;
            m_object = static_cast<CWwdSpriteObject*>(object);
            m_logicRecord = object->m_logicRecord;
            m_deferredCallback = NULL;
            m_gatedCallback = NULL;
            m_gatedCallbackCode = IDX(ACT_NONE);

            break;
        }
        case SERIAL_SAVE: {

            char buf[0x100];
            ostrstream accum(buf, 0x100);
            accum << m_actBits;
            i32 len = accum.pcount();
            ar->Write(&len, sizeof(len));
            ar->Write(accum.str(), len);
            ar->Write(&m_gatedCallbackCode, sizeof(m_gatedCallbackCode));
            ar->Write(&m_reserved2c, sizeof(m_reserved2c));
            ar->Write(&g_logicTypesRegistered, sizeof(g_logicTypesRegistered));
            ar->Write(&m_previousAnimationActId, sizeof(m_previousAnimationActId));

            break;
        }
    }
    return 1;
}

RVA_COMPGEN(0x0016e9c0, 0x45, ??_GCButeTree@@UAEPAXI@Z)
RVA(0x0016ea10, 0x1)
void ButeTreeNopFree(void*) {}

RVA_COMPGEN(0x0016ea20, 0x51, ??_GCTypeCollRuntime@@UAEPAXI@Z)
