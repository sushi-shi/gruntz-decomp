// GetRetAddr.cpp - the shared coordinate-registry breadcrumb helper (0x16d990).
//
// A 3-byte leaf (`pop eax; push eax; ret`) that returns the caller's call-site
// return address, which the activation-registry resolve paths stamp into
// g_retAddrBreadcrumb (0x6bf428) right before the slow rebuild's Insert. It is
// called ~100x across the whole logic-leaf family (every FireActivation / RunAct /
// Dispatch that reaches the slow coordinate lookup), so it is a real, shared,
// standalone .text function - NOT the per-TU `extern void* GetRetAddr()` fake it
// was modeled as. Its real home is the typekeycoll .text band (it sits between
// CVariantSlot::Set @0x16d850 and _zvec::GrowTo @0x16da80), but that TU is owned
// elsewhere, so the single bound definition lives here; the callers' rel32 to it
// now resolve to 0x16d990 (was UNBOUND). See <Gruntz/ActColl.h>.
#include <rva.h>

// pop the return address into eax and push it back (esp unchanged), then ret: the
// value returned is the caller's return address. Naked + inline asm is the only
// way to reproduce the exact `58 50 c3` (an intrinsic emits `mov eax,[esp]`).
RVA(0x0016d990, 0x3)
__declspec(naked) void* GetRetAddr() {
    __asm {
        pop  eax
        push eax
        ret
    }
}
