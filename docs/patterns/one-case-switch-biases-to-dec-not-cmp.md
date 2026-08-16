# `dec <reg>` where you wrote `cmp <reg>,1` — the source is a ONE-ARM `switch`, not an `if`

tags: cpp:switch cpp:if cpp:branch | asm:dec asm:sub asm:cmp asm:jne | topic:codegen-idiom
symptoms: a tiny handler is byte-for-byte right except for its single guard; `gruntz walls diagnose --asm` shows exactly one replaced pair, `-cmp eax,0x1` against `+dec eax`, with the same
  `jne`, the same branch count and the same ret count
confidence: 10/10

cl5 lowers EVERY `switch` by biasing the subject against the lowest case value before
it tests anything — that is the same `value - min` normalisation the jump table needs
(see switch-arm-emission-follows-source-order.md, where `ClearTabGroup`'s `dec eax`
makes table slot `k` mean `key == k+1`). The bias survives all the way down to the
degenerate switch: **one case, no table, no range check** still gets the subtract, and
a subtract by 1 is encoded `dec`.

An `if` never does this. `if (x == 1)` is a comparison against a constant, so cl emits
`cmp <reg>,0x1` and keeps `<reg>` live. There is no /O2 peephole that folds one into
the other — `dec` is a *destructive* subtract, `cmp` is not, and cl decides between them
in the front end from the statement form, not in the encoder from the operand.

So the direction of the read is fixed:

| retail                | source it came from            |
|-----------------------|--------------------------------|
| `cmp <reg>,0x1; jne`  | `if (x == 1) { … }`            |
| `dec <reg>; jne`      | `switch (x) { case 1: … }`     |
| `sub <reg>,N; jne`    | `switch (x) { case N: … }`     |

## Worked example — `CMultiStartDlg::OnTimer` @0xc2c80

Retail, 0x1a bytes:

```
mov  eax,dword ptr [esp+0x4]   ; nIDEvent
push esi
dec  eax                       ; <-- the tell
mov  esi,ecx
jne  0x4c2c8f
call 0x4016cc                  ; ILT -> CMultiStartDlg::Watchdog
mov  ecx,esi
call 0x5bb18f                  ; CWnd::Default
pop  esi
ret  0x4
```

The `if` spelling reconstructs everything else exactly and stops at **94.00%**:

```cpp
void CMultiStartDlg::OnTimer(u32 nIDEvent) {
    if (nIDEvent == 1) {        // -> cmp eax,0x1
        Watchdog();
    }
    Default();
}
```

```
--- base
+++ target
 mov eax,dword ptr [esp+0x4]
 push esi
-cmp eax,0x1
+dec eax
 mov esi,ecx
 jne <tgt>
```

The switch spelling is **100.0000% EXACT**, nothing else changed:

```cpp
void CMultiStartDlg::OnTimer(u32 nIDEvent) {
    switch (nIDEvent) {
        case MULTI_START_WATCHDOG_TIMER:
            Watchdog();
            break;
    }
    Default();
}
```

This is also the natural source: MFC handlers written against a timer-id domain use a
`switch` even when only one id is live, which is why the shape turns up in `OnTimer` /
`OnCommand` / `OnSysCommand` bodies specifically.

## Screening the tree

A one-instruction `cmp <reg>,<imm>` -> `dec`/`sub <reg>,<imm>` replacement is the whole
objdiff residue, so these functions sit high (90-99%) and read as a register/scheduling
wall. They are not — they are a one-line source-form fix. Sieve for them with

```
gruntz walls diagnose <rva>          # branch-shape agreement
gruntz walls diagnose <rva> --asm | grep -A2 -B2 -E '(cmp|dec|sub)'
```

and prefer the switch form wherever the subject is a domain (a message id, a timer id,
a mode tag) rather than a boolean-ish test.

related: switch-arm-emission-follows-source-order.md,
inline-switch-serialize-record-unroll.md, zero-register-compare-is-against-zero.md
