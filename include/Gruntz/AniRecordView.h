#ifndef GRUNTZ_CANIRECORDVIEW_H
#define GRUNTZ_CANIRECORDVIEW_H

#include <rva.h>

#include <Mfc.h>

#include <Enums.h>
#include <Ints.h>
#include <Wwd/WwdAnimStepMode.h>

class SoundCueRegistry;
class CDDrawSurfaceMgr;
struct SoundCue;

struct CDDPalette; // The class key is ABI-significant in MSVC mangling.

GZ_ENUM_FLAGS_BEGIN(AniRecordFlags, u16)
    ANI_RECORD_FLAG_FRAME_COUNT = 0x01,
    ANI_RECORD_FLAG_HAS_CUES = 0x02,
    ANI_RECORD_FLAG_POSITIONAL_CUE = 0x04,
    ANI_RECORD_FLAG_CULL_CUE_WHEN_NOT_DRAWN = 0x08
GZ_ENUM_FLAGS_END(AniRecordFlags, u16)
GZ_ENUM_FLAGS_OPS(AniRecordFlags)

GZ_ENUM_CONST_BEGIN(AniRecordTiming)
    ANI_FRAME_QUANTUM_MS = 22
GZ_ENUM_CONST_END(AniRecordTiming)

struct CAniRecordView : public CObject {
    virtual ~CAniRecordView() OVERRIDE {
        CAniRecordView* r = this;
        if (r->m_cues != NULL) {
            delete[] r->m_cues;
        }
        r->m_loopMode = WWDLOOP_INVALID;
        r->m_cueCount = 0;
        r->m_cues = NULL;
    }

    i32 Parse(SoundCueRegistry* ctx, const i16* src);
    i32 GetDurationMs();
    void ResolveIndices(SoundCueRegistry* owner, const char* str);

    i32 Rng2Next();

    i32 GetRandomNumber() {
        static long holdrand = timeGetTime();
        return (((holdrand = holdrand * 214013L + 2531011L) >> 16) & 0x7fff);
    }

    inline CAniRecordView() {
        m_cueCount = 0;
        m_cues = NULL;
        m_loopMode = WWDLOOP_INVALID;
    }

    GZ_ENUM_STORAGE(AniRecordFlags, u16) m_flags;
    u16 m_pad06;
    WwdAnimStepMode m_stepMode;
    WwdAnimLoopMode m_loopMode;
    WwdAnimPositionMode m_positionMode;
    i32 m_param;
    i32 m_duration;
    i32 m_drawValue;
    i32 m_positionDeltaX;
    i32 m_positionDeltaY;
    u16 m_reserved28; // parsed from ANI frame record; never read
    u16 m_pad2a;
    i32 m_cueCount;
    SoundCue** m_cues;
};

#endif // GRUNTZ_CANIRECORDVIEW_H
