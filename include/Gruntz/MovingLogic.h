#ifndef GRUNTZ_CMOVINGLOGIC_H
#define GRUNTZ_CMOVINGLOGIC_H

#include <rva.h>

#include <Mfc.h>

#include <Gruntz/CoordNode.h>
#include <Gruntz/LogicTypeId.h>
#include <Gruntz/MotionState.h>
#include <Gruntz/SerialArchive.h>
#include <Gruntz/UserLogic.h>

extern const double g_movingLogicMin;
extern const double g_movingLogicMax;

extern "C" u32 g_frameTime;
extern const double g_motionZScale;
extern u32 g_defaultZ;

class CMovingLogic : public CUserLogic {
public:
    RVA(0x0016f4a0, 0x1da)
    virtual i32
    SerializeMove(CFileMemBase* arc, SerialMode mode, LogicTypeId typeId, CGameObject* pObj)
        OVERRIDE {
        if (arc == 0) {
            return 0;
        }
        switch (mode) {
            case SERIAL_LOAD: {

                i32 len;
                arc->Read(&len, 4);
                void* buf = ::operator new(len);
                arc->Read(buf, len);
                istrstream accum(static_cast<char*>(buf), len);
                ReadCurve(accum, *Motion());
                ::operator delete(buf);
                arc->Read(&m_previousScreenPosition.m_x, 4);
                arc->Read(&m_previousScreenPosition.m_y, 4);
                arc->Read(&m_collisionFlags, 4);
                arc->Read(&m_moveFlags, 4);

                break;
            }
            case SERIAL_SAVE: {

                char buf[0x100];
                ostrstream accum(buf, 0x100);
                WriteCurve(accum, *Motion());
                i32 len = accum.pcount();
                arc->Write(&len, 4);
                arc->Write(accum.str(), len);
                arc->Write(&m_previousScreenPosition.m_x, 4);
                arc->Write(&m_previousScreenPosition.m_y, 4);
                arc->Write(&m_collisionFlags, 4);
                arc->Write(&m_moveFlags, 4);

                break;
            }
        }
        return CUserLogic::SerializeMove(arc, mode, typeId, pObj) != 0;
    }
    virtual LogicTypeId GetTypeTag() OVERRIDE {
        return LOGIC_NONE;
    }

    virtual void FinalizeStep(char* unused) OVERRIDE;

    char m_pad34[0x38 - 0x34];

public:
    CMovingLogic();

    CMovingLogic(CGameObject* owner);
    virtual ~CMovingLogic() OVERRIDE;

    virtual void AdvanceMotion();

    CMotionState* Motion() {
        return &m_motion;
    }

    CMotionState m_motion;
    Coord m_previousScreenPosition;
    i32 m_collisionFlags;
    i32 m_moveFlags;
};
SIZE_UNKNOWN();

inline CMovingLogic::CMovingLogic() {}

inline CMovingLogic::CMovingLogic(CGameObject* owner) : CUserLogic(owner) {}

inline CMovingLogic::~CMovingLogic() {}

extern const double g_motionTimeScale;
#endif // GRUNTZ_CMOVINGLOGIC_H
