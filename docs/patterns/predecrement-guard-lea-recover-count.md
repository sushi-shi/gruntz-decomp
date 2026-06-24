# Counted-loop entry guard: `if (--count >= 0)` emits `dec eax; js` + `lea edi,[eax+1]` recover

tags: cpp:loop cpp:branch cpp:local | asm:dec asm:js asm:lea | topic:codegen-idiom

symptoms: a counted `do-while` loop over `count` iterations whose retail prologue
reads the count once, then guards with a bare `dec eax; js skip` (testing
`count-1 < 0`, i.e. `count <= 0`) BEFORE saving any callee-saved registers, and
AFTER the saves rematerializes the live loop counter with `lea edi,[eax+1]`
(eax still holds `count-1`). A natural `if (count > 0) do {...} while(--count)`
gives `test;jle` (not `dec;js`) and the wrong counter register; a
`for (i=count; i; i--)` gets the body+regs right but pushes a register before the
guard (no pre-save guard); `if (count-- >= 1)` keeps the original count alive in a
second reg and emits `mov ecx,eax; cmp ecx,1; jl` instead of the single `dec;js`.

confidence: 8/10

The retail shape is what MSVC5 /O2 emits when the SOURCE guard pre-decrements the
count and tests the decremented value's sign, and the loop counter is the
ORIGINAL count recovered as `(count-1)+1`:

```cpp
void __stdcall ApplyToRange(Elem* base, i32 stride, i32 count, Fn fn) {
    if (--count >= 0) {            // dec eax; js skip   (count-1 < 0)
        char* item = (char*)base;
        i32 i = count + 1;         // lea edi,[eax+1]    (recover the iteration count)
        do {
            fn(item);              // mov ecx,item; call reg
            item += stride;
        } while (--i);             // dec edi; jne
    }
}
```

Key: `--count` (predecrement, used in the comparison) is what produces the bare
`dec eax; js` — testing the post-decrement sign flag directly, no extra copy/cmp.
`i = count + 1` then folds into the `lea edi,[eax+1]` that recovers the live count
AFTER the prologue saves (the guard's `dec` left `count-1` in eax). Writing the
guard as `count > 0`, `count >= 1`, or `count-- >= 1` all diverge.

Also note: a `__thiscall`-via-register callback (`mov ecx,item; call reg`, no
stack cleanup) is modeled as a pointer-to-member of a COMPLETE single-inheritance
class — `class Elem {}; typedef void (Elem::*Fn)();` then `(elem->*fn)()`. A
FORWARD-DECLARED class makes MSVC pick the general PMF representation (the
`mov (%esi),%ecx; mov (%ecx,%ebp),%ecx; add...; add...` this-adjust/vcall thunk);
the complete class gives the simple 4-byte code-pointer form (a plain `call`).
A free `void(__thiscall*)(void*)` typedef is rejected (C4234, reserved keyword).

WALL? No — fully closable. Evidence: ApplyToRange (@0x007c20) 65% (`for`) /
77% (`do-while(--count)`) / 87.6% (`count-- >= 1`) -> 100% with `if(--count>=0)` +
`i=count+1; do{}while(--i)`. related: loop-preheader-vs-exit-block-order.md
(the guard-placement WALL when a preheader `lea` also competes for placement).
