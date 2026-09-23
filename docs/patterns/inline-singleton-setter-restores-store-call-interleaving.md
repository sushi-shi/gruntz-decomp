# A singleton setter restores stores interleaved with the next call's arguments

tags: cpp:inline cpp:array cpp:local cpp:call | asm:mov asm:push | topic:codegen-idiom topic:scheduling

## Signature

A long table initializer is identical to retail except for one or two stores
between argument pushes for its next external call. The extent, instruction
census, constants, and ordered relocation positions already agree. Do not
rewrite the preceding arithmetic merely because an old wall review names it.

The seven `CMinimap` palette builders below had exactly this residue. Their
RGB packing, fill loops, stack homes, and register assignments already matched.
Only singleton `u16` stores around `FillSpan` calls were scheduled too late:

```asm
; bare array assignments             ; retail / inline setter calls
push ebx                             push ebx
push 107h                            push 107h
push 105h                            mov word ptr [esi+24eh],bx
mov ecx,esi                          push 105h
mov word ptr [esi+24eh],bx            mov ecx,esi
mov word ptr [esi+252h],bx            mov word ptr [esi+252h],bx
call FillSpan                        call FillSpan
```

## Controlled closure

The typed boundary is small but load-bearing:

```cpp
static inline void SetTileColor(u16* colors, u32 tile, u16 color) {
    colors[tile] = color;
}

SetTileColor(buf, 257, color);
SetTileColor(buf, 259, color);
FillSpan(0x105, 0x107, color);
```

On `BuildGruntziclezPalette`, changing those two stores alone took 99.48718%
to strict 100%: 2130 bytes, 624 decoded instructions, and all 55 ordered
relocations, including offsets and addends, identical. Keeping the helper
definition but restoring the bare stores returned to 99.48718%. The inline
calls, not the mere declaration's TU-state effect, are causal. A chained
assignment instead reversed the two stores and scored 99.48558%.

Applying the same boundary to every singleton store, while retaining every
fill loop, index, color, and external call, closed the complete sibling set:

| Palette | RVA | Before | Exact bytes / instructions / relocations |
| --- | --- | ---: | ---: |
| Rocky Roadz | `0xa3dc0` | 99.503105% | 2143 / 644 / 44 |
| Gruntziclez | `0xa4890` | 99.487180% | 2130 / 624 / 55 |
| Tropicz | `0xa5310` | 99.498436% | 2133 / 638 / 43 |
| High on Sweetz | `0xa5d90` | 99.479675% | 2085 / 615 / 51 |
| Honey | `0xa7260` | 98.855420% | 2240 / 664 / 43 |
| Miniature Masterz | `0xa7d50` | 99.379410% | 2383 / 709 / 46 |
| Space | `0xa8900` | 99.083090% | 2342 / 698 / 45 |

`BuildHighRollerzPalette` improved from 99.23876% to 99.92093%, retaining a
separate temporary-home permutation and one-byte extent difference. The setter
does not by itself prove that remaining allocator choice.

## Reverse use

Read the first actual divergence. A setter's entire machine body can disappear
while its argument temporaries still determine the surrounding schedule. Test
the authentic typed operation before emulating the emitted push/store ordering
with caller-side temporary variables. Verify a declaration-only negative
control whenever an added helper could also perturb TU state.

This complements the [fill-loop recovery](adjacent-same-value-stores-are-a-loop.md):
adjacent ranges remain loops; singleton writes retain their own abstraction.
It also supersedes stale RGB-order explanations for the seven exact source
families above, without claiming that all RGB or scheduling residues are fixed.
