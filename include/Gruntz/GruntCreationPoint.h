#ifndef GRUNTZ_CGRUNTCREATIONPOINT_H
#define GRUNTZ_CGRUNTCREATIONPOINT_H

#include <rva.h>

#include <Gruntz/ActReg.h>
#include <Gruntz/LogicTypeId.h>
#include <Gruntz/SerialArchive.h>
#include <Gruntz/UserLogic.h>

class CGruntCreationPoint : public CUserLogic, public CWapX {
public:
    RVA(0x0003e7a0, 0xd7)
    virtual i32 SerializeMove(CFileMemBase* ar, SerialMode tag, LogicTypeId c, CGameObject* d)
        OVERRIDE {
        if (!CUserLogic::SerializeMove(ar, tag, c, d)) {
            return 0;
        }
        if (!Chain(ar, tag, c, d)) {
            return 0;
        }
        if (tag != SERIAL_SAVE && tag == SERIAL_POSTLOAD) {
            i32 idx;
            if (g_gameReg->m_gameMode != 1) {
                if (g_gameReg->m_options[m_object->m_smarts].m_liveGate != 0) {
                    idx = g_gameReg->m_options[m_object->m_smarts].m_colorIndex;
                } else {
                    idx = ChannelSlots_FindFree();
                }
            } else {
                idx = m_object->m_smarts;
            }
            CShadeTable* sel = g_gameReg->m_spriteFactory->GetSel(idx, 0);
            if (sel == 0) {
                sel = g_gameReg->m_spriteFactory->GetSel(1, 0);
            }
            CWwdGameObjectA* obj = m_object;
            obj->m_drawActive = 1;
            obj->m_drawFillCmd = SHADE_PAL_16;
            obj->m_drawFillArg = sel;
        }
        return 1;
    }
    RVA(0x000106e0, 0x6)
    virtual LogicTypeId GetTypeTag() OVERRIDE {
        return LOGIC_GRUNTCREATIONPOINT;
    }

public:
    CGruntCreationPoint() {}
    CGruntCreationPoint(CGameObject* obj);

    static void RegisterActs();

    virtual void FireActivation(i32 id) OVERRIDE;
    i32 AdvanceAnim();
};
SIZE(0x54);

#endif // GRUNTZ_CGRUNTCREATIONPOINT_H
