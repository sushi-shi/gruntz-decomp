#include <rva.h>
#include <Gruntz/CGameRegistry.h>
// TileTriggerLoad.cpp - the version-4 deserialize handler (0x1138b0) for a tile
// trigger-data record. Reached through the per-version Load dispatcher (0x513860,
// sub-selector 4) under the outer Serialize switch (0x517636). It pulls a fixed
// run of dword fields off the archive (virtual Read @ vtable byte 0x30) once the
// game registry's m_30 sub-manager is live. Names are placeholders; the field
// offsets + the emitted Read sequence are load-bearing.

// The serialize archive: Read(buf, len) sits at vtable slot 12 (byte 0x30). The
// dispatch `mov edx,[esi]; call [edx+0x30]` falls out of the polymorphic call.
class CSerialArchive {
public:
    virtual void s00();
    virtual void s01();
    virtual void s02();
    virtual void s03();
    virtual void s04();
    virtual void s05();
    virtual void s06();
    virtual void s07();
    virtual void s08();
    virtual void s09();
    virtual void s10();
    virtual void s11();
    virtual i32 Read(void* buf, i32 len); // slot 12 (+0x30)
};

// The game registry singleton; only its m_30 sub-manager gates this load.
DATA(0x0064556c)
extern CGameRegistry* g_gameReg; // ?g_gameReg (0x64556c)

// The tile trigger-data record being loaded. Reads land at +0x08..+0x20, +0x28
// (NB: +0x24 is skipped), then a 24-dword run from +0x2c.
class CTileTriggerData {
public:
    i32 LoadV4(CSerialArchive* ar); // 0x1138b0

    void* m_vptr; // +0x00
    i32 m_04;     // +0x04
    i32 m_08;     // +0x08
    i32 m_0c;     // +0x0c
    i32 m_10;     // +0x10
    i32 m_14;     // +0x14
    i32 m_18;     // +0x18
    i32 m_1c;     // +0x1c
    i32 m_20;     // +0x20
    i32 m_24;     // +0x24 (not read here)
    i32 m_28;     // +0x28
    i32 m_2c[24]; // +0x2c..+0x88
};
SIZE_UNKNOWN(CTileTriggerData);

// ===========================================================================
// 0x1138b0 - read the trigger-data block from the archive. Bails if the archive
// is null or the registry sub-manager (m_30) is not yet live; otherwise reads
// the eight scalar fields then the 24-dword tail run, returning 1.
// ===========================================================================
RVA(0x001138b0, 0xb4)
i32 CTileTriggerData::LoadV4(CSerialArchive* ar) {
    if (ar == 0) {
        return 0;
    }
    if (g_gameReg->m_30 == 0) {
        return 0;
    }
    ar->Read(&m_08, 4);
    ar->Read(&m_0c, 4);
    ar->Read(&m_10, 4);
    ar->Read(&m_14, 4);
    ar->Read(&m_18, 4);
    ar->Read(&m_1c, 4);
    ar->Read(&m_20, 4);
    ar->Read(&m_28, 4);
    i32* p = m_2c;
    i32 n = 24;
    do {
        ar->Read(p, 4);
        p++;
    } while (--n);
    return 1;
}
