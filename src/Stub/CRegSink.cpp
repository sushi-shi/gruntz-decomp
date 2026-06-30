#include <rva.h>
// CRegSink.cpp - engine-label stubs for CRegSink (reloc-correlation).
//
// The 5-byte body at 0x3616 (rtti-labeled CRegSink::Post) is a low-RVA ILT
// `jmp rel32` thunk landing on 0x7c2e0 (CTriggerMgr::CycleMoveIcons, reconstructed
// in src/Gruntz/TriggerMgr.cpp). There is no plain-C++ source form for a bare
// unframed `jmp`, so it is transcribed as a __declspec(naked) body whose single
// jmp's rel32 reloc-masks against the named target (same approach as NetThunks.cpp).
extern "C" void n_CycleMoveIcons_7c2e0(); // 0x7c2e0 (jmp target)

RVA(0x00003616, 0x5)
__declspec(naked) void Ilt_RegSinkPost_3616() {
    __asm { jmp n_CycleMoveIcons_7c2e0 }
}
