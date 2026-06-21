# `__thiscall` that ignores `this` vs `static __stdcall`: identical callee bytes — decide by the CALLER's `mov ecx`
tags: cpp:method cpp:static asm:mov | topic:codegen-idiom
symptoms: a member fn whose body never reads `this`/ecx (all args off stack, `ret N`); declaring it static drops a caller-side `mov ecx,this` and regresses every caller
confidence: 8/10

A `__thiscall` member that never touches `this`/ecx produces callee bytes IDENTICAL to a
`static __stdcall` (same `ret N`, args all off the stack). The ONLY difference is on the CALLER
side: callers of the thiscall emit a redundant `mov ecx,this` before each call. So decide by the
caller, not the callee — if callers set ecx, keep it a (this-ignoring) thiscall member; declaring
it static drops the `mov ecx` and regresses every caller.

STEERABLE. Evidence: RegistryHelper::GetRegistryKey (@0x139650) is `?…@@QAEH…` thiscall (ret 0xc)
even though its body never reads `this` — declaring it static regressed callers. Contrast
geterrorstring-static-cdecl.md (the DX/Net error formatters ARE genuinely static __cdecl — there
the call site is `push×3; call; add esp,0xc`, no `mov ecx`). related: ret-n-calling-convention.md.
