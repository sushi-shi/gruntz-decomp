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

// The two container globals the whole Bute/act layer runs on.  They live in their
// own compiland because retail's `$E` dynamic-init helpers for them carry the
// constructor and destructor bodies INLINE (0x16e6a0 / 0x16e6e0 / 0x16e730 /
// 0x16e7a0), and MSVC 5 only inlines a ctor/dtor into a `$E` helper when the TU is
// compiled WITHOUT /GX - every function in TypeKeyColl.cpp's own compiland carries
// an EH frame, so that one is /GX and cannot be the source of these four bodies.
// See docs/patterns/gx-blocks-ctor-inlining-into-e-helper.md.

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

DATA(0x002bf650)
CTypeCollRuntime g_typeColl;

DATA(0x002bf620)
CButeTree g_buteTree(&ButeTreeNopFree, 0);

RVA(0x0016e7f0, 0x1cf)
i32 CUserLogic::SerializeMove(
    CFileMemBase* arc,
    SerialMode mode,
    LogicTypeId typeId,
    CGameObject* pObj
) {
    if (arc == NULL) {
        return 0;
    }
    switch (mode) {
        case SERIAL_LOAD: {

            i32 len;
            arc->Read(&len, sizeof(len));
            char* buf = new char[len];
            arc->Read(buf, len);
            istrstream accum(buf, len);
            accum >> m_actBits;
            delete[] buf;
            arc->Read(&m_gatedActKey, sizeof(m_gatedActKey));
            arc->Read(&m_reserved2c, sizeof(m_reserved2c));
            arc->Read(&g_logicTypesRegistered, sizeof(g_logicTypesRegistered));
            arc->Read(&m_prevAnimSetNode, sizeof(m_prevAnimSetNode));
            m_logicObject = pObj;
            m_object = static_cast<CWwdGameObjectA*>(pObj);
            m_objAux = (pObj)->m_animWorker;
            m_deferredCallback = 0;
            m_gatedCallback = 0;
            m_gatedActKey = IDX(ACT_NONE);

            break;
        }
        case SERIAL_SAVE: {

            char buf[0x100];
            ostrstream accum(buf, 0x100);
            accum << m_actBits;
            i32 len = accum.pcount();
            arc->Write(&len, sizeof(len));
            arc->Write(accum.str(), len);
            arc->Write(&m_gatedActKey, sizeof(m_gatedActKey));
            arc->Write(&m_reserved2c, sizeof(m_reserved2c));
            arc->Write(&g_logicTypesRegistered, sizeof(g_logicTypesRegistered));
            arc->Write(&m_prevAnimSetNode, sizeof(m_prevAnimSetNode));

            break;
        }
    }
    return 1;
}

RVA_COMPGEN(0x0016e9c0, 0x45, ??_GCButeTree@@UAEPAXI@Z)
RVA(0x0016ea10, 0x1)
void ButeTreeNopFree(void*) {}

RVA_COMPGEN(0x0016ea20, 0x51, ??_GCTypeCollRuntime@@UAEPAXI@Z)
