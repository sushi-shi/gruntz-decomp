# `(x >> k) << n` folds into one `lea`; spell it `* (1<<n)` to keep retail's `sar`/`shl` pair

**Confidence 9/10.**
Tags: `cpp:bitop` `cpp:int` | `asm:sar` `asm:shl` `asm:lea` `asm:and` | `topic:codegen-idiom`

## Symptom

Retail composes a packed key out of two shifted halves and emits three instructions
for the high half plus a separate `add`:

```
mov  eax,edi          ; x
mov  edx,esi          ; y
sar  eax,0x5          ; x >> 5
sar  edx,0x5          ; y >> 5
shl  eax,0x8          ; << 8
add  edx,eax
```

The obvious transcription `((x >> 5) << 8) + (y >> 5)` compiles to something SHORTER —
cl fuses the whole thing into an `and` plus a scaled `lea`:

```
mov  edx,esi
and  edx,0xffffffe0   ; x & ~31
sar  eax,0x5          ; y >> 5
lea  ecx,[eax+edx*8]  ; (y>>5) + (x & ~31)*8
```

Both compute the same value: `(x>>5)<<8 == (x & ~31) * 8` for an arithmetic shift.
The recompile being **shorter than retail on a pure integer expression** is the tell.

## Cause

MSVC5's peephole recognises `(x >> k) << n` when `n > k` and rewrites it as
`(x & ~((1<<k)-1)) << (n-k)`, which then absorbs the surrounding `+` into one `lea`
addressing mode. The rewrite fires on the *shift-of-a-shift* syntactic form.

## Fix (steerable)

Write the high half as a **multiply**, not a shift:

```cpp
i32 key = ((x >> 5) * 0x100) + (y >> 5);   // sar / sar / shl / add  (retail)
// NOT: ((x >> 5) << 8) + (y >> 5);        // and / sar / lea
```

cl still lowers `* 0x100` to `shl eax,8`, so the emitted bytes for the high half are
identical to retail's — but the shift-of-a-shift pattern is no longer present in the
IR, so the fold does not fire and the `add` stays a real `add`.

`|` instead of `+` is a partial workaround (it blocks the `lea` absorption but not the
`and` rewrite) and scores between the two.

## Evidence

`CTriggerMgr::ApplySwitch` @0x6d300 builds `((sx>>5)*0x100) + (sy>>5)` as the
`CTileTriggerContainer::FindSwitchLogic` key in **five** separate switch arms. Measured over
the same reconstruction, all five sites changed together:

| spelling | fuzzy |
|---|---|
| `((sx >> 5) << 8) + (sy >> 5)` | 79.94 |
| `(sy >> 5) + ((sx >> 5) << 8)` | 79.94 (operand order is inert) |
| `(((sx >> 5) << 8) \| (sy >> 5))` | 88.11 |
| `((sx >> 5) * 0x100) + (sy >> 5)` | **93.19** |

## See also

- [strength-reduced-dst-cursor-indexed-vs-pointer.md](strength-reduced-dst-cursor-indexed-vs-pointer.md)
  — the loop-cursor form of "cl strength-reduced something retail did not".
- [unsigned-divmod-reuse-quotient.md](unsigned-divmod-reuse-quotient.md) — the inverse
  case, where you *want* cl to strength-reduce.
