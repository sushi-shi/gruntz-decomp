#ifndef DSNDMGR_SOUNDVOLUMERAMP_H
#define DSNDMGR_SOUNDVOLUMERAMP_H

#include <rva.h>

#include <Dsndmgr/SoundTask.h>

class SoundBuffer;

struct SoundVolumeRamp : public SoundTask {

    virtual i32 Tick(i32 now) OVERRIDE;
    virtual i32 Stop() OVERRIDE;

    i32 m_targetVolumePct;
    i32 m_initialVolumePct;
    i32 m_rampDurationMs;
    i32 m_rampStartTime;

    SoundVolumeRamp(
        i32 targetVolumePct,
        i32 initialVolumePct,
        i32 durationMs,
        SoundBuffer* buffer,
        b32 stopAndRewind,
        i32 startTime
    );
};

#endif // DSNDMGR_SOUNDVOLUMERAMP_H
