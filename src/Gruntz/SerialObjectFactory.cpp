#include <Gruntz/SerialCounter.h> // own extern surface
#include <Gruntz/GruntzMgr.h>     // the mgr's real type
#include <Ints.h>
#include <string.h>

#include <Gruntz/GameRegistry.h> // CGameRegistry (mgr->m_world)

#include <rva.h>
#include <Io/GameSave.h>              // g_saveBuf (ex .cpp extern)
#include <DDrawMgr/DDrawSurfaceMgr.h> // RestoreChildren + HP_Callback (the parse dispatch)

// fwd: the (de)serialize dispatch this TU defines below (0xd2a0); ParseSerial hands
// it to RestoreChildren as the parse callback (retail wires the ILT thunk 0x4024e6).
i32 __cdecl SerialObjectFactory(void* ctx, void* ar, i32 mode, i32 typeId, void** ppObj);

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
    return mgr->m_world->RestoreChildren(reinterpret_cast<HP_Callback>(&SerialObjectFactory), s, 0)
           != 0;
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
// >512B leaf-first megafunction (6532 B). Deferred to the leaf-first final sweep per the
// same >512B REVERT rule as BattlezSetupDlgInit: this body inlines the FULL ctor of ~80
// distinct CUserBase-derived game object classes (verified: each case is operator-new +
// an inlined ctor that stamps ??_7C<Class> + ??_7CUserBase and runs the embedded-member
// ctors, e.g. CGrunt's 0x21a-byte inline ctor at 0xd37a). A faithful match REQUIRES every
// one of those ~80 leaf ctors matched first AND MSVC5 to inline each identically here;
// none are set up to inline-match yet, so any partial would under-count AND diverge its
// own regalloc. The structural dossier above (mode dispatch + typeId->class->size->vtable
// factory table) is the final-sweep starting point; the return-0 normalization artifact
// is kept per the >512B rule.
i32 __cdecl SerialObjectFactory(void* ctx, void* ar, i32 mode, i32 typeId, void** ppObj);
RVA(0x0000d2a0, 0x1984)
i32 __cdecl SerialObjectFactory(void* ctx, void* ar, i32 mode, i32 typeId, void** ppObj) {
    return 0;
}
