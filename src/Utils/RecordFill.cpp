// RecordFill.cpp - two leaf math/memory helpers.
//   0x17f500  ZeroRecords  (__stdcall) - inline-memset `count` 0x28-byte records.
//   0x18c1f5  IntegerScale - x87 whole-number classifier (naked: bare x87 with a
//             ds:-relative double constant; no C++ source produces the cl-built
//             0/1/2 result with this exact fcomp/fcompp schedule).
#include <rva.h>
#include <string.h>

// ---------------------------------------------------------------------------
// 0x17f500 - zero `count` consecutive 0x28-byte records at `dst`.
// ---------------------------------------------------------------------------
RVA(0x0017f500, 0x23)
void __stdcall ZeroRecords(void* dst, int count) {
    memset(dst, 0, count * 0x28);
}

// The ds:-relative scale constant (VA 0x6256f0) the classifier multiplies by.
extern double g_scale6256f0;
extern double g_scale6256f0;

// ---------------------------------------------------------------------------
// 0x18c1f5 - classify the x87 top value: 0 if not whole; else 1, or 2 when it
// stays whole after one scale. Result is built in cl (verbatim).
// ---------------------------------------------------------------------------
RVA(0x0018c1f5, 0x28)
__declspec(naked) void IntegerScale() {
    __asm {
        fld st(0)
        frndint
        fcomp st(1)
        mov cl, 0
        fstsw ax
        sahf
        jne notwhole
        fmul qword ptr [g_scale6256f0]
        inc cl
        fld st(0)
        frndint
        fcompp
        fstsw ax
        sahf
        jne done
        inc cl
    done:
        ret
    notwhole:
        fstp st(0)
        ret
    }
}
