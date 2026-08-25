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

    CAniRecordBase2(i32 id, class CDDrawSurfaceMgr* owner)
        : CWapObj(owner, id, 0, CWapObj::NO_SEED) {
        m_buf = NULL;
    }

    // 0x165dd0 (RVA_COMPGEN pin at the keeper, DDrawSurfacePair.cpp - an RVA()
    // here would annotate BOTH cl dtor variants and collide with
    // ??_GCAniRecordBase2@0x165db0).
    virtual ~CAniRecordBase2() OVERRIDE {
        Unload();
    }
    RVA(0x00165d90, 0xb)
    virtual i32 IsLoaded() OVERRIDE {
        return m_buf != NULL;
    }

    virtual void Unload() OVERRIDE;
    RVA(0x00165da0, 0x6)
    virtual LoadableClassId GetClassId() OVERRIDE {
        return CLASSID_ANIRECORDBASE2;
    }

    virtual i32 CreatePaletteFromEntries(PALETTEENTRY* entries, i32 flag);
    virtual i32 CreatePaletteFromRgb(u8* data, i32 flag);
    virtual i32 LoadPaletteFromFile(char* path, i32 flag);
    virtual i32 CreatePaletteFromTrailingData(void* data, i32 size, i32 flag);
    virtual i32 PushPalette();
};

#endif // GRUNTZ_DDRAWMGR_ANIRECORDBASE2_H
