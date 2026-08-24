# `(dwordVar & bit) == bit` — the dword-load + byte-AND mix comes from the `if` STATEMENT form, not the `return` expression

**Tags:** cpp:bitand cpp:int cpp:if | asm:and asm:mov | topic:codegen-idiom

## Symptom

A single-bit test returning bool, written as a returned expression:

```cpp
u32 status;
buf->GetStatus(&status);
return (status & DSBSTATUS_LOOPING) == DSBSTATUS_LOOPING;   // bit == 0x2
```

Body is byte-identical to retail **except the mask instruction**:

```
retail:  8b 44 24 00   mov eax,[esp]      ; dword load of `status`
         24 02         and al,0x2         ; BYTE and
base:    8b 44 24 00   mov eax,[esp]      ; dword load (matches)
         83 e0 02      and eax,0x2        ; DWORD and (1 byte too long)
```

Everything after (`cmp al,imm; sete` for a multi-bit mask, `dec al / neg al /
sbb eax,eax / inc eax` for bit 0) already matches. ~99.9% fuzzy, 1-instruction
residual.

## Fix

**Write the test as an `if` statement with two `return`s, not as a returned
expression.** That is the whole difference:

```cpp
// emits `and eax,2` — does NOT match
return (status & DSBSTATUS_LOOPING) == DSBSTATUS_LOOPING;

// emits `mov eax,[esp]` + `and al,2` — MATCHES retail
if ((status & DSBSTATUS_LOOPING) == DSBSTATUS_LOOPING) {
    return 1;
}
return 0;
```

`if (...) { return 1; } else { return 0; }` works identically. The generated
code is otherwise the same — cl still folds the branch into the flag-materialising
sequence — it just narrows the mask to the low byte.

## Why

The two forms reach cl's back end differently. As a *returned expression* the
`==` result is materialised as an `int` value, and the mask feeding it keeps
dword width. As a *branch condition* the `==` is a jump-on-condition whose
operand cl knows only needs the low byte; it narrows the AND to `and r8,imm8`
(2 bytes for `al`) and only afterwards re-materialises the boolean into eax with
the same `sete` / `dec-neg-sbb-inc` tail.

Do **not** reach for `(u8)status` — that narrows the *load* too (`mov al,[esp]`,
opcode `8a`), which retail does not do.

## Evidence

Probe (`cl 5.0 /O2 /MT /GX`), one TU, `u32 s` filled by an out-param call:

| source | emitted |
|---|---|
| `return (s & 2) == 2;` | `mov ecx,[esp+4]` · **`and ecx,2`** · `xor eax,eax` · `cmp cl,2` · `sete al` |
| `if ((s & 2) == 2) { return 1; } return 0;` | `mov ecx,[esp+4]` · **`and cl,2`** · `xor eax,eax` · `cmp cl,2` · `sete al` |
| `return (s & 1) == 1;` | `mov eax,[esp+4]` · **`and eax,1`** · `dec al` · `neg al` · `sbb eax,eax` · `inc eax` |
| `if ((s & 1) == 1) { return 1; } return 0;` | `mov eax,[esp+4]` · **`and al,1`** · `dec al` · `neg al` · `sbb eax,eax` · `inc eax` |
| `return (u8)s & 2) == 2;` | **`mov cl,[esp+4]`** (byte load — wrong) · `and cl,2` |

Flipped `SoundBuffer::IsPlaying` (0x1353f0), `IsLooping` (0x135440) and
`IsInHardware` (0x135490) from 99.85–99.88% to **100% EXACT** in one edit each
(2026-07-28). All three had been parked as an "unsteerable partial-register
tiebreak" wall.

## Related

- [char-and1-movb-vs-movsx](char-and1-movb-vs-movsx.md) — the *load* narrows for a
  signed-char `& 1` (movb vs movsx); a different mismatch on the same family.
- [align-down-byte-and-encoding](align-down-byte-and-encoding.md) — `& ~0x1f`
  emits byte `and al,0xe0`; steerable there via the mask spelling.
