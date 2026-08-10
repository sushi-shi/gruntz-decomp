#ifndef SRC_DSNDMGR_SOUNDVOICELIST_H
#define SRC_DSNDMGR_SOUNDVOICELIST_H

#include <rva.h>

#include <Enums.h>

GZ_ENUM_CONST_BEGIN(SoundVoiceTag)
    SOUND_VOICE_TAG_ALL = 0xffff
GZ_ENUM_CONST_END(SoundVoiceTag)

class DirectSoundMgr;

struct DSoundLink {
    DSoundLink* m_next;
    DSoundLink* m_prev;
};
SIZE(0x8);

// Language-forced container-of adjustment at the intrusive-list boundary.
template<class T> inline T* elemOf(DSoundLink* link) {
    return link ? reinterpret_cast<T*>((reinterpret_cast<char*>(link) - 4)) : 0;
}

struct PureSoundElem {
    virtual i32 Tick(i32 now) = 0;
    virtual i32 Stop() = 0;

    // No class-level `operator delete`: retail's unwind funclet for a
    // `new`-cleanup on this class calls the GLOBAL ??3@YAXPAX@Z, and a
    // class-level forwarder cannot produce that - cl emits
    // ??3PureSoundElem@@SAXPAX@Z and has the funclet call that instead.
    ~PureSoundElem() {}
};
SIZE(0x4);

struct DSoundElem : public PureSoundElem {

    DSoundLink m_link;
    u32 m_tag;
    DirectSoundMgr* m_key;
};
SIZE(0x14);

struct DSoundList {
    DSoundLink* m_head;
    DSoundLink* m_tail;

    DSoundList() : m_head(0), m_tail(0) {}

    ~DSoundList() {}

    void InsertHead(DSoundLink* node);
    void InsertTail(DSoundLink* node);
    void InsertAfter(DSoundLink* after, DSoundLink* node);
    void InsertBefore(DSoundLink* before, DSoundLink* node);
    void Unlink(DSoundLink* node);
    void RemoveMatching(DirectSoundMgr* key, u32 tag);
};
SIZE(0x8);

#endif // SRC_DSNDMGR_SOUNDVOICELIST_H
