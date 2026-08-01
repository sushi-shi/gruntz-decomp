#ifndef GRUNTZ_CANIRECORDVIEW_H
#define GRUNTZ_CANIRECORDVIEW_H
#include <rva.h>

#include <Ints.h>
#include <Mfc.h>

class CDDrawSubMgrLeafScan;
class CDDrawSurfaceMgr;

struct CDDPalette; // The class key is ABI-significant in MSVC mangling.

struct CAniRecordView : public CObject {
    virtual ~CAniRecordView() OVERRIDE;

    i32 Parse(void* ctx, const i16* src);
    i32 GetSize();
    void ResolveIndices(CDDrawSubMgrLeafScan* owner, const char* str);

    inline CAniRecordView() {
        m_count = 0;
        m_indices = 0;
        m_owner = 0xffff;
    }

    u16 m_flags;
    u16 m_06;
    i32 m_08;

    i32 m_owner;
    i32 m_palette;
    i32 m_seedFrame;
    i32 m_frameCount;
    i32 m_1c;
    i32 m_20;
    i32 m_24;
    u16 m_28;
    u16 m_2a;
    i32 m_count;
    i32* m_indices;
};
SIZE_UNKNOWN();

#endif // GRUNTZ_CANIRECORDVIEW_H
