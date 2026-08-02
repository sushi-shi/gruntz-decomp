#ifndef GRUNTZ_CANIRECORDVIEW_H
#define GRUNTZ_CANIRECORDVIEW_H

#include <rva.h>

#include <Mfc.h>

#include <Ints.h>

class CDDrawSubMgrLeafScan;
class CDDrawSurfaceMgr;
struct LeafCue;

struct CDDPalette; // The class key is ABI-significant in MSVC mangling.

struct CAniRecordView : public CObject {
    virtual ~CAniRecordView() OVERRIDE;

    i32 Parse(void* ctx, const i16* src);
    i32 GetSize();
    void ResolveIndices(CDDrawSubMgrLeafScan* owner, const char* str);

    inline CAniRecordView() {
        m_cueCount = 0;
        m_cues = 0;
        m_loopMode = 0xffff;
    }

    u16 m_flags;
    u16 m_pad06;
    i32 m_stepMode;
    i32 m_loopMode;
    i32 m_positionMode;
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
