// GruntWingzTimeSprite.cpp - the grunt "wingz" time eyecandy sprite (C:\Proj\Gruntz).
//
// Two trace-discovered CGruntWingzTimeSprite methods, defined in ascending
// retail-RVA order:
//   ~CGruntWingzTimeSprite @0x0121f0 - the /GX leaf dtor (folds the CUserLogic teardown).
//   GetWingzTime           @0x07fd90 - a tiny __stdcall +0x3f8 accessor (ret 4).
//
// CGruntWingzTimeSprite : CUserLogic (the base hierarchy comes from <Gruntz/UserLogic.h>).
// Only offsets / code bytes are load-bearing; names are placeholders for the
// recovered engine identities.
#include <Gruntz/GruntWingzTimeSprite.h>
#include <Gruntz/LogicTypeId.h>
#include <Bute/ButeTree.h> // g_buteTree.Find (0x16d190) - the "A" animset seed

// The engine bute store the sprite ctor queries for its "A" animset node. wwdfile /
// others own the DATA label (0x2bf620); declared extern so the address reloc-masks.

// CGruntWingzTimeSprite::GetTypeTag (0x000121a0) is now an inline member in the class header.

// CGruntWingzTimeSprite::CGruntWingzTimeSprite @0x0007fcc0 - the /GX HUD sprite ctor.
// Chains the CGruntHealthSprite base ctor (0x7eb00, via thunk 0x3224; declared-only
// here so the base `call` reloc-masks). Same shape as CGruntStaminaSprite's ctor
// (different sprite key + anchor constants); see GruntStaminaSprite.cpp.
//
// @confidence: high
// @source: rtti-vptr
// @early-stop
// eh-ctor-vptr-restamp-position wall (94.76%): real polymorphic base, body+offsets
// byte-identical; the single residual is the leaf vptr re-stamp (mov [esi],&??_7) that
// MSVC5's /GX EH-state machine sinks into the first throwing call's state 0 instead of
// retail's eager post-base-ctor stamp. NOT source-steerable
// (docs/patterns/eh-ctor-vptr-restamp-position.md). Deferred to the final sweep.
RVA(0x0007fcc0, 0xa0)
CGruntWingzTimeSprite::CGruntWingzTimeSprite(CGameObject* obj) : CGruntHealthSprite(obj) {
    m_38->ApplyLookupSprite("GAME_GRUNTWINGZTIMESPRITE", 1);
    m_prevAnimSetNode = m_objAux->m_1c;
    m_objAux->m_1c = g_buteTree.Find("A");
    CGameObject* o = m_object;
    if (o->m_latchedAnimId != 0xdbba0) {
        o->m_latchedAnimId = 0xdbba0;
        o->m_flags |= 0x20000;
    }
    m_health = 0; // +0x5c  wingz-time icon screen-offset X (reuses the base slot)
    m_60 = -0x26; // +0x60  wingz-time icon screen-offset Y (drawn above the grunt)
}

// CGruntWingzTimeSprite::~CGruntWingzTimeSprite @0x0121f0 - the leaf adds no
// destructible members beyond CUserLogic, so its dtor folds the bare CUserLogic
// teardown: store the CUserLogic vptr (0x5e705c), inline-destruct the +0x18 link
// (the embedded ~EngStr call 0x16d2a0), store the CUserBase vptr (0x5e70b4). The
// destructible link forces the /GX EH frame. Byte-identical in shape to the
// established leaf dtors; the empty body is enough for cl.
RVA(0x000121f0, 0x44)
CGruntWingzTimeSprite::~CGruntWingzTimeSprite() {}

// GetWingzTime @0x07fd90 - free __stdcall accessor: read the bound grunt's +0x3f8
// wingz-timer field and return it (single stack arg, callee cleanup -
// `mov eax,[esp+4]; mov eax,[eax+0x3f8]; ret 4`). Not a sprite member: the ecx
// trace mis-homed this __stdcall callee (stale-ecx owner); it reads a foreign
// CGrunt and is never stored as a fn pointer.
RVA(0x0007fd90, 0xd)
i32 __stdcall GetWingzTime(CGrunt* o) {
    return o->m_wingzTime;
}
