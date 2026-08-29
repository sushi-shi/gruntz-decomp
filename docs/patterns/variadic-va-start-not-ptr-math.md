# Variadic `...` function: use `<stdarg.h>` `va_list`/`va_start`/`va_end`, not `(char*)(&lastarg+1)`
tags: cpp:vararg cpp:cast | asm:lea | topic:codegen-idiom
symptoms: reconstructing a `...` variadic function; you reached for `(char*)(&lastarg + 1)` pointer math to get the arg pointer to hand to `vsprintf`/`vfprintf`/an engine va-callee
confidence: 8/10

Recover a variadic function with the REAL C varargs macros, not a hand-rolled reinterpret cast:

```cpp
void CButeMgr::DisplayMessage(const char* fmt, ...) {
    va_list args;
    va_start(args, fmt);
    vsprintf(m_sErrorString.GetBuffer(0x100), fmt, args);
    m_sErrorString.ReleaseBuffer(-1);
    ...
    va_end(args);
}
```

`va_start(args, fmt)` lowers to the same `lea reg, [fmt+4]` arg-pointer setup as `(char*)(&fmt + 1)`, and `va_end` is a no-op on x86 — so this is **matching-NEUTRAL**, AND it is the devs' real shape (avoids a placeholder reinterpret cast → matcher doctrine #1 "model the real type, almost never cast"). Declare the engine va-callee with its real trailing `va_list`/`char*` param (e.g. `extern "C" int vsprintf(char*, const char*, va_list);`) so the push reloc-masks. `<stdarg.h>` comes in via `<Mfc.h>`/the class header umbrella; don't re-typedef `va_list`.

STEERABLE (matching-neutral). Evidence: `CButeMgr::DisplayMessage` @0x1706c0
(`?DisplayMessage@CButeMgr@@QAAXPBDZZ`, a variadic member) — swapping
`(char*)(&fmt+1)` for `va_start` preserved the generated argument setup; the
remaining residue at the time was in callee relocation typing, not varargs.
related: external-nobody-callee.md, win32-import-decl-stdcall.md, inline-mem-ops-rep-movs.md.
