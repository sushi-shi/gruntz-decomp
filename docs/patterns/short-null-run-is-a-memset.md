# A short run of NULL/0 stores gets its OWN `lea` base and its OWN zero register — it was a `memset`

- confidence: 9/10
- tags: `cpp:array` `cpp:struct` `cpp:builtin` | `asm:lea` `asm:xor` `asm:mov` | `topic:codegen-idiom` `topic:regalloc`

## Symptom

A function that already pins 0 in a callee-saved register (`xor ebp,ebp` in the prologue,
then `cmp <r>,ebp` / `mov [this+N],ebp` everywhere) has one or more *runs* of 3-5 adjacent
pointer members set to NULL. Retail writes each run as

```
lea  ecx,[ebx+0x204]      ; or `add ebx,0x61c` when `this` is dead after
xor  eax,eax              ; a SECOND zero, not the function-wide one
mov  DWORD PTR [ecx],eax
mov  DWORD PTR [ecx+0x4],eax
mov  DWORD PTR [ecx+0x8],eax
mov  DWORD PTR [ecx+0xc],eax
mov  DWORD PTR [ecx+0x10],eax
mov  DWORD PTR [ebx+0x218],ebp    ; the NEXT, non-run member keeps the shared zero
```

while the recompile emits every store as `mov DWORD PTR [this+disp],<shared-zero>` — no
`lea`, no second `xor`. Two instructions short per run, and because retail needs one more
live register it also pushes one more callee-saved register, so **every** `ret` site in
the function differs by a `pop` too. On a five-armed `switch` that is a 12-instruction
deficit and ~30 points.

## Cause

`memset(arr, 0, sizeof(arr))` with a **constant** size under 6 dwords is expanded by cl5
as a base-pointer + N dword stores, and the expansion materializes its own address and its
own zero — it does not participate in the surrounding function's constant pinning. Writing
the same stores as source-level assignments (`p[0] = NULL; p[1] = NULL; ...`, with or
without a named `T** p = m_arr;` cursor) folds them into `this`-relative displacements and
reuses the function-wide zero register instead. The named cursor does not help: cl
constant-folds `p = m_arr` into the displacement.

Boundary: at ~12 dwords and up the same `memset` becomes `mov ecx,N; xor eax,eax;
lea edi,[this+off]; rep stos` — that form is already unambiguous in the diff. It is the
**short** runs that read like hand-written assignments and are not.

## Fix

Write the run as `memset`, sized by the array:

```cpp
case TAB_MULTIPLAYER:
    memset(m_warlordHead, 0, sizeof(m_warlordHead));   // 4 slots
    break;
```

Members that are *not* part of a contiguous array stay as plain `= NULL` assignments —
they are the ones that keep the shared zero register, and converting them too costs the
`lea`.

## Evidence

`CStatusBarMgr::ClearTabGroup` 0x100b00, three arms: 69.14 -> **99.72** (size 336, exactly
retail's), and the ripple took `CStatusBarMgr::UpdateStatusBarTabHighlight` 0x0fe910
47.35 -> 85.03 and `ResetWidgets` 0x100930 74.68 -> 77.88 in the same TU. A three-axis
Cartesian over the arms (`batch_source_variants`) is monotone in the number of arms
converted: 75.81 (none) / 80.77-90.88 (one) / 90.15-93.69 (two) / 99.72 (all three), so
each run is independently worth points.

## Related

- [`rep stosd` with an ELEMENT count and no byte tail is a LOOP, not `memset`](rep-stos-without-a-byte-tail-is-a-loop.md) — the opposite direction, for VARIABLE sizes.
- [A zero-store GROUP after a call is a LOOP](zero-group-loop-gives-its-own-constant.md) — same "the group gets its own constant" mechanism, read from the ctor side.
