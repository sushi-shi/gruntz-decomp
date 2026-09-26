#ifndef GRUNTZ_GRUNTZ_CGRUNTVOICE_H
#define GRUNTZ_GRUNTZ_CGRUNTVOICE_H

#include <rva.h>

#include <Mfc.h>

#include <Gruntz/ActReg.h>
#include <Gruntz/ClockInterval.h>
#include <Gruntz/InGameIcon.h>
#include <Gruntz/UserLogic.h>

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

    ClockInterval m_playbackTiming;
    i32 m_sourceObjectId;
    i32 m_priority;
    i32 m_positionMode;
};

#endif // GRUNTZ_GRUNTZ_CGRUNTVOICE_H
