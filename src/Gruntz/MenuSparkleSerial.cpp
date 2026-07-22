#include <Gruntz/MenuSparkle.h> // the ONE canonical CMenuSparkle (the Grunt.h-world
#include <Io/FileMem.h> // the serialize stream (CSerialArchive == the real CFileMemBase)
#include <Globals.h>

DATA(0x001ea3d4)
i32 g_menuSparkleLo = 1000; // 0x5ea3d4
DATA(0x001ea3d8)
i32 g_menuSparkleHi = 5000; // 0x5ea3d8

RVA(0x000ae1c0, 0xae)
i32 CMenuSparkle::SerializeMove(CGruntArchive* arc, i32 mode, i32 a3, i32 a4) {
    if (arc == 0) {
        return 0;
    }
    // The slot-1 base serialize is the shared CMovingLogicBase::Serialize @0x16e7f0
    // (the real callee - CUserLogic::SerializeMove was a fake, unbound name for it).
    if (!CUserLogic::SerializeMove(static_cast<CSerialArchive*>(arc), mode, a3, a4)) {
        return 0;
    }
    if (!Chain(static_cast<CSerialArchive*>(arc), mode, a3, reinterpret_cast<CGameObject*>(a4))) {
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

#include <rva.h>
