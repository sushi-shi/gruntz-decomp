#ifndef GRUNTZ_CRANDOMAMBIENTSOUND_H
#define GRUNTZ_CRANDOMAMBIENTSOUND_H

#include <rva.h>

#include <Dsndmgr/SoundBuffer.h>
#include <Globals.h>
#include <Gruntz/AmbientSound.h>
#include <Gruntz/GameRegistry.h>
#include <Gruntz/GameRegMfcPtr.h>
#include <Ints.h>
#include <Rez/FrameClock.h>

class CRandomAmbientSound : public CAmbientSound {
public:
    CRandomAmbientSound() {}

    virtual void Update(i32 x, i32 y, b32 immediate) OVERRIDE;

    virtual ~CRandomAmbientSound() OVERRIDE {}

    void InitCycleTiming(
        i32 playDurationMin,
        i32 playDurationMax,
        i32 silenceDurationMin,
        i32 silenceDurationMax
    );

    CRange<i32> m_playDuration;
    CRange<i32> m_silenceDuration;
    i32 m_countdownMs;
    b32 m_playPhase;
};

#endif // GRUNTZ_CRANDOMAMBIENTSOUND_H
