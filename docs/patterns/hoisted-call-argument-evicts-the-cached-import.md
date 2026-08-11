# A hoisted call ARGUMENT is a register, and the register it takes is the one holding the cached import

**Tags:** `cpp:branch` `cpp:local` `cpp:call` | `asm:push` `asm:jmp` `asm:call` | `topic:codegen-idiom` `topic:regalloc`
**Confidence:** 10/10 (one edit, 93.56 -> 100.00 EXACT, and it moved all three symptoms at once)

## Symptom

`reloc_multiset` reports a Win32 import referenced **once more** on the base side than
on the target side, in a loop that calls it twice:

    __imp__ScreenToClient@8      base  2  target  1

and the masked diff shows retail caching the import in a callee-saved register while
we rematerialise it at both sites:

    retail   mov ebx,<__imp__ScreenToClient>   ...   call ebx      call ebx
    base                                             call [<imp>]  call [<imp>]

That row is NOT an independent defect and there is no spelling of the *call* that
fixes it. It is a **readout of register pressure**: one register too few, and the
import is the value cl chooses to spill.

## What actually took the register

Look further down the same function for a two-armed `if/else` whose arms each compute
one value and a call after the join that consumes it. Retail:

    or eax,edx          <- enabled arm computes the colour
    push eax
    jmp <join>
    push 0x808080       <- disabled arm
  <join>:
    call [__imp__CreateSolidBrush]

Retail pushes the argument **inside each arm** and cross-jumps only the shared `call`.
That is the two-arm-CALL shape (cf. [`nrv-return-repeated-in-every-arm`](nrv-return-repeated-in-every-arm.md)),
and the source that produces it is **two call sites**, not one:

```cpp
if (it->IsWindowEnabled()) {
    ...
    scratch.Attach(CreateSolidBrush((v << 8 | v) << 8 | v));
} else {
    scratch.Attach(CreateSolidBrush(0x808080));
}
```

Our source had hoisted the argument into a local:

```cpp
i32 color;                       // <- this
if (...) { ...; color = ...; } else { color = 0x808080; }
scratch.Attach(CreateSolidBrush(color));
```

`color` is live across the join, so it wants a register for the whole if/else. cl paid
for it by parking the constant **0** in `ebx` instead - which is then visible in two
more places, both of which look like separate scheduling noise:

    base    cmp esi,ebx                  base    mov [esp+0x18],ebx
    retail  test esi,esi                 retail  mov [esp+0x18],0x0

and by evicting `__imp__ScreenToClient` from the register retail keeps it in.

## Rule

**Three symptoms, one cause.** When you see (a) a duplicated `__imp__` reference,
(b) a zero compared out of a register where retail uses `test`, and (c) a zero STORE
from a register where retail stores an immediate - do not chase any of them. Find the
local that is live across a join and whose only consumer is one call, and give the call
its own site in each arm. All three fall out together.

Read the direction off retail: the `push` sitting **before** the `jmp` to the join is
the whole tell. A hoisted local pushes after the join.

## Measured

`CBattlezDlg::FlashCtrlD` 0x160f0 (`dialogs`), 581 B: **93.56 -> 100.00 EXACT** on the
one edit. `?ScreenToClient@8` 2 -> 1, `cmp esi,ebx` -> `test esi,esi`, and the two
`call [imp]` -> `mov ebx,<imp>` + `call ebx` twice, all in the same build.

The function-pointer locals it already had (`BOOL (WINAPI *stc)(HWND,LPPOINT) = ::ScreenToClient;`)
were NOT the problem and did not need changing - cl simply had no register left for
the second one.

## Related

- [`nrv-return-repeated-in-every-arm`](nrv-return-repeated-in-every-arm.md) - the same
  two-arm-CALL shape applied to an implicit copy-construction.
- [`mfc-wrapper-vs-free-api-shows-in-arg-order`](mfc-wrapper-vs-free-api-shows-in-arg-order.md) -
  the other reason a lone `call *__imp__` survives (an unconverted call site).
- [`store-hoisted-out-of-both-arms-costs-the-zero-eax-return`](store-hoisted-out-of-both-arms-costs-the-zero-eax-return.md) -
  the same hoist, costing a return value instead of a register.
