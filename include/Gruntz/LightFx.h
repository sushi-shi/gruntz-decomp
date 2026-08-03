#ifndef GRUNTZ_GRUNTZ_CLIGHTFX_H
#define GRUNTZ_GRUNTZ_CLIGHTFX_H

#include <rva.h>

#include <Gruntz/ActReg.h>
#include <Gruntz/LogicTypeId.h>
#include <Gruntz/SerialArchive.h>
#include <Gruntz/UserLogic.h>

class CLightFx : public CUserLogic, public CWapX {
public:
    RVA(0x0009d660, 0xc8)
    virtual i32
    SerializeMove(CFileMemBase* ar, SerialMode mode, LogicTypeId typeId, CGameObject* pObj)
        OVERRIDE {
        if (CUserLogic::SerializeMove(ar, mode, typeId, pObj) == 0) {
            return 0;
        }
        if (Chain(ar, mode, typeId, pObj) == 0) {
            return 0;
        }
        switch (mode) {
            case SERIAL_SAVE:
                (ar)->Write(&m_anchorA, 4);
                (ar)->Write(&m_anchorB, 4);
                break;
            case SERIAL_LOAD:
                (ar)->Read(&m_anchorA, 4);
                (ar)->Read(&m_anchorB, 4);
                break;
            case SERIAL_POSTLOAD:
                g_gameReg
                    ->m_logicPump

                    ->Push(m_wwdObject->m_frameSet, m_anchorA, SHADE_DST_BY_SRC_16);
                break;
        }
        return 1;
    }
    RVA(0x000123e0, 0x6)
    virtual LogicTypeId GetTypeTag() OVERRIDE {
        return LOGIC_LIGHTFX;
    }

public:
    CLightFx() {}
    CLightFx(CGameObject* obj);

    virtual void FireActivation(i32 id) OVERRIDE;

    static void RegisterActs();

    i32 AdvanceAnim();

    i32 Activate(const char* spec, const char* effect, i32 anchorA, i32 anchorB);

    i32 RebindNode();

    i32 m_anchorA;
    i32 m_anchorB;
};
SIZE(0x5c);

#endif // GRUNTZ_GRUNTZ_CLIGHTFX_H
