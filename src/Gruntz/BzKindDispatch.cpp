// BzKindDispatch.cpp - CLatencyList::Dispatch (0x37910), the connection-latency
// slot-list mode dispatcher (C:\Proj\Gruntz).
//
// Dispatch latches the incoming mode at +0x1c and routes modes 1..5 to the matching
// per-mode populator (Populate1..5 @ 0x37b40/c30/d20/e10/f00, reached via ILT thunks
// 0x3760/3008/41ce/1eab/1cc1 - verified), reporting whether the populator succeeded
// (else 0 for an out-of-range mode). Folded from the former CBzKindDispatch view onto
// the canonical CLatencyList (wave 3); same object CMultiStartDlg::BuildSlotList
// dispatches on. The populators are reloc-masked cross-unit calls (bodies elsewhere).
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
