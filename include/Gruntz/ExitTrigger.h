#ifndef GRUNTZ_CEXITTRIGGER_H
#define GRUNTZ_CEXITTRIGGER_H

#include <rva.h>

#include <Gruntz/LogicTypeId.h>
#include <Gruntz/SerialArchive.h>
#include <Gruntz/UserLogic.h>

class CWarlord;

class CExitTrigger : public CUserLogic, public CWapX {
public:
public:
    CExitTrigger() {}
    CExitTrigger(CGameObject* obj);

    RVA(0x00010870, 0x6)
    virtual LogicTypeId GetTypeTag() OVERRIDE {
        return LOGIC_EXITTRIGGER;
    }
    RVA(0x0003f040, 0x147)
    virtual i32
    SerializeMove(CFileMemBase* ar, SerialMode mode, LogicTypeId typeId, CGameObject* pObj)
        OVERRIDE {
        if (!CUserLogic::SerializeMove(ar, mode, typeId, pObj)) {
            return 0;
        }
        CFileMemBase* arc = ar;
        if (!Chain(arc, mode, typeId, pObj)) {
            return 0;
        }

        CDDrawSurfaceMgr* holder = g_gameReg->m_world;
        switch (mode) {
            case SERIAL_LOAD: {
                arc->Read(&m_resolved, 4);
                i32 key = 0;
                arc->Read(&key, 4);
                if (key != 0) {
                    CGameObject* found = 0;

                    CGameObject* obj = 0;

                    if (MapLookupById(holder->m_childGroup->m_map48, key, found)) {
                        obj = found;
                    }
                    m_warlordLogic = static_cast<CWarlord*>(obj->m_animWorker->m_logic);
                    if (m_warlordLogic == 0) {
                        return 0;
                    }
                } else {
                    m_warlordLogic = 0;
                }
                break;
            }
            case SERIAL_SAVE: {
                arc->Write(&m_resolved, 4);
                if (m_warlordLogic == 0) {
                    g_serialCounter++;
                    i32 id = 0;
                    arc->Write(&id, 4);
                } else {
                    g_serialCounter++;
                    i32 id = 0;
                    if (m_warlordLogic->m_object != 0) {
                        id = m_warlordLogic->m_object->m_objectId;
                    }
                    arc->Write(&id, 4);
                }
                break;
            }
        }
        return 1;
    }

    virtual void FireActivation(i32 id) OVERRIDE;
    static void RegisterActs();
    i32 AdvanceAnim();

    CWarlord* m_warlordLogic;
    i32 m_resolved;
};
SIZE(0x5c);

#endif // GRUNTZ_CEXITTRIGGER_H
