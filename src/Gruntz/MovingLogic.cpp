#define CMOVINGLOGIC_STANDALONE_CTOR
#include <Gruntz/MovingLogic.h>
#include <Io/FileMem.h>
#include <strstrea.h>
#include <Gruntz/MovingLogicSerial.h>
#include <Gruntz/GameLevel.h>
#include <DDrawMgr/DDrawSurfaceMgr.h>
#include <rva.h>
#include <Gruntz/MotionState.h>

DATA(0x001f04f8)
const double g_motionNegHalf = -0.5;

DATA(0x001f04f0)
const double g_motionTimeScale = 0.001;

RVA(0x00013940, 0x1e1)
CMovingLogic::CMovingLogic() {}

RVA(0x00013bb0, 0x4)
LogicTypeId CMovingLogic::GetTypeTag() {
    return LOGIC_NONE;
}

RVA(0x00013bd0, 0x44)
CMovingLogic::~CMovingLogic() {}

RVA(0x0016cdd0, 0x22f)
ostream& WriteCurve(ostream& accum, const CMotionState& c) {
    accum << c.m_00;
    accum << c.m_08;
    accum << c.m_10;
    accum << c.m_18;
    accum << c.m_20;
    accum << c.m_28;
    accum << c.m_30;
    accum << c.m_38;
    accum << c.m_40;
    accum << c.m_48;
    accum << c.m_50;
    accum << c.m_70;
    accum << c.m_78;
    accum << c.m_80;
    accum << c.m_88;
    accum << c.m_90;
    accum << c.m_98;
    accum << c.m_a0;
    accum << c.m_a8;
    accum << c.m_b0;
    accum << c.m_b8;
    accum << c.m_c0;
    accum << c.m_c8;
    accum << c.m_d0;
    accum << c.m_d8;
    accum << c.m_e0;
    accum << c.m_e8;
    accum << c.m_f0;
    accum << c.m_f8;
    accum << c.m_100;
    return accum;
}

RVA(0x0016d000, 0x189)
istream& ReadCurve(istream& accum, CMotionState& c) {
    accum >> c.m_00;
    accum >> c.m_08;
    accum >> c.m_10;
    accum >> c.m_18;
    accum >> c.m_20;
    accum >> c.m_28;
    accum >> c.m_30;
    accum >> c.m_38;
    accum >> c.m_40;
    accum >> c.m_48;
    accum >> c.m_50;
    accum >> c.m_70;
    accum >> c.m_78;
    accum >> c.m_80;
    accum >> c.m_88;
    accum >> c.m_90;
    accum >> c.m_98;
    accum >> c.m_a0;
    accum >> c.m_a8;
    accum >> c.m_b0;
    accum >> c.m_b8;
    accum >> c.m_c0;
    accum >> c.m_c8;
    accum >> c.m_d0;
    accum >> c.m_d8;
    accum >> c.m_e0;
    accum >> c.m_e8;
    accum >> c.m_f0;
    accum >> c.m_f8;
    accum >> c.m_100;
    return accum;
}

// @early-stop
RVA(0x0016e7f0, 0x1cf)
i32 CUserLogic::SerializeMove(CFileMemBase* arc, i32 mode, i32 typeId, CGameObject* pObj) {
    if (arc == 0) {
        return 0;
    }
    switch (mode) {
        case 7: {

            i32 len;
            arc->Read(&len, 4);
            void* buf = RezAlloc(len);
            arc->Read(buf, len);
            istrstream accum(static_cast<char*>(buf), len);
            accum >> m_link.m_str;
            RezFree(buf);
            arc->Read(&m_28, 4);
            arc->Read(&m_2c, 4);
            arc->Read(&g_logicTypesRegistered, 4);
            arc->Read(&m_prevAnimSetNode, 4);
            m_0c = pObj;
            m_object = static_cast<CWwdGameObjectA*>(pObj);
            m_objAux = (pObj)->m_animWorker;
            m_deferredCallback = 0;
            m_gatedCallback = 0;
            m_28 = 0x3e9;

            break;
        }
        case 4: {

            char buf[0x100];
            ostrstream accum(buf, 0x100);
            accum << m_link.m_str;
            i32 len = accum.pcount();
            arc->Write(&len, 4);
            arc->Write(accum.str(), len);
            arc->Write(&m_28, 4);
            arc->Write(&m_2c, 4);
            arc->Write(&g_logicTypesRegistered, 4);
            arc->Write(&m_prevAnimSetNode, 4);

            break;
        }
    }
    return 1;
}

// @early-stop
RVA(0x0016ea90, 0x234)
void CMovingLogic::MovingSlot16() {

    m_140 = static_cast<i32>(Motion()->m_40);
    m_144 = static_cast<i32>(Motion()->m_48);
    Motion()->Step(static_cast<double>(g_frameTime) * g_motionTimeScale - Motion()->m_00);

    if ((m_object->m_flags & 0x10) && m_object->m_carrier != 0) {
        m_object->m_screenX += m_object->m_carrier->m_deltaX;
        Motion()->m_40 = static_cast<double>(m_object->m_screenX);
        m_object->m_screenY += m_object->m_carrier->m_deltaY;
        Motion()->m_48 = static_cast<double>(m_object->m_screenY);
    }

    if (m_object->m_moveMode == 1) {
        m_148 = m_object->OwnerMgr()->m_level->MoveToward(
            m_object,
            static_cast<i32>(Motion()->m_40),
            m_object->m_screenY,
            m_14c
        );
        Motion()->m_30 = 0.0;
    } else {
        m_object->m_flags &= ~0x10;
        m_148 = m_object->OwnerMgr()->m_level->MoveToward(
            m_object,
            static_cast<i32>(Motion()->m_40),
            static_cast<i32>(Motion()->m_48),
            m_14c
        );
    }

    CMotionState* ms = Motion();
    i32 sx = m_object->m_screenX;
    if (static_cast<i32>(Motion()->m_40) != sx) {
        double d = static_cast<double>(sx);
        ms->m_28 = ms->ArrivalVelX(d);
        double a0new = ms->m_a0 - (ms->m_40 - d);
        ms->m_40 = d;
        ms->m_a0 = a0new;
    }

    i32 sy = m_object->m_screenY;
    if (static_cast<i32>(Motion()->m_48) != sy) {
        double d = static_cast<double>(sy);
        ms->m_30 = ms->ArrivalVelY(d);
        double a8new = ms->m_a8 - (ms->m_48 - d);
        ms->m_48 = d;
        ms->m_a8 = a8new;
    }

    if (m_object->m_moveMode != 7) {
        i32 f = m_148;
        if (f & 0x800000) {
            Motion()->m_30 = -Motion()->m_30;
            return;
        }
        if (f & 0x40000) {
            Motion()->m_88 = static_cast<double>(m_140);
            Motion()->m_28 = Motion()->m_28 * g_motionNegHalf;
            return;
        }
        if (f & 0x80000) {
            Motion()->m_70 = static_cast<double>(m_140);
            Motion()->m_28 = Motion()->m_28 * g_motionNegHalf;
        }
    }
}

// @early-stop
RVA(0x0016f4a0, 0x1da)
i32 CMovingLogic::SerializeMove(CFileMemBase* arc, i32 mode, i32 typeId, CGameObject* pObj) {
    if (arc == 0) {
        return 0;
    }
    switch (mode) {
        case 7: {

            i32 len;
            arc->Read(&len, 4);
            void* buf = RezAlloc(len);
            arc->Read(buf, len);
            istrstream accum(static_cast<char*>(buf), len);
            ReadCurve(accum, *Motion());
            RezFree(buf);
            arc->Read(&m_140, 4);
            arc->Read(&m_144, 4);
            arc->Read(&m_148, 4);
            arc->Read(&m_14c, 4);

            break;
        }
        case 4: {

            char buf[0x100];
            ostrstream accum(buf, 0x100);
            WriteCurve(accum, *Motion());
            i32 len = accum.pcount();
            arc->Write(&len, 4);
            arc->Write(accum.str(), len);
            arc->Write(&m_140, 4);
            arc->Write(&m_144, 4);
            arc->Write(&m_148, 4);
            arc->Write(&m_14c, 4);

            break;
        }
    }
    return CUserLogic::SerializeMove(arc, mode, typeId, pObj) != 0;
}
