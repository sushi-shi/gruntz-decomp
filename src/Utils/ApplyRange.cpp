// ApplyRange.cpp - a generic "construct/visit each element of a contiguous
// array" helper (0x007c20), the engine's hand-rolled ConstructElements idiom.
//
// Reached only through the incremental-link thunk at 0x001aa5, from the big
// init routine at 0x0c7ec0 (the WinAPI-stub region): it sweeps an array of
// `count` fixed-stride elements, invoking a __thiscall callback on each in turn
// (mov ecx,item; call ptr; item += stride). __stdcall, 4 args, callee-cleans 16
// bytes (ret 0x10). Self-contained (no relocs); modeled with a pointer-to-
// member-function so `mov ecx,item; call <reg>` falls out with no stack cleanup.
#include <Ints.h>
#include <rva.h>

// The callback is a __thiscall member function (mov ecx,item; call reg). Model
// it as a pointer-to-member of a COMPLETE single-inheritance class so the PMF is
// the simple 4-byte code-pointer form (a plain `call`), not the general thunk.
class ApplyElem {};
typedef void (ApplyElem::*ApplyFn)();

// __stdcall void ApplyToRange(base, stride, count, fn):
//   if (count > 0) do { (item->*fn)(); item += stride; } while (--count);
// The guard is `dec count; js skip` (count-1 < 0), then a do-while over `count`
// iterations with the running pointer in esi and the live count in edi.
RVA(0x00007c20, 0x2a)
void __stdcall ApplyToRange(ApplyElem* base, i32 stride, i32 count, ApplyFn fn) {
    if (--count >= 0) {
        char* item = (char*)base;
        i32 i = count + 1;
        do {
            (((ApplyElem*)item)->*fn)();
            item += stride;
        } while (--i);
    }
}
