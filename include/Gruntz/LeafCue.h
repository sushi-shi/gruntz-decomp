#ifndef GRUNTZ_GRUNTZ_LEAFCUE_H
#define GRUNTZ_GRUNTZ_LEAFCUE_H

#include <Ints.h>
#include <rva.h>
#include <Gruntz/Loadable.h>

class DSoundCloneInst;
struct CParseSource;

struct LeafCue : public CLoadable {

    RVA(0x00158650, 0xb)
    virtual i32 IsLoaded() OVERRIDE {
        return m_10 != 0;
    }

    virtual void Unload() OVERRIDE;

    virtual ~LeafCue() OVERRIDE;

    LeafCue(i32 count, class CDDrawSurfaceMgr* handle);

    i32 LoadSoundA(void* riff);
    i32 LoadSoundB(void* src);
    i32 Configure(CParseSource* src);

    i32 PlayIfElapsed(i32 vol, i32 pan, i32 freqPct, i32 loop);

    i32 TriggerBlit(i32 pos, i32 center, i32 range1, i32 range2);

    DSoundCloneInst* m_10;
    i32 m_14;
    i32 m_18;
};
SIZE(0x1c);
inline LeafCue::LeafCue(i32 count, CDDrawSurfaceMgr* handle) : CLoadable(count, handle) {
    m_10 = 0;
    m_18 = 0;
    m_14 = 0;
}

#endif // GRUNTZ_GRUNTZ_LEAFCUE_H
