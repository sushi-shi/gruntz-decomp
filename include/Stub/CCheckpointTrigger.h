#ifndef GRUNTZ_STUB_CCHECKPOINTTRIGGER_H
#define GRUNTZ_STUB_CCHECKPOINTTRIGGER_H
#include <rva.h>
#include <Stub/CUserLogic.h>
// CCheckpointTrigger : CUserLogic (RTTI). sizeof 0x94.
class CCheckpointTrigger : public CUserLogic {
public:
    CCheckpointTrigger(i32);
    static void InitActReg();   // 0x10ea00 (constructs g_checkpointActReg @0x64e7c0)
    static void RegisterActs(); // 0x10ebe0 (binds the "A" handler)
    i32 Trigger();              // 0x10ede0 (the trivial activation handler)
    char m_size_pad[0x54];      // own region over CUserLogic (0x40)
};
SIZE(CCheckpointTrigger, 0x94);
#endif
