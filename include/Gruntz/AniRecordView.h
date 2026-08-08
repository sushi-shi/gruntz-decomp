#ifndef GRUNTZ_CANIRECORDVIEW_H
#define GRUNTZ_CANIRECORDVIEW_H

#include <rva.h>

#include <Mfc.h>

#include <Ints.h>
#include <Wwd/WwdAnimStepMode.h>

class CDDrawSubMgrLeafScan;
class CDDrawSurfaceMgr;
struct LeafCue;

struct CDDPalette; // The class key is ABI-significant in MSVC mangling.

struct CAniRecordView : public CObject {
    virtual ~CAniRecordView() OVERRIDE;

    i32 Parse(void* ctx, const i16* src);
    i32 GetSize();
    void ResolveIndices(CDDrawSubMgrLeafScan* owner, const char* str);

    // A __thiscall whose body never touches `this` (it steps the two globals at
    // 0x6c2798/0x6c278c).  The receiver is proved by the call sites in
    // CAniAdvanceCursor::Advance: `mov ecx,edi` (edi = m_element) immediately
    // before each `call 0x15cbe0`, dead for a __cdecl callee.
    i32 Rng2Next();

    // Monolith's GetRandomNumber, in-class (implicitly inline, no keyword) so the
    // local static is emitted COMMON with this class in its mangled name - which
    // is what gives this module its own guard/seed pair. See <Gruntz/GameRand.h>.
    i32 GetRandomNumber() {
        static long holdrand = timeGetTime();
        return (((holdrand = holdrand * 214013L + 2531011L) >> 16) & 0x7fff);
    }

    inline CAniRecordView() {
        m_cueCount = 0;
        m_cues = NULL;
        m_loopMode = WWDLOOP_INVALID;
    }

    u16 m_flags;
    u16 m_pad06;
    WwdAnimStepMode m_stepMode;
    WwdAnimLoopMode m_loopMode;
    WwdAnimPositionMode m_positionMode;
    i32 m_param;
    i32 m_frameTime;
    i32 m_drawValue;
    i32 m_positionDeltaX;
    i32 m_positionDeltaY;
    u16 m_reserved28; // parsed from ANI frame record; never read
    u16 m_pad2a;
    i32 m_cueCount;
    LeafCue** m_cues;
};
SIZE(0x34);

#endif // GRUNTZ_CANIRECORDVIEW_H
