// CCheckpointTrigger.h - the checkpoint-trigger tile-logic object (C:\Proj\Gruntz),
// a CUserLogic leaf (vftables 0x5e705c / 0x5e70b4). Only the /GX leaf dtor is
// reconstructed here; offsets + code bytes are load-bearing.
#ifndef GRUNTZ_CCHECKPOINTTRIGGER_H
#define GRUNTZ_CCHECKPOINTTRIGGER_H

#include <rva.h>
#include <Gruntz/UserLogic.h>

class CCheckpointTrigger : public CUserLogic {
public:
    CCheckpointTrigger(CGameObject* obj); // 0x10ee20 (1-arg leaf ctor)
    ~CCheckpointTrigger();                // 0x011480 (folds the CUserLogic teardown)
    static void InitActReg();             // 0x10ea00 (constructs g_checkpointActReg @0x64e7c0)

    char m_pad40[0x54 - 0x40]; // +0x40
    i32 m_54[15];              // +0x54  the captured checkpoint state (15 dwords)
    i32 m_90;                  // +0x90  first-empty index
};
SIZE(CCheckpointTrigger, 0x94);

#endif // GRUNTZ_CCHECKPOINTTRIGGER_H
