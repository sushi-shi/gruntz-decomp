// DroppedObjectSerialize.cpp - the slot-1 Serialize round-trips for CObjectDropper
// (0xc6680) and CDroppedObject (0xc73a0), homed from the GapFunctions stubs. Both are
// the CRollingBall::Serialize archetype (RollingBall.cpp): chain the shared CUserLogic
// serialize helper on `this`, then the +0x34 CSerialObjRef sub-object (both gate on
// failure), then stream the leaf's fields through the archive's read/write vtable slots
// (mode 4 = Write @+0x30, mode 7 = Read @+0x2c). Homed in their own unit so they cannot
// perturb the matched dtors/ctors in the sibling TUs. Only offsets + code bytes are
// load-bearing.
#include <rva.h>

#include <Gruntz/ObjectDropper.h>   // CObjectDropper : CUserLogic
#include <Gruntz/DroppedObject.h>   // CDroppedObject : CUserLogic
#include <Gruntz/SerialArchive.h>   // CSerialArchive (Read @+0x2c / Write @+0x30)
#include <Gruntz/SerialObjRef.h>    // CSerialObjRef::Chain (0x8c00)
#include <Gruntz/GameRegistry.h>    // g_gameReg (WwdGameReg) for the mode-8 fill-arg source
#include <Gruntz/LightFxMgr.h>      // g_gameReg->m_logicPump (+0x78) -> m_tables[5] (+0x28)

// The game-registry singleton (VA 0x64556c); its +0x78 light-FX pump supplies the
// mode-8 draw-fill arg. Reference-only extern (reloc-masks against the DATA-label owner).
extern CGameRegistry* g_gameReg;

// CObjectDropper::Serialize (0xc6680): the base/chain gate, then the +0x88/+0x90 drop-
// timing i64 pair, then the +0x58..+0x80 move/state fields; mode 8 instead seeds a
// draw-fill command on the bound object from the light-FX table set.
RVA(0x000c6680, 0x1b4)
i32 CObjectDropper::Serialize(CSerialArchive* ar, i32 tag, i32 c, i32 d) {
    if (!SerializeChain((i32)ar, tag, c, d)) {
        return 0;
    }
    if (!((CSerialObjRef*)((char*)this + 0x34))->Chain(ar, tag, c, (CSerialObj*)d)) {
        return 0;
    }

    // The drop-timing i64 pair (+0x88/+0x90), streamed through a walking pointer.
    char* p = (char*)this + 0x88;
    switch (tag) {
        case 4:
            ar->Write(p, 8);
            p += 8;
            ar->Write(p, 8);
            break;
        case 7:
            ar->Read(p, 8);
            p += 8;
            ar->Read(p, 8);
            break;
    }

    // The move/state field list (+0x58..+0x80); mode 8 seeds a draw-fill instead.
    switch (tag) {
        case 4:
            ar->Write(&m_speed, 8);
            ar->Write(&m_posX, 8);
            ar->Write(&m_posY, 8);
            ar->Write(&m_travelDx, 4);
            ar->Write(&m_travelDy, 4);
            ar->Write(&m_lastDropTileX, 4);
            ar->Write(&m_lastDropTileY, 4);
            ar->Write(&m_scrollMode, 4);
            break;
        case 7:
            ar->Read(&m_speed, 8);
            ar->Read(&m_posX, 8);
            ar->Read(&m_posY, 8);
            ar->Read(&m_travelDx, 4);
            ar->Read(&m_travelDy, 4);
            ar->Read(&m_lastDropTileX, 4);
            ar->Read(&m_lastDropTileY, 4);
            ar->Read(&m_scrollMode, 4);
            break;
        case 8: {
            i32 fill = (i32)g_gameReg->m_logicPump->m_tables[5];
            CGameObject* o = m_object;
            o->m_drawActive = 1;
            o->m_drawFillArg = fill;
            o->m_drawFillCmd = 7;
            break;
        }
    }
    return 1;
}

// CDroppedObject::Serialize (0xc73a0): base/chain gate, then the +0x58/+0x60 doubles
// and the +0x68 landing row.
RVA(0x000c73a0, 0xb5)
i32 CDroppedObject::Serialize(CSerialArchive* ar, i32 tag, i32 c, i32 d) {
    if (!SerializeChain((i32)ar, tag, c, d)) {
        return 0;
    }
    if (!((CSerialObjRef*)((char*)this + 0x34))->Chain(ar, tag, c, (CSerialObj*)d)) {
        return 0;
    }
    switch (tag) {
        case 4:
            ar->Write(&m_timePerTile, 8);
            ar->Write(&m_fallY, 8);
            ar->Write(&m_landY, 4);
            break;
        case 7:
            ar->Read(&m_timePerTile, 8);
            ar->Read(&m_fallY, 8);
            ar->Read(&m_landY, 4);
            break;
    }
    return 1;
}
