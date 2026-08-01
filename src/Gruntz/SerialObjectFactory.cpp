#include <Gruntz/SerialObjectFactory.h> // this TU's external declarations
#include <Gruntz/SerialCounter.h>       // own extern surface
#include <Gruntz/GruntzMgr.h>           // the mgr's real type
#include <Ints.h>
#include <string.h>

#include <Gruntz/GameRegistry.h> // CGameRegistry (mgr->m_world)

#include <rva.h>
#include <Io/GameSave.h>              // g_saveBuf (ex .cpp extern)
#include <DDrawMgr/DDrawSurfaceMgr.h> // RestoreChildren + HP_Callback (the parse dispatch)

// fwd: the (de)serialize dispatch this TU defines below (0xd2a0); ParseSerial hands
// it to RestoreChildren as the parse callback (retail wires the ILT thunk 0x4024e6).

RVA(0x0000d210, 0x65)
i32 ParseSerial(CGruntzMgr* mgr, char* s) {
    if (mgr == 0) {
        return 0;
    }
    if (s == 0) {
        return 0;
    }
    if (strlen(s) == 0) {
        return 0;
    }
    g_serialCounter = 0;
    memset(g_saveBuf, 0, 0x90);
    if (mgr->m_world == 0) {
        return 0;
    }
    return mgr->m_world->RestoreChildren(&SerialObjectFactory, s, 0) != 0;
}

// SerialObjectFactory (0xd2a0, __cdecl, ends 0xec24): the game's (de)serialize object
// dispatch - the callback ParseSerial's Parse156530 code table (0x4024e6/0x401e9c) and
// gamesave:SaveGame hand to the parser. args (ctx, ar, mode, typeId, ppObj): guards
// ctx/ar non-null, then a first switch on `mode` (1..0xa) - modes 1/2 virtual-call the
// archive (slot 0x30 / 0x2c) over the 0x90-byte g_saveBuf (0x629930) header; mode 9 is
// the OBJECT FACTORY: a second switch on typeId-1000 (0..0x44, 69 cases) that `operator
// new`s the class for that tag (CGrunt 0x8d8, CRollingBall 0xa0, ...) and INLINES its
// full ctor - stamping the derived + CUserBase (0x5e70b4) vtables, constructing the
// embedded members, zero-initing the scalar fields - then writes the object through
// ppObj and returns 1. ~80 game classes are constructed inline (??_7CGrunt/CRollingBall/
// CAniCycle/CSingleFrameMessage/CDoNothing/.../CGuardPoint - the full vtable set is in
// the reloc table 0xd3af..0xe9e5).
//
// @early-stop
RVA(0x0000d2a0, 0x1984)
i32 __cdecl SerialObjectFactory(void* ctx, void* ar, i32 mode, i32 typeId, void* payload) {
    return 0;
}
