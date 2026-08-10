#ifndef GRUNTZ_DDRAWMGR_ANIRECORDBASE2_H
#define GRUNTZ_DDRAWMGR_ANIRECORDBASE2_H

#include <rva.h>

#include <Ints.h>
#include <Wap32/WapObj.h>

#include <stddef.h>

class CDDrawSurfaceMgr;

struct CDDPalette; // The class key is ABI-significant in MSVC mangling.

struct CAniRecordBase2 : public CWapObj {
    CDDPalette* m_buf;

    CAniRecordBase2() {}

    CAniRecordBase2(i32 field04, class CDDrawSurfaceMgr* owner)
        : CWapObj(owner, field04, 0, CWapObj::NO_SEED) {
        m_buf = NULL;
    }

    virtual ~CAniRecordBase2() OVERRIDE;
    virtual i32 IsLoaded() OVERRIDE;

    virtual void Unload() OVERRIDE;
    virtual LoadableClassId GetClassId() OVERRIDE;

    virtual i32 CreatePaletteFromEntries(i32 handle, i32 flag);
    virtual i32 CreatePaletteFromRgb(void* data, i32 flag);
    virtual i32 LoadPaletteFromFile(char* path, i32 flag);
    virtual i32 CreatePaletteFromTrailingData(void* data, i32 size, i32 flag);
    virtual i32 PushPalette();
};
SIZE(0x14);

#endif // GRUNTZ_DDRAWMGR_ANIRECORDBASE2_H
