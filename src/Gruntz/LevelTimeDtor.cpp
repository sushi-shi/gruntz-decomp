// LevelTimeDtor.cpp - CLevelTime's /GX leaf destructor (C:\Proj\Gruntz).
//
// This TU hosts the leaf dtor + the 1-arg ctor (the "unrolled" CUserLogic(obj)
// prologue) against the matched <Gruntz/UserLogic.h> teardown.
#include <Gruntz/LevelTimeDtor.h>
#include <Gruntz/LogicTypeTableInline.h> // unrolled built-in logic-type registration

// CLevelTime::~CLevelTime @0x00011a50 - folds the bare CUserLogic teardown: store
// the CUserLogic vptr (0x5e705c), inline-destruct the +0x18 link (the embedded
// ~EngStr call 0x16d2a0), store the CUserBase vptr (0x5e70b4). The destructible
// link forces the /GX EH frame.
RVA(0x00011a50, 0x44)
CLevelTime::~CLevelTime() {}

// CLevelTime::CLevelTime @0x9b8b0 - fold the shared CUserLogic(obj) init (with the
// built-in logic types inlined-registered), then flag the sub-object (+0x08 bit 1).
// @early-stop
// eh-ctor-vptr-restamp-position wall (docs/patterns/eh-ctor-vptr-restamp-position.md):
// body byte-identical (incl. the unrolled logic-type registration); residual is the
// /GX leaf-vptr re-stamp position + EH-state ids.
RVA(0x0009b8b0, 0x18f)
CLevelTime::CLevelTime(CGameObject* obj) : CTileLogic(obj) {
    m_38->m_flags |= 2;
}

// class-metadata SIZE sweep (misc-Gruntz A-C): SIZE(CLevelTime, 0x54) now lives in
// <Gruntz/LevelTimeDtor.h> (pinned by the StateDispatch new-site push 0x54).
