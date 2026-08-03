#ifndef GRUNTZ_CFORTRESSFLAG_H
#define GRUNTZ_CFORTRESSFLAG_H

#include <rva.h>

#include <Gruntz/ActReg.h>
#include <Gruntz/LogicTypeId.h>
#include <Gruntz/SerialArchive.h>
#include <Gruntz/UserLogic.h>

class CFortressFlag : public CUserLogic, public CWapX {
public:
public:
    CFortressFlag() {}
    CFortressFlag(CGameObject* obj);

    static void RegisterActs();
    virtual void FireActivation(i32 id) OVERRIDE;
    i32 AdvanceAnim();

    RVA(0x00010e40, 0x6)
    virtual LogicTypeId GetTypeTag() OVERRIDE {
        return LOGIC_FORTRESSFLAG;
    }
    RVA(0x00046410, 0x92)
    virtual i32 SerializeMove(CFileMemBase* ar, SerialMode tag, LogicTypeId c, CGameObject* d)
        OVERRIDE {
        if (!CUserLogic::SerializeMove(ar, tag, c, d)) {
            return 0;
        }
        if (!Chain(ar, tag, c, d)) {
            return 0;
        }
        if (tag == SERIAL_POSTLOAD) {
            CWwdGameObjectA* spr = m_object;
            i32 idx = g_gameReg->m_options[spr->m_smarts].m_colorIndex;
            CShadeTable* sel = g_gameReg->m_spriteFactory->GetSel(idx, 0);
            spr = m_object;
            spr->m_drawActive = 1;
            spr->m_drawFillCmd = SHADE_PAL_16;
            spr->m_drawFillArg = sel;
        }
        return 1;
    }

    template<> RVA(0x000464e0, 0x74)
    CActHandler* zDArray<CActHandler>::Resolve(i32 id) {
        char* r;
        m_grown = 0;
        if (id >= m_lo && id <= m_hi) {
            r = m_base + (id - m_lo) * m_stride;
        } else if (GrowTo(id, 0)) {
            r = m_base + (id - m_lo) * m_stride;
        } else {
            char* msg = g_errOutOfMem;
            g_retAddrBreadcrumb = GetRetAddr();
            m_errSink->Set(this, msg, 0xc);
            r = m_spare;
        }

        union {
            char* m_bytes;
            CActHandler* m_slot;
        } band;
        band.m_bytes = r;
        return band.m_slot;
    }
};
SIZE(0x54);

#endif // GRUNTZ_CFORTRESSFLAG_H
