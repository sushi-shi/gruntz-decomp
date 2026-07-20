#include <Net/LatencyList.h>
#include <rva.h>

// @early-stop
// 79%: the CODE is byte-exact (confirmed llvm-objdump base vs target). The residual
// is the dense-switch jump-table layout: MSVC emits the table as a separate `$L209`
// symbol after the function (+ a 2-byte `8b ff` align filler), but the delinker
// inlines the table into the Dispatch symbol at +0x5c (+ `90 90` filler), so the
// jmp's table reloc + the table data don't pair. See docs/patterns/switch-jumptable-
// separate-comdat.md - a delinker artifact, not source-fixable.
RVA(0x00037910, 0x5a)
i32 CLatencyList::Dispatch(i32 mode) {
    m_mode = mode;
    switch (mode) {
        case 1:
            if (Populate1()) {
                break;
            }
            return 0;
        case 2:
            if (Populate2()) {
                break;
            }
            return 0;
        case 3:
            if (Populate3()) {
                break;
            }
            return 0;
        case 4:
            if (Populate4()) {
                break;
            }
            return 0;
        case 5:
            if (Populate5()) {
                break;
            }
            return 0;
        default:
            return 0;
    }
    return 1;
}
