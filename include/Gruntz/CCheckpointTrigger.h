// CCheckpointTrigger.h - the checkpoint-trigger tile-logic object (C:\Proj\Gruntz),
// a CUserLogic leaf (vftables 0x5e705c / 0x5e70b4). Only the /GX leaf dtor is
// reconstructed here; offsets + code bytes are load-bearing.
#ifndef GRUNTZ_CCHECKPOINTTRIGGER_H
#define GRUNTZ_CCHECKPOINTTRIGGER_H

#include <rva.h>
#include <Gruntz/UserLogic.h>

class CCheckpointTrigger : public CUserLogic {
public:
    ~CCheckpointTrigger();    // 0x011480 (folds the CUserLogic teardown)
    static void InitActReg(); // 0x10ea00 (constructs g_checkpointActReg @0x64e7c0)
    char m_pad40[0x54 - 0x40];
};
SIZE(CCheckpointTrigger, 0x54);

#endif // GRUNTZ_CCHECKPOINTTRIGGER_H
