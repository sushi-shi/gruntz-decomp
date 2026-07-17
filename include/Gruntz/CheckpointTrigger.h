// CheckpointTrigger.h - the checkpoint-trigger tile-logic object (C:\Proj\Gruntz),
// a CUserLogic leaf (vftables 0x5e705c / 0x5e70b4). Only the /GX leaf dtor is
// reconstructed here; offsets + code bytes are load-bearing.
#ifndef GRUNTZ_CCHECKPOINTTRIGGER_H
#define GRUNTZ_CCHECKPOINTTRIGGER_H

#include <rva.h>
#include <Gruntz/UserLogic.h>

class CCheckpointTrigger : public CUserLogic, public CWapX {
public:
    virtual i32 SerializeMove(CGruntArchive*, i32, i32, i32) OVERRIDE; // slot 1
    virtual LogicTypeId GetTypeTag() OVERRIDE;                         // slot 2
public:
    CCheckpointTrigger(CGameObject* obj);   // 0x10ee20 (1-arg leaf ctor)
    // NO user-declared dtor: retail's is COMPILER-GENERATED (implicit
    // elides the leaf-vptr restamp; @rva-symbol pin in the home TU).
    static void InitActReg();               // 0x10ea00 (constructs g_checkpointActReg @0x64e7c0)
    virtual void FireActivation(i32 id)
        OVERRIDE; // 0x10ea80 (vtable slot 4 body; in ActRegSiblings.cpp)
    static void
    RegisterActs(); // 0x10ebe0 (binds the "A" activation handler; in ActRegSiblings.cpp)
    i32 Trigger();  // 0x10ede0 (the activation handler; declared-only, used as a PMF)
    i32 m_state[15];           // +0x54  the captured checkpoint state (15 dwords)
    i32 m_firstEmpty;          // +0x90  first-empty index
};
VTBL(CCheckpointTrigger, 0x1e7ebc);
SIZE(CCheckpointTrigger, 0x94);

// The activation-registry entry record (the .data CActReg row; 4-byte PMF).
struct CCheckpointActEntry {
    i32 (CUserLogic::*m_fn)();
};

#endif // GRUNTZ_CCHECKPOINTTRIGGER_H
