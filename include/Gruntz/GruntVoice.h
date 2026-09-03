#ifndef GRUNTZ_GRUNTZ_CGRUNTVOICE_H
#define GRUNTZ_GRUNTZ_CGRUNTVOICE_H

#include <rva.h>

#include <Mfc.h>

#include <Clock64.h>
#include <Gruntz/ActReg.h>
#include <Gruntz/InGameIcon.h>
#include <Gruntz/UserLogic.h>
#include <Wap32/zBitVec.h>
#include <Wap32/ZVec.h>

struct StreamVoice;

enum {
    VOICE_INDICATOR_AT_LOGIC_OBJECT = 0,
    VOICE_INDICATOR_AT_IMAGE_ORIGIN = 1
};

class CGruntVoice : public CUserLogic, public CWapX {
    inline b32 PositionIndicatorAtLogicObject();
    inline b32 PositionIndicatorAtSourceObject();

public:
    CGruntVoice(CGameObject* obj);

    virtual void FireActivation(i32 actionId) OVERRIDE;
    i32 BeginPlayback(i32 sourceObjectId, StreamVoice* stream, i32 priority, i32 positionMode);
    void ResetPlayback();

    i32 HideIndicator();
    i32 UpdateIndicator();

    StreamVoice* m_stream;

    union {
        Clock64 m_startStamp;
        struct {
            i32 m_startStampLo;
            i32 m_startStampHi;
        };
    };
    union {
        Clock64 m_duration;
        struct {
            i32 m_durationMs;
            i32 m_durationHi;
        };
    };
    i32 m_sourceObjectId;
    i32 m_priority;
    i32 m_positionMode;
    char m_pad74[0x78 - 0x74];
};

typedef i32 (CUserLogic::*CActHandler)();

#endif // GRUNTZ_GRUNTZ_CGRUNTVOICE_H
