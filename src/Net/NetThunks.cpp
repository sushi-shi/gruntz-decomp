// NetThunks.cpp - the linker's incremental-link (ILT) `jmp rel32` thunks for the
// already-matched CNetMgr / CMulti net methods. Retail routes some call sites
// through a 5-byte `E9 rel32` thunk in the low-RVA ILT band; each lands on the
// real body that lives (byte-exact) in NetMgr.cpp / CMulti.cpp. There is no
// plain-C++ source form for a bare unframed `jmp`, so each is transcribed as a
// __declspec(naked) body whose single jmp's rel32 reloc-masks against the named
// target (same sanctioned approach as Wap32/M3CompilerThunks.cpp).
//
// NB: previously these lived in src/Stub/CNetMgr.cpp under the SAME mangled names
// as the real bodies (?AckDropPlayer@CNetMgr@@QAEXH@Z etc.), a symbol-name
// collision across two units. Giving the thunk its own name removes that.
#include <rva.h>

extern "C" {
void n_AckDropPlayer_ba590();    // CNetMgr::AckDropPlayer  (0xba590)
void n_ReportVersionMsg_b7e30(); // CMulti::ReportVersionMsg (0xb7e30)
void n_SendNetStat_b9290();      // CNetMgr::SendNetStat     (0xb9290)
void n_SendStatFlag_b9240();     // CNetMgr::SendStatFlag    (0xb9240)
void n_SendStat3_b9410();        // CNetMgr::SendStat3       (0xb9410)
}

RVA(0x000016d1, 0x5)
__declspec(naked) void Ilt_AckDropPlayer_16d1() {
    __asm { jmp n_AckDropPlayer_ba590 }
}

RVA(0x00001af0, 0x5)
__declspec(naked) void Ilt_ReportVersionMsg_1af0() {
    __asm { jmp n_ReportVersionMsg_b7e30 }
}

RVA(0x00002955, 0x5)
__declspec(naked) void Ilt_SendNetStat_2955() {
    __asm { jmp n_SendNetStat_b9290 }
}

RVA(0x00002e82, 0x5)
__declspec(naked) void Ilt_SendStatFlag_2e82() {
    __asm { jmp n_SendStatFlag_b9240 }
}

RVA(0x00003d0a, 0x5)
__declspec(naked) void Ilt_SendStat3_3d0a() {
    __asm { jmp n_SendStat3_b9410 }
}
