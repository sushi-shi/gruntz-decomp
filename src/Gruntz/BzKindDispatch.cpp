// BzKindDispatch.cpp - a battlez-setup kind dispatcher (C:\Proj\Gruntz).
//
// Dispatch @0x37910 stores the incoming kind at +0x1c and routes kinds 1..5 to the
// matching per-kind handler, reporting whether the handler succeeded (else 0 for an
// out-of-range kind). The handlers are modeled NO-body so their calls reloc-mask.
// Class name + handler names are placeholders; only OFFSETS + code bytes matter.
#include <rva.h>

class CBzKindDispatch {
public:
    i32 Dispatch(i32 kind); // 0x37910
    i32 Handler1();         // 0x3760 (ILT thunk)
    i32 Handler2();         // 0x3008
    i32 Handler3();         // 0x41ce
    i32 Handler4();         // 0x1eab
    i32 Handler5();         // 0x1cc1
    char m_pad0[0x1c];
    i32 m_1c; // +0x1c  latched kind
};

// @early-stop
// 79%: the CODE is byte-exact (confirmed llvm-objdump base vs target). The residual
// is the dense-switch jump-table layout: MSVC emits the table as a separate `$L209`
// symbol after the function (+ a 2-byte `8b ff` align filler), but the delinker
// inlines the table into the Dispatch symbol at +0x5c (+ `90 90` filler), so the
// jmp's table reloc + the table data don't pair. See docs/patterns/switch-jumptable-
// separate-comdat.md - a delinker artifact, not source-fixable.
RVA(0x00037910, 0x5a)
i32 CBzKindDispatch::Dispatch(i32 kind) {
    m_1c = kind;
    switch (kind) {
        case 1:
            if (Handler1()) {
                break;
            }
            return 0;
        case 2:
            if (Handler2()) {
                break;
            }
            return 0;
        case 3:
            if (Handler3()) {
                break;
            }
            return 0;
        case 4:
            if (Handler4()) {
                break;
            }
            return 0;
        case 5:
            if (Handler5()) {
                break;
            }
            return 0;
        default:
            return 0;
    }
    return 1;
}

// class-metadata SIZE sweep (misc-Gruntz A-C): matching-neutral, hosted at
// .cpp EOF (see docs/class-metadata-sweep-log.md). SIZE_UNKNOWN = size not yet pinned.
SIZE_UNKNOWN(CBzKindDispatch);
