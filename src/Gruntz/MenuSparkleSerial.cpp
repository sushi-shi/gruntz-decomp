// MenuSparkleSerial.cpp - CMenuSparkle::SerializeMove (0xae1c0): the class's vtable
// slot-1 serialize. Bail on a null archive; chain the CUserLogic base serialize +
// the +0x34 sub-object serialize (bail if either fails); then round-trip two 4-byte
// globals through the archive (mode 4 = write via vtbl[0x30], mode 7 = read via
// vtbl[0x2c]). Plain /O2 leaf (no /GX). The base/sub serialize + the archive
// Read/Write are reloc-masked.
#include <Gruntz/MenuSparkleSerial.h>
#include <Io/FileMem.h> // the serialize stream (CSerialArchive == the real CFileMemBase)
#include <Globals.h>

// The two serialized 4-byte globals (owner-TU defs; VA 0x5ea3d4/0x5ea3d8). Reloc-
// masked DIR32 referents; defaults 1000/5000, round-tripped through the archive.
DATA(0x001ea3d4)
i32 g_menuSparkleLo = 1000; // 0x5ea3d4
DATA(0x001ea3d8)
i32 g_menuSparkleHi = 5000; // 0x5ea3d8

// ===========================================================================
// CMenuSparkle::SerializeMove  (0xae1c0) - the class's vtable slot-1 serialize.
// ===========================================================================
RVA(0x000ae1c0, 0xae)
i32 CMenuSparkle::SerializeMove(CGruntArchive* arc, i32 mode, i32 a3, i32 a4) {
    if (arc == 0) {
        return 0;
    }
    // The slot-1 base serialize is the shared CMovingLogicBase::Serialize @0x16e7f0
    // (the real callee - CUserLogic::SerializeMove was a fake, unbound name for it).
    if (!CUserLogic::SerializeMove((CSerialArchive*)arc, mode, a3, a4)) {
        return 0;
    }
    if (!m_34.Chain((CSerialArchive*)arc, mode, a3, (CGameObject*)a4)) {
        return 0;
    }
    if (mode != 4) {
        if (mode != 7) {
            return 1;
        }
        arc->Read(&g_menuSparkleLo, 4);
        arc->Read(&g_menuSparkleHi, 4);
        return 1;
    }
    arc->Write(&g_menuSparkleLo, 4);
    arc->Write(&g_menuSparkleHi, 4);
    return 1;
}

// class-metadata SIZE sweep (misc-Gruntz A-C): matching-neutral, hosted at
// .cpp EOF (see docs/class-metadata-sweep-log.md). SIZE_UNKNOWN = size not yet pinned.
#include <rva.h>
SIZE_UNKNOWN(CMenuSparkle);
