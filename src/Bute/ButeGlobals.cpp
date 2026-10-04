#include <StdAfx.h>

#include <Ints.h>

#include <Gruntz/ActRegistry.h>
#include <Gruntz/SerialArchive.h>
#include <Gruntz/TypeKeyColl.h>
#include <Gruntz/UserLogic.h>
#include <Io/FileMem.h>
#include <Wwd/WwdGameObjectFamily.h>
#include <ZTools/PTree.h>
#include <ZTools/ZDArray.h>

#include <strstrea.h>

std::map<i32, std::string> g_typeColl;

zSymTab<i32> g_buteTree(zPtrColl::PASSIVE);

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
