# A `default: return;` we invented is an extra early return - it pins the whole prologue

tags: cpp:switch cpp:return | asm:push asm:pop | topic:codegen-idiom
symptoms: a `switch` whose arms only `strcat`/append, then one call after it; base has an
extra `push`/`pop` pair versus retail, the shared epilogue is one block where retail has
two, and every callee-saved register is rotated
confidence: 10/10

A `switch` that appends to a buffer and then falls into a single call almost never had a
`default:` arm in the original source - retail simply lets an unmatched value fall through
with the buffer un-appended:

```asm
   dec  eax
   jne  <the SetWindowTextA call>        ; <-- default falls THROUGH, it does not return
```

Reconstructing that as

```cpp
switch (mode) {
    case A: strcat(sz, "(640x480)");  break;
    ...
    default: return;                      // <-- fabricated; nothing in the bytes asks for it
}
SetWindowTextA(h, sz);
```

is not just a spurious block: it is an extra `return` in the body, and by
[shrink-wrapped-prologue-needs-one-tail-return.md](shrink-wrapped-prologue-needs-one-tail-return.md)
that pins every callee-saved `push` back into the prologue. The cost is spread over the
whole function, so it does not look like a `switch` bug at all.

Deleting the arm and nesting the entry guards recovered both halves at once:
`LoadVideoResolutionConfig` 0x36f30 **97.27 -> 100.00 EXACT** and
`SaveVideoResolutionConfig` 0x370a0 **88.81 -> 100.00 EXACT** (2026-08-08). In the second
one retail's prologue is a bare `sub esp,0x40` and all four saves are emitted inside the
body.

**Check before you write `default:`.** Point `gruntz sema disasm <rva> --blocks --lite` at
the switch's last `jne`/`jmp` and see where the unmatched value goes. If it lands on the
code AFTER the switch, there is no `default:` arm.

related: [shrink-wrapped-prologue-needs-one-tail-return.md](shrink-wrapped-prologue-needs-one-tail-return.md)
