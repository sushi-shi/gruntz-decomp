#ifndef GRUNTZ_GRUNTZ_CGRUNTVOICE_H
#define GRUNTZ_GRUNTZ_CGRUNTVOICE_H

#include <rva.h>
#include <Clock64.h>
#include <Wap32/ZVec.h>
#include <Wap32/zBitVec.h>

#include <Mfc.h>

#include <Gruntz/UserLogic.h>
#include <Gruntz/InGameIcon.h>
#include <Gruntz/ActReg.h>

struct CVoiceSample {};
SIZE_UNKNOWN();

struct StreamVoice;

class CGruntVoice : public CUserLogic, public CWapX {
public:
public:
    CGruntVoice(CGameObject* obj);

    virtual void FireActivation(i32 id) OVERRIDE;
    i32 Setup(i32 source, StreamVoice* sample, i32 playFlags, i32 owner);
    void Reset();

    i32 IdleHidden();
    i32 Update();

    StreamVoice* m_sample;

    union {
        Clock64 m_startStamp;
        struct {
            i32 m_icon;
            i32 m_5c;
        };
    };
    union {
        Clock64 m_duration;
        struct {
            i32 m_durationMs;
            i32 m_64;
        };
    };
    i32 m_source;
    i32 m_playFlags;
    i32 m_owner;
    char m_pad74[0x78 - 0x74];
};
SIZE(0x78);

typedef i32 (CUserLogic::*CActHandler)();

#endif // GRUNTZ_GRUNTZ_CGRUNTVOICE_H
