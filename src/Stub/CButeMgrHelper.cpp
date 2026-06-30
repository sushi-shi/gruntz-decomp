#include <rva.h>
#include <Win32.h>
// CButeMgrHelper.cpp - engine-label stubs for CButeMgrHelper (reloc-correlation).

void winapi_16c9d0_DeleteCriticalSection(LPCRITICAL_SECTION cs);
typedef void(__stdcall* CButeMgrHelper_ScalarDeletingDestructor)(i32);

DATA(0x006bf400)
extern i32 g_func_b_ref;

DATA(0x006bf3c8)
extern CRITICAL_SECTION g_func_b_cs;

DATA(0x005f03bc)
extern CButeMgrHelper_ScalarDeletingDestructor g_func_b_destructor;

// The child object at +0x4 destroyed in FuncB through its slot-0 scalar-deleting
// dtor (`mov edx,[m_4]; push 1; call [edx]`). Modeled polymorphic so the virtual
// dispatch falls out at slot 0.
struct CButeMgrChild {
    virtual void ScalarDtor(i32 flag); // slot 0
};

class CButeMgrHelper {
public:
    void FuncB();

    CButeMgrHelper_ScalarDeletingDestructor* m_0;
    CButeMgrChild* m_4;
    i32 m_8;
    char m_pad0c[0x10];
    i32 m_1c;
    char m_pad20[0x14];
    i32 m_34;
    i32 m_38; // embedded CRITICAL_SECTION head (DeleteCriticalSection(&m_38))
};
// FuncA (0x169be0): the virtual-base-class displacement adjustor `vtordisp` thunk
// MSVC auto-generates for a vbase ctor: load the vbtable handle at [this-0x14], read
// the +4 displacement, stamp the most-derived vtable (0x5f0394) at that displaced
// slot, then tail-jmp the real ctor body (0x16c950). C++ can't express the vbase
// adjustor directly, so it is emitted as a __declspec(naked) thunk; RVA-keyed pairing
// absorbs the FuncA-vs-?vtordisp name mismatch and the two reloc operands mask.
DATA(0x005f0394)
extern void* g_butemgrhelper_vtbl_5f0394;
extern "C" void CButeMgrHelper_VbaseCtorBody(); // 0x16c950 (jmp target)

// A free naked thunk (clang rejects `naked` on member functions; the body is the
// raw vbase-adjustor, `this` arrives in ecx as for the original __thiscall thunk).
RVA(0x00169be0, 0x13)
__declspec(naked) void CButeMgrHelper_FuncA() {
    __asm {
        mov eax, [ecx - 0x14]
        mov edx, [eax + 4]
        mov dword ptr [edx + ecx - 0x14], offset g_butemgrhelper_vtbl_5f0394
        jmp CButeMgrHelper_VbaseCtorBody
    }
}
// @confidence: med
// @source: reloc-correlation (1 caller)
RVA(0x00169d70, 0x5a)
void CButeMgrHelper::FuncB() {
    m_0 = &g_func_b_destructor;
    m_34 = 0xffffffff;
    if (--g_func_b_ref == 0) {
        winapi_16c9d0_DeleteCriticalSection(&g_func_b_cs);
    }
    winapi_16c9d0_DeleteCriticalSection((LPCRITICAL_SECTION)&m_38);

    if (m_1c && m_4) {
        m_4->ScalarDtor(0x1);
    }

    m_4 = 0x0;
    m_8 = 0x4;
}
