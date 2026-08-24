#ifndef DSNDMGR_SOUNDTASK_H
#define DSNDMGR_SOUNDTASK_H

#include <rva.h>

#include <Dsndmgr/IntrusiveList.h>
#include <Enums.h>

GZ_ENUM_CONST_BEGIN(SoundTaskTag)
    SOUND_TASK_TAG_VOLUME_RAMP = 1,
    SOUND_TASK_TAG_ALL = 0xffff
GZ_ENUM_CONST_END(SoundTaskTag)

class SoundBuffer;

// The dtor at 0x137330 restores this class's vptr. SoundVolumeRamp's ctor writes
// +0xc, +0x10 and +0x14 in one run before its own vptr stamp, proving that the
// link, tag, buffer and stop flag belong to this base.
struct SoundTask {
    virtual i32 Tick(i32 timestampMs) = 0;
    virtual i32 Stop() = 0;

    IntrusiveLink m_link;
    u32 m_tag;
    SoundBuffer* m_buffer;
    i32 m_stopAndRewind;

    SoundTask(u32 tag, SoundBuffer* buffer, i32 stopAndRewind)
        : m_tag(tag), m_buffer(buffer), m_stopAndRewind(stopAndRewind) {}

    // No class-level `operator delete`: retail's new-cleanup funclet calls the
    // global operator delete; a class-level forwarder changes that referent.
    RVA(0x00137330, 0x7)
    ~SoundTask() {}
};

struct SoundTaskList : public IntrusiveList {
    RVA(0x001364f0, 0x1)
    ~SoundTaskList() {}

    void RemoveMatching(SoundBuffer* buffer, u32 tag);
};

#endif // DSNDMGR_SOUNDTASK_H
