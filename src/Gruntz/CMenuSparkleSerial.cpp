// CMenuSparkleSerial.cpp - CMenuSparkle::Serialize (0xae1c0): the class's vtable
// slot-1 serialize. Bail on a null archive; chain the base + the +0x34 sub-object
// serialize (bail if either fails); then round-trip two 4-byte globals through the
// archive (mode 4 = write via vtbl[0x30], mode 7 = read via vtbl[0x2c]). Plain /O2
// leaf (no /GX). The base/sub serialize + the archive Read/Write are reloc-masked.
#include <Gruntz/CMenuSparkleSerial.h>

// The two serialized 4-byte globals (.data). Reloc-masked DIR32 referents.
DATA(0x001ea3d4)
extern i32 g_5ea3d4; // 0x5ea3d4
DATA(0x001ea3d8)
extern i32 g_5ea3d8; // 0x5ea3d8

// ===========================================================================
// CMenuSparkle::Serialize  (0xae1c0)
// ===========================================================================
RVA(0x000ae1c0, 0xae)
i32 CMenuSparkle::Serialize(CMsSerialArchive* arc, i32 mode, i32 a3, i32 a4) {
    if (arc == 0) {
        return 0;
    }
    if (!BaseSerialize(arc, mode, a3, a4)) {
        return 0;
    }
    if (!m_34.Serialize(arc, mode, a3, a4)) {
        return 0;
    }
    if (mode != 4) {
        if (mode != 7) {
            return 1;
        }
        arc->Read(&g_5ea3d4, 4);
        arc->Read(&g_5ea3d8, 4);
        return 1;
    }
    arc->Write(&g_5ea3d4, 4);
    arc->Write(&g_5ea3d8, 4);
    return 1;
}
