# A `push <imm>` next to a `mov reg,<imm>; push reg` proves TWO call sites, not one

tags: cpp:switch cpp:if cpp:call | asm:push asm:jmp | topic:codegen-idiom
symptoms: N switch arms read `mov eax,<imm>; push eax; jmp <join>` in the target but only `mov eax,<imm>; jmp <join>` in the base, so every arm is exactly one instruction short; the `else`/default path pushes a BARE IMMEDIATE in the target where the base routes it through the same register; the join block is 1 instruction longer in the base
confidence: 9/10

## Symptom

```asm
; TARGET                          ; BASE
  mov  eax,0xffff00                 mov  eax,0xffff00
  push eax                          jmp  <join>
  jmp  <join>                     ...
  ...                               <join>:
  push 0xffffff        <-- !!         mov  eax,0xffffff
  push esi                            push eax
  call SetTextColor                   push esi
                                      call SetTextColor
```

The block skeleton says every arm is `3i` in the target and `2i` in the base, and the
join is `1i` versus `2i`.

## Cause

The bare `push 0xffffff` is the tell. If the whole function had ONE call whose argument
came from a variable, cl would route *every* path through that variable's register —
there would be no immediate push anywhere. An immediate push on one path and a register
push on the others means the source has **two distinct call sites**, one taking a literal
and one taking the variable, and cl cross-jumped them down to their longest common tail
(`push hdc; call SetTextColor`). The switch arms then inherit the `push <var>` because
the variable-side call site is what they flow into.

Transcribing it as a single call after the `if`/`else` — the shape that *reads* the same
in C++ — gives cl only one push to emit, so it sinks it into the join and every arm loses
an instruction.

## The fix

Put the call inside both branches:

```cpp
// NO - one call, one push, arms are short
COLORREF color;
if (HAS(item->flags, FONT_ITEM_COLORED)) {
    switch (item->payload) { case ...: color = TCLR_NAVY; break; ... }
} else {
    color = TCLR_WHITE;
}
SetTextColor(hdc, color);

// YES - two call sites; cl cross-jumps them and the arms keep their push
if (HAS(item->flags, FONT_ITEM_COLORED)) {
    COLORREF color;
    switch (item->payload) { case ...: color = TCLR_NAVY; break; ... }
    SetTextColor(hdc, color);
} else {
    SetTextColor(hdc, TCLR_WHITE);
}
```

`CFontConfig::DrawTextLines` 0x22360: **89.51 -> 93.30**, all seventeen arms exact.

## Reading it the right way round

This is the inverse of assuming an N-arm shortfall is the unsteerable cross-jump wall.
Before parking a switch under
[`switch-arm-tail-crossjump-vs-duplicate`](switch-arm-tail-crossjump-vs-duplicate.md),
check the *default*/`else` path's argument: a literal there and a register everywhere
else is this bug, and it is a one-line source fix.

## Related

- [`two-calls-cross-jumped-down-to-the-differing-push`](two-calls-cross-jumped-down-to-the-differing-push.md)
- [`switch-arm-break-not-return-replicates-the-epilogue`](switch-arm-break-not-return-replicates-the-epilogue.md)
