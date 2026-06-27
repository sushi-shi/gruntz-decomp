# MSVC5 won't value-init `T()` for a POD — give the temp an explicit zeroing ctor
tags: cpp:ctor cpp:local cpp:new | asm:xor asm:mov | topic:codegen-idiom

When a function passes a default/zero value-type temporary by value (the classic
"call a worker with N default parameter blocks" forwarder), the natural C++98
spelling `Worker(..., T(), T(), ...)` relies on `T()` *value-initializing* a POD
to all-zero. **MSVC 5.0 predates that rule and does NOT zero `T()` for a POD** — it
leaves the temp uninitialized, so the recompile *copies garbage* (whatever the
adjacent stack/arg slots hold) into the param blocks instead of retail's `xor`
+ zero-stores. The fix is to give the value type an **explicit default ctor that
zeroes every member**; then `T()` runs the ctor (zeroing in place in the arg slot)
and the optimizer reproduces retail's shared zero-register stores
(`xor eax,eax; … mov [ebx],eax; mov [ebx+4],edx; …`).

```cpp
// WRONG (VC5): T() is NOT value-initialized -> temps hold garbage, recompile
// copies args/stack into the blocks (CTileTriggerWiring::AddLogicDefaults 27%).
struct CTrigParam { i32 m0, m4, m8, mc; };
Worker(a1, a2, CTrigParam(), CTrigParam(), /*…*/, a6);

// RIGHT: explicit zeroing ctor -> each T() temp is zeroed in place (76%; the
// residual is a separate regalloc choice).
struct CTrigParam {
    i32 m0, m4, m8, mc;
    CTrigParam() : m0(0), m4(0), m8(0), mc(0) {}
};
```

Steerable (`topic:codegen-idiom`): the explicit ctor is required, not optional, on
VC5 — a no-ctor POD aggregate diverges hard. Evidence: CTileTriggerWiring::
AddLogicDefaults (0x1163b0) — six zeroed 16-byte param blocks forwarded to the
0x116610 factory, 27.85% (no-ctor POD, garbage copy) -> 75.88% (zeroing ctor,
correct `xor`-shared zero stores). Aggregate-init named locals (`T z = {0,0,0,0};`)
also zero but get copied into the slots (double work) rather than constructed in
place, so prefer the rvalue `T()` + zeroing ctor for an in-place build.
