#include <rva.h>
// CNetMgr.cpp - engine-label stubs for CNetMgr (reloc-correlation).

class CNetMgr {
public:
    void AckDropPlayer(i32);
    void ReportVersionMsg(char const*, i32);
    void SendNetStat(i32, u32, i32);
    void SendStat3(i32, u32, i32);
    void SendStatFlag(i32, i32);
    i32 winapi_0bb700_GetAsyncKeyState_Sleep_timeGetTime_wsprintfA();
    i32 winapi_0bba10_Sleep(i32);
    void LoadMenuSelectSprite(i32);
};
// @confidence: med
// @source: reloc-correlation (1 caller)
// @stub
RVA(0x000016d1, 0x5)
void CNetMgr::AckDropPlayer(i32) {}
// @confidence: med
// @source: reloc-correlation (1 caller)
// @stub
RVA(0x00001af0, 0x5)
void CNetMgr::ReportVersionMsg(char const*, i32) {}
// @confidence: high
// @source: reloc-correlation (2 callers)
// @stub
RVA(0x00002955, 0x5)
void CNetMgr::SendNetStat(i32, u32, i32) {}
// @confidence: med
// @source: reloc-correlation (1 caller)
// @stub
RVA(0x00003d0a, 0x5)
void CNetMgr::SendStat3(i32, u32, i32) {}
// @confidence: med
// @source: reloc-correlation (1 caller)
// @stub
RVA(0x00002e82, 0x5)
void CNetMgr::SendStatFlag(i32, i32) {}

// Proximity-attributed (HIGH, both-sides RVA bracket) - re-homed from
// src/Stub/ApiCallers.cpp (was ThisStubOwnerUnknown).
// @confidence: low
// @source: winapi:GetAsyncKeyState;Sleep;timeGetTime;wsprintfA
// @stub
RVA(0x000bb700, 0x265)
i32 CNetMgr::winapi_0bb700_GetAsyncKeyState_Sleep_timeGetTime_wsprintfA() {
    return 0;
}
// @confidence: low
// @source: winapi:Sleep
// @stub
RVA(0x000bba10, 0x1fb)
i32 CNetMgr::winapi_0bba10_Sleep(i32) {
    return 0;
}
// @confidence: med
// @source: decomp-xref
// @stub
RVA(0x000ba620, 0x14a)
void CNetMgr::LoadMenuSelectSprite(i32) {}
