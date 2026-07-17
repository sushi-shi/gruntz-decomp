// LevelTimeDtor.cpp - CLevelTime's /GX leaf destructor (C:\Proj\Gruntz).
//
// This TU hosts the leaf dtor + the 1-arg ctor (the "unrolled" CUserLogic(obj)
// prologue) against the matched <Gruntz/UserLogic.h> teardown.
#include <Gruntz/LevelTimeDtor.h>
#include <Gruntz/LogicTypeTableInline.h> // unrolled built-in logic-type registration
#include <Gruntz/SerialArchive.h> // CSerialArchive (the inherited CWapX::Chain arg; ex SerialObjRef.h)
#include <Gruntz/SerialArchive.h>        // the serialize stream (== the real CFileMemBase)

// CLevelTime::~CLevelTime @0x00011a50 - folds the bare CUserLogic teardown: store
// the CUserLogic vptr (0x5e705c), inline-destruct the +0x18 link (the embedded
// ~EngStr call 0x16d2a0), store the CUserBase vptr (0x5e70b4). The destructible
// link forces the /GX EH frame.
// IMPLICIT dtor (retail is COMPILER-GENERATED - eh-dtor-vptr-restamp CAUSE B):
// a user-declared `~CLevelTime() {}` emits the leaf-vptr restamp, and the CWapX
// base EH state blocks the dead-store elision that used to hide it. The ??_G
// in the vtable-emitting TU forces the implicit ??1 COMDAT; pinned by name.
// @rva-symbol: ??1CLevelTime@@UAE@XZ 0x00011a50 0x44

// CLevelTime::SerializeMove (0x119b0), vtable slot 1 - the
// CSecretTeleporterTrigger::Serialize archetype (chain + +0x34 CSerialObjRef gate).
RVA(0x000119b0, 0x47)
i32 CLevelTime::SerializeMove(CGruntArchive* ar, i32 mode, i32 a3, i32 a4) {
    if (!CUserLogic::SerializeMove((CSerialArchive*)((i32)ar), mode, a3, a4)) {
        return 0;
    }
    return Chain((CSerialArchive*)ar, mode, a3, (CGameObject*)a4) != 0;
}

// CLevelTime::CLevelTime @0x9b8b0 - fold the shared CUserLogic(obj) init (with the
// built-in logic types inlined-registered), then flag the sub-object (+0x08 bit 1).
// @early-stop
// eh-ctor-vptr-restamp-position wall (docs/patterns/eh-ctor-vptr-restamp-position.md):
// body byte-identical (incl. the unrolled logic-type registration); residual is the
// /GX leaf-vptr re-stamp position + EH-state ids.
RVA(0x0009b8b0, 0x18f)
CLevelTime::CLevelTime(CGameObject* obj) : CUserLogic(obj), CWapX(obj) {
    m_38->m_flags |= 2;
}

// class-metadata SIZE sweep (misc-Gruntz A-C): SIZE(CLevelTime, 0x54) now lives in
// <Gruntz/LevelTimeDtor.h> (pinned by the StateDispatch new-site push 0x54).
