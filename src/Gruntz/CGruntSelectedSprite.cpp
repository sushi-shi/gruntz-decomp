// CGruntSelectedSprite.cpp - the "grunt is selected" indicator sprite
// (C:\Proj\Gruntz). A CUserLogic-derived game object; methods in ascending
// retail-RVA order:
//   ~CGruntSelectedSprite  @0x011e80 - the /GX leaf dtor (CUserLogic teardown).
//   SetCell                @0x07e9c0 - stash the (x,y) grunt cell, return 1.
//   Update                 @0x07e9f0 - track the selected grunt's screen pos.
//
// The 0x44 is a DESTRUCTOR (it stamps the CUserLogic 0x5e705c then CUserBase
// 0x5e70b4 vptr and tears down the +0x18 link via ~EngStr @0x16d2a0), NOT a
// ctor - identical in shape to ~CTimeBomb @0x012a70 / ~CInGameIcon @0x011d00.
#include <Gruntz/CGruntSelectedSprite.h>

// ~CGruntSelectedSprite @0x011e80 - the leaf adds no destructible members beyond
// CUserLogic, so its dtor folds the bare CUserLogic teardown: store the
// CUserLogic vptr (0x5e705c), inline-destruct the +0x18 link (the embedded
// ~EngStr call 0x16d2a0), store the CUserBase vptr (0x5e70b4). The destructible
// link forces the /GX EH frame; the empty body is enough for cl.
RVA(0x00011e80, 0x44)
CGruntSelectedSprite::~CGruntSelectedSprite() {}

// SetCell @0x07e9c0 - stash the (x,y) grunt cell into m_54/m_58, return 1.
RVA(0x0007e9c0, 0x16)
i32 CGruntSelectedSprite::SetCell(i32 x, i32 y) {
    m_54 = x;
    m_58 = y;
    return 1;
}

// Update @0x07e9f0 - resolve the grunt for cell (m_54,m_58) from the registry's
// grunt table; if that grunt is drawn (entry->m_1d8), sync the +0x38 object's
// helper and copy the grunt's screen position into the bound renderable so the
// "selected" ring tracks the grunt. Returns 0.
//
// @early-stop
// regalloc/scheduling wall (zero-register-pinning class): the logic is byte-exact
// but cl pins g_gameReg in a different register than retail (ecx vs edx) and emits
// the reg->m_68 load before the index lea-chain where retail defers it - the
// `m_1d8` second condition shifts the register pressure so the deref ordering is
// not source-steerable (the sibling Toy::Update, no m_1d8 check, reaches 99.3%).
// Every instruction matches modulo register names. Deferred to the final sweep.
RVA(0x0007e9f0, 0x5f)
i32 CGruntSelectedSprite::Update() {
    CIndicatorReg* reg = g_gameReg;
    CGruntEntry* e = ((CGruntEntry**)(reg->m_68 + 0x1c))[m_54 * 15 + m_58];
    if (e != 0 && e->m_1d8 != 0) {
        ((CIndicatorSyncHelper*)((char*)m_38 + 0x1a0))->Sync(g_indicatorSync);
        m_10->m_5c = e->m_10->m_5c;
        m_10->m_60 = e->m_10->m_60;
    }
    return 0;
}
