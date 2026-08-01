#ifndef GRUNTZ_CRANDOMAMBIENTSOUND_H
#define GRUNTZ_CRANDOMAMBIENTSOUND_H

#include <Gruntz/AmbientSound.h>
#include <Ints.h>
#include <Gruntz/GameRegistry.h>
#include <rva.h>

#include <Dsndmgr/DirectSoundMgr.h>
#include <Gruntz/GameRegMfcPtr.h>

#include <Rez/FrameClock.h>

extern "C" i32 __ftol(double v);

struct AmbSoundRecord {
    char m_pad00[0x10];
    DirectSoundMgr* m_mgr;
};
SIZE_UNKNOWN();

class CRandomAmbientSound : public CAmbientSound {
public:
    CRandomAmbientSound() {}

    virtual void Update(i32 x, i32 y, i32 force) OVERRIDE;

    void StopPos(i32 obj);
    i32 TickObj(i32 obj);

    virtual ~CRandomAmbientSound() OVERRIDE {}

    void InitCycleTiming(
        i32 playDurationMin,
        i32 playDurationMax,
        i32 silenceDurationMin,
        i32 silenceDurationMax
    );

    i32 m_playDurationMin;
    i32 m_playDurationMax;
    i32 m_silenceDurationMin;
    i32 m_silenceDurationMax;
    i32 m_countdownMs;
    i32 m_phase;
};
SIZE(0x58);

#endif // GRUNTZ_CRANDOMAMBIENTSOUND_H
