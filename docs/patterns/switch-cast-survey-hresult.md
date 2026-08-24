# `case static_cast<i32>(HRESULT)` is correct — do not hoist the cast to the switch

tags: cpp:cast cpp:switch | topic:tooling topic:measured-negative
confidence: 10/10 (measured, reverted)

## The temptation

`DinMgr2.cpp`, `DirectDrawMgr.cpp` and `SoundStream.cpp` carry **74** case labels of the form

```cpp
switch (hr) {
    case static_cast<i32>(DIERR_UNSUPPORTED):
    case static_cast<i32>(0x80004001):
```

which reads like 74 removable casts, replaceable by one at the switch.

## Why it is not

1. **The parameter type is right.** Retail's mangling for the DirectSound one is
   `?GetErrorString@SoundBuffer@@SAXPADHH@Z` — `PAD`, `H`, `H`. `hr` is `int`.
   Widening it would change the mangled name and break the RVA binding.
2. **The constants are a MIX.** Some are `>INT_MAX` literals (`0x80004001`); others come
   from `MAKE_HRESULT`, which expands to a **signed negative** value.
3. **So `switch (static_cast<u32>(hr))` cannot hold both.** It compiles under cl 5.0 and is
   byte-neutral (dinmgr2+directdrawmgr+soundstream mean 89.1885 before and after), but
   **clang rejects it**:

   ```
   error: case value evaluates to -2005401450, which cannot be narrowed to
          type 'u32' (aka 'unsigned int') [-Wc++11-narrowing]
   note: expanded from macro 'MAKE_HRESULT'
   ```

   clang runs the label pass, so `gen_labels` emits no IR and **every function in those
   three TUs silently vanishes from the label CSV** — the build fails at
   `[labels] ERROR ... label pass produced NO IR`.

## Rule

The per-label `static_cast<i32>` is the only spelling both compilers accept. Leave it.
A cast that survives in a `case` label is not automatically a modelling defect — check
whether the switch variable's type is retail's before assuming the cast is removable.

Tried and reverted 2026-08-07 (`58b23518a`, reverted by the following commit).
Related: `docs/cast-metric-policy.md` (named casts are the approved form).
