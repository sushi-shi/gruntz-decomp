#include <rva.h>
// ReconBatch2O1.cpp - MFC-region leaf ctors from the engine_unmatched worklist
// that were compiled /O1 (favor SIZE). The size-favoring optimizer materializes
// `member = 0` as `and [mem],0` (4B) rather than `mov [mem],0` (7B) - the tell
// that this lives in an /O1 MFC TU, not the /O2 game TUs. Placeholder class shape;
// only offsets + code bytes are load-bearing.

// ===========================================================================
// 0x0011e4d0 (13B) - small object ctor: zero m_4, stamp the manual vtable
// (PTR_LAB_005ed0e4); returns this.
// ===========================================================================
DATA(0x001ed0e4)
extern void* g_vtbl_5ed0e4;
struct Obj_11e4d0 {
    void* m_vptr; // +0x00
    i32 m_4;      // +0x04
    Obj_11e4d0();
};
RVA(0x0011e4d0, 0xd)
Obj_11e4d0::Obj_11e4d0() {
    m_4 = 0;
    m_vptr = &g_vtbl_5ed0e4;
}

// ===========================================================================
// 0x001c0fa8 (9B) - bump the MFC module thread-state lock count at +0x10.
// Logic byte-faithful (call + ret exact); retail folds the store to
// `inc dword [eax+0x10]` (3B) but cl 5.0 here biases the pointer first
// (`add eax,0x10; inc [eax]`, 5B) for an increment through a call-returned
// pointer at a non-zero offset. Tried struct-member ++/post++/+=1, volatile,
// constant array-index, /O1 and /O2 - cl 5.0 will not fold the displacement on
// a fresh call result. Pointer-bias codegen wall.
// @early-stop
// cl 5.0 pointer-bias: `add eax,0x10; inc [eax]` vs retail `inc [eax+0x10]`,
// not source-steerable (6 spellings x 2 opt levels).
// ===========================================================================
extern "C" i32* AfxGetModuleThreadState();
RVA(0x001c0fa8, 0x9)
void Bump_1c0fa8() {
    AfxGetModuleThreadState()[4]++; // [eax + 4*4] = retail inc [eax+0x10]
}

// ===========================================================================
// 0x001bae9b (13B) - call a CWnd method on a global object with arg -2.
// (push -2; mov ecx,&g_652ec0; call <CWnd>; ret) __thiscall callee.
// ===========================================================================
struct CWndHost_1bae9b {
    i32 Method1baf15(i32 a); // 0x1baf15
};
DATA(0x00252ec0)
extern CWndHost_1bae9b g_wnd_652ec0;
RVA(0x001bae9b, 0xd)
void Call_1bae9b() {
    g_wnd_652ec0.Method1baf15(-2);
}

// ===========================================================================
// 0x001d31bc / 0x001d4bc1 (12B each) - compiler-style atexit registration
// thunks for a static object's cleanup (push &cleanup; call atexit; pop ecx).
// ===========================================================================
extern "C" int atexit(void(__cdecl* fn)());
extern "C" void Cleanup_1d31c8(); // 0x1d31c8 (mov ecx,&g_652f40; jmp ~Class)
extern "C" void Cleanup_1d4bcd(); // 0x1d4bcd
RVA(0x001d31bc, 0xc)
void AtexitReg_1d31bc() {
    atexit(Cleanup_1d31c8);
}
RVA(0x001d4bc1, 0xc)
void AtexitReg_1d4bc1() {
    atexit(Cleanup_1d4bcd);
}
