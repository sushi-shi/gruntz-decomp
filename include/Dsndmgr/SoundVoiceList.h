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

struct DSoundElem : public PureSoundElem {

    DSoundLink m_link;
    u32 m_tag;
    DirectSoundMgr* m_key;
};

// The shared list head: retail carries exactly one copy of each operation below
// (InsertHead 0x1390e0 .. Unlink 0x1391e0), reached from DirectSoundMgr,
// SoundDevice, SoundStream, CSymParser, CHashBase and CWwdGrid.
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

// Three typed list heads, each with its OWN destructor. cl emits one dtor COMDAT
// per type, and retail carries THREE distinct empty (`ret`-only) ones - 0x135ba0,
// 0x1364e0 and 0x1364f0 - reached from the unwind funclets of
// ??0/??1DSoundCloneInst (this+0x58) and ??0/??1SoundDevice (this+0x4, this+0xc).
// Three dtor COMDATs mean three classes; one type could only produce one.
struct DSoundBufferList : public DSoundList {
    ~DSoundBufferList() {}
};

struct DSoundVoiceList : public DSoundList {
    ~DSoundVoiceList() {}
};

struct DSoundCloneList : public DSoundList {
    ~DSoundCloneList() {}
};

#endif // SRC_DSNDMGR_SOUNDVOICELIST_H
