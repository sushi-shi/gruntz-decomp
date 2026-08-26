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

struct SoundTask : public IntrusiveLink {
    virtual i32 Tick(i32 timestampMs) = 0;
    virtual i32 Stop() = 0;

    u32 m_tag;
    SoundBuffer* m_buffer;
    b32 m_stopAndRewind;

    SoundTask(u32 tag, SoundBuffer* buffer, b32 stopAndRewind)
        : m_tag(tag), m_buffer(buffer), m_stopAndRewind(stopAndRewind) {}

    RVA(0x00137540, 0x7)
    ~SoundTask() {}
};

struct SoundTaskList : public IntrusiveList {
    RVA(0x00136700, 0x1)
    ~SoundTaskList() {}

    void RemoveMatching(SoundBuffer* buffer, u32 tag);
};

#endif // DSNDMGR_SOUNDTASK_H
