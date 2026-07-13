// CheckpointTrigger.h - the checkpoint-trigger tile-logic object (C:\Proj\Gruntz),
// a CUserLogic leaf (vftables 0x5e705c / 0x5e70b4). Only the /GX leaf dtor is
// reconstructed here; offsets + code bytes are load-bearing.
#ifndef GRUNTZ_CCHECKPOINTTRIGGER_H
#define GRUNTZ_CCHECKPOINTTRIGGER_H

#include <rva.h>
#include <Gruntz/UserLogic.h>

class CCheckpointTrigger : public CUserLogic {
public:
    virtual i32 SerializeMove(CGruntArchive*, i32, i32, i32) OVERRIDE; // slot 1
    virtual LogicTypeId GetTypeTag() OVERRIDE;                         // slot 2
    virtual i32 UserLogicVfunc2() OVERRIDE;                            // slot 4
    TILE_LOGIC_TAIL
public:
    CCheckpointTrigger(CGameObject* obj);   // 0x10ee20 (1-arg leaf ctor)
    virtual ~CCheckpointTrigger() OVERRIDE; // 0x011480 (folds the CUserLogic teardown)
    static void InitActReg();               // 0x10ea00 (constructs g_checkpointActReg @0x64e7c0)
    void FireActivation(i32 coord);         // 0x10ea80 (vtable slot 4 body; in ActRegSiblings.cpp)
    static void
    RegisterActs(); // 0x10ebe0 (binds the "A" activation handler; in ActRegSiblings.cpp)
    i32 Trigger();  // 0x10ede0 (the activation handler; declared-only, used as a PMF)

    char m_pad40[0x54 - 0x40]; // +0x40
    i32 m_state[15];           // +0x54  the captured checkpoint state (15 dwords)
    i32 m_firstEmpty;          // +0x90  first-empty index
};
VTBL(CCheckpointTrigger, 0x1e7ebc);
SIZE(CCheckpointTrigger, 0x94);

#endif // GRUNTZ_CCHECKPOINTTRIGGER_H
