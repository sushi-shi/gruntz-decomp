// M3MfcLeaves.cpp - two tiny leaf accessors from the MFC address run. Both are
// self-contained except for one reloc-masked extern; best-guess owners, only the
// member offset / external call are load-bearing.
#include <Ints.h>
#include <rva.h>

// 0x1ce95f - return the +0x30 field of the current AFX module state.
void* AfxGetModuleState(); // 0x1d3631 (MFC, external, reloc-masked)

RVA(0x001ce95f, 0x9)
i32 ModuleStateField30() {
    return *(i32*)((char*)AfxGetModuleState() + 0x30);
}

// 0x1d047f - plain +0x98 getter (thiscall).
struct CDdeView {
    char m_pad0[0x98];
    i32 m_98;        // +0x98
    i32 GetActive(); // 0x1d047f
};

RVA(0x001d047f, 0x7)
i32 CDdeView::GetActive() {
    return m_98;
}
