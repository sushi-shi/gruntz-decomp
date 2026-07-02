# `(dwordVar & bit) == bit` — retail keeps the dword load but a byte AND; no source spelling reproduces the mix

**Tags:** cpp:bitand cpp:int | asm:and asm:mov | topic:wall topic:regalloc

## Symptom

A single-bit test returning bool, e.g.

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

Everything after (`cmp al,imm; sete`) matches. ~99% fuzzy, 1-instruction residual.

## Why

Under `/O2` (favor speed) MSVC5 keeps `status` a dword stack slot (it is the
`GetStatus` out-param, so it is loaded with a full `mov r32,[mem]`), and its own
peephole for `x & imm8` picks `and r32,imm8` (avoids the partial-register write).
Retail's build narrowed the *same* AND to `and al,imm8` (the shorter encoding)
while **keeping** the dword load — a mixed form.

Neither pure source spelling reproduces retail's mix:

- `(status & bit) == bit` → dword load **+ dword AND** (`and eax`), matches the load,
  misses the AND. ~99%.
- `((u8)status & bit) == bit` → **byte load** (`mov al,[esp]`, opcode `8a`) + byte AND
  (`and al`), matches the AND but now the load opcode differs. ~99.85% (closer, but
  still 1 byte, and the load is now wrong / uglier source).

The dword-load-plus-byte-AND combination is an internal `/O2` register-width /
partial-register tiebreak, not selectable from C. Leave the clean
`(status & bit) == bit` spelling and `@early-stop`.

## Evidence

- `DirectSoundMgr::IsLooping` (0x135440) and `IsPlaying` (0x1353f0): identical
  residual (`and eax,2`/`and eax,1` vs retail `and al,2`/`and al,1`), 99.85%.

## Related

- [char-and1-movb-vs-movsx](char-and1-movb-vs-movsx.md) — the *load* narrows for a
  signed-char `& 1` (movb vs movsx); a different mismatch on the same family.
- [align-down-byte-and-encoding](align-down-byte-and-encoding.md) — `& ~0x1f`
  emits byte `and al,0xe0`; steerable there via the mask spelling, not here.
