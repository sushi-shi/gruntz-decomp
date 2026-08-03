#ifndef GRUNTZ_CRAINCLOUD_H
#define GRUNTZ_CRAINCLOUD_H

#include <rva.h>

#include <Gruntz/LogicTypeId.h>
#include <Gruntz/PathHazard.h>
#include <Gruntz/SerialArchive.h>

class CRainCloud : public CPathHazard {
public:
    RVA(0x000b4cb0, 0x56)
    virtual i32 SerializeMove(CFileMemBase* stream, SerialMode tag, LogicTypeId c, CGameObject* d)
        OVERRIDE {
        if (!CPathHazard::SerializeMove(stream, tag, c, d)) {
            return 0;
        }
        if (tag == SERIAL_POSTLOAD) {
            CShadeTable* x = g_gameReg->m_logicPump->m_tables[5];
            CWwdGameObjectA* o = m_object;
            o->m_drawActive = 1;
            o->m_drawFillCmd = SHADE_DST_BY_SRC_16;
            o->m_drawFillArg = x;
        }
        return 1;
    }
    RVA(0x000132f0, 0x6)
    virtual LogicTypeId GetTypeTag() OVERRIDE {
        return LOGIC_RAINCLOUD;
    }
    CRainCloud() {}
    CRainCloud(CGameObject* obj);

    virtual i32 Tick() OVERRIDE;
    virtual i32 HitTest(i32, i32) OVERRIDE;
};
SIZE(0x130);

#endif // GRUNTZ_CRAINCLOUD_H
