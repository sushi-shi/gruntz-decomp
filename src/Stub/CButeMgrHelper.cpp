#include <rva.h>
#include <Win32.h>
// CButeMgrHelper.cpp - engine-label stubs for CButeMgrHelper (reloc-correlation).

void winapi_16c9d0_DeleteCriticalSection(LPCRITICAL_SECTION cs);
typedef void(__stdcall* CButeMgrHelper_ScalarDeletingDestructor)(int);

DATA(0x006bf400)
extern int g_func_b_ref;

DATA(0x006bf3c8)
extern CRITICAL_SECTION g_func_b_cs;

DATA(0x005f03bc)
extern CButeMgrHelper_ScalarDeletingDestructor g_func_b_destructor;

class CButeMgrHelper {
public:
    void FuncA();
    void FuncB();

    CButeMgrHelper_ScalarDeletingDestructor* m_0;
    int m_4;
    int m_8;
    char m_pad0c[0x10];
    int m_1c;
    char m_pad20[0x14];
    int m_34;
    int m_38;
};
// @confidence: med
// @source: reloc-correlation (1 caller)
// @stub
RVA(0x169be0, 0x13)
void CButeMgrHelper::FuncA() {}
// @confidence: med
// @source: reloc-correlation (1 caller)
// @stub
RVA(0x169d70, 0x5a)
void CButeMgrHelper::FuncB() {
    m_0 = &g_func_b_destructor;
    m_34 = 0xffffffff;
    if (--g_func_b_ref) {
        winapi_16c9d0_DeleteCriticalSection(&g_func_b_cs);
    }
    winapi_16c9d0_DeleteCriticalSection((LPCRITICAL_SECTION)m_38);

    if (!m_1c && !m_4) {
        (*m_0)(0x1);
    }

    m_4 = 0x0;
    m_8 = 0x4;
}
