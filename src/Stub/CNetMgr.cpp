#include <rva.h>
// CNetMgr.cpp - engine-label stubs for CNetMgr (reloc-correlation).

// The five 5-byte ILT `jmp` thunks (AckDropPlayer/ReportVersionMsg/SendNetStat/
// SendStat3/SendStatFlag) that used to live here are reconstructed as naked-asm
// thunks in src/Net/NetThunks.cpp (their stub names collided with the real,
// already-matched bodies in NetMgr.cpp / CMulti.cpp).

class CNetMgr {
public:
    i32 winapi_0bb700_GetAsyncKeyState_Sleep_timeGetTime_wsprintfA();
    i32 winapi_0bba10_Sleep(i32);
    void LoadMenuSelectSprite(i32);
};

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
