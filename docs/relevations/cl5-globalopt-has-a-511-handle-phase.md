# cl 5.0 `/Og` has a 511-handle phase, but only phase-sensitive functions move

An unchanged function can alternate between two code states as unrelated declarations
change the translation unit. This is not random register allocation and it is not an
accumulated "compiler mood" from compiling earlier functions. On the pinned VC5 SP3
compiler, `CGameLevel::ProbeHeadSoft` proves a **511-symbol-handle periodic input to
`/Og`**. `CFaderShape::RenderTile` was the counter-example during the state campaign: it
remained byte-identical because its then-current optimized graph did not consume that
phase. A later authentic inline-helper reconstruction changed that graph and closed the
function exactly; declaration-state probes alone still cannot reach it.

The practical distinction is therefore conditional:

* a **phase-sensitive** function has an order-equivalent choice inside `/Og`; changing
  global symbol state selects another legal lowering;
* a **phase-stable** function has no such live choice under the tested state family, so
  more state islands do not search its remaining structural or stack-coloring residue.

"Stable" is evidence from a campaign, not an intrinsic property of a function forever.
A different authentic structural change can create or remove an order-sensitive choice.

## Worked example: one unchanged expression, two optimizer states

`CGameLevel::ProbeHeadSoft` 0x00160450 contains the unchanged expression:

```cpp
i32 px = t->m_screenX;
i32 py = t->m_screenY + t->m_extent.top + dy;
```

The two observed objects differ only in which independent member load is issued first:

```asm
; retail / exact phase             ; baseline phase
mov eax,[edx+0x60]                 mov eax,[edx+0x138]
mov ebx,[edx+0x138]                mov ebx,[edx+0x60]
mov edx,[edx+0x5c]                 mov edx,[edx+0x5c]
...
add eax,ebx                        add eax,ebx
```

The normalized target states are:

| state | COFF `.text` hash | outcome |
|---|---|---|
| lower-displacement first | `3c08935fd6b8cb93` | retail exact |
| higher-displacement first | `6927ed7ef132fb1d` | 99.977010% |

Nine honest expression/local-order variants emitted the baseline object byte-for-byte.
A 128-island mixed declaration-forest campaign found only the other target state, and
that state was exact. The declarations were disposable; MAX was banked for the unchanged
source fingerprint.

## The period is 511 handles

A body-free `struct S_n { int x; };` inserted immediately before the function advances
C1XX's measured `gl` high-water by seven handles. The state boundaries repeat after 73
such declarations:

| declaration count | added handles | observed target state |
|---:|---:|---|
| 20 | 140 | baseline |
| 21 | 147 | exact |
| 57 | 399 | exact |
| 58 | 406 | baseline |
| 93 | 651 | baseline |
| 94 | 658 | exact |
| 130 | 910 | exact |
| 131 | 917 | baseline |

Both boundary pairs are separated by `73 * 7 = 511` handles: 20/21 repeats at 93/94,
and 57/58 repeats at 130/131. Sparse controls inside the bands agree. This is stronger
than a score curve: adjacent source inputs select two byte-stable target objects, and the
transition itself recurs at exactly one 511-handle period.

The period is not a file-size or allocator-growth threshold. Classes with zero through
four inline members reach the same exact state at different declaration counts, and two
inputs with identical aggregate IL stream sizes can emit opposite states. Raising only
the `gl` high-water, appending ignored bytes to `ex`/`sy`, and removing the generated
pre-target function records one by one are also inert.

## The target's C1 IL is identical; `/Og` consumes global stream state

The `/d1il` tap resolves which compiler half owns the choice. At the adjacent 20/21
boundary:

* all four streams have identical sizes (`ex` 172068, `gl` 41357, `in` 4337,
  `sy` 55353 bytes);
* the target's own `ex` record is byte-identical on both sides;
* the target's own `sy` record is byte-identical on both sides;
* the first substantive `ex`/`sy` differences occur **after** the target, in later symbol
  records.

Feeding either captured stream set back through the same unchanged C2 reproduces its
corresponding object. Optimization controls then isolate the pass:

| compiler mode | baseline vs exact-island source |
|---|---|
| `/O2` | different target states |
| `/O2 /Og-` | byte-identical target |
| `/O2 /Ob0` | split remains |
| `/O2 /Ot-` | split remains |
| `/Od` | byte-identical target |

Thus C1XX creates the global symbol-handle phase in the streams, but the target function's
own intermediate representation does not change. C2's `/Og` processing observes global
stream state and chooses the load order. Register assignment and stack layout happen
later; calling this merely a "regalloc coin" loses the causal boundary.

Static RE gives the pass anchor: `globopt.c` owns `FUN_0040be34`; its hot continuation
from the line-7373 assert is 0x0040bf8d. The optimizer also maintains epoch
`DAT_0048fa34`, reset at 0x00421ad4 and used as visitation marks. Those addresses locate
the responsible domain, but do not yet identify the exact 511-state carrier.

One tempting carrier is a 511-bucket signed-key table:

| address | role |
|---|---|
| 0x0041256a | clears 0x1ff entries at `DAT_00496000` |
| 0x00412655 | lookup/insert; signed `idiv 0x1ff`, remainder selects the chain |
| `DAT_00496000` | bucket heads; nodes are 0x14 bytes, key at +4, next at +0x10 |

That identification is static and real, but causation is **falsified** for this wall:
replacing the divisor with 509 and 479 via `/B2`, then replaying both adjacent 20/21 IL
captures, leaves baseline/exact unchanged. A separate 101-bucket expression table at
0x0040aa4f was likewise inert under 97/89/83/79 divisor controls. The measured period is
511; which internal structure carries it remains open.

## Why some functions stay stable until their structure changes

At the campaign baseline, `CFaderShape::RenderTile` 0x00182610 was a direct
counter-example. Its remaining six bytes were two frame homes in the opposite order
(`rowBytes` and spilled `rowSrcA`). Across the baseline plus 128 deterministic mixed
declaration forests:

* all 129 compiles emit one normalized target state;
* the target stays 747 bytes with the same 32 branches and one return;
* the source structures, declaration families, and handle strides vary, but the two frame
  homes never exchange.

Its own `ex` and `sy` target records are also identical between baseline and a forest
island. Unlike `ProbeHeadSoft`, no tested global phase reaches the residue. The frame-home
choice belongs to later stack-slot coloring and must be searched structurally.

Ghidra RE of `c2.exe` makes the reason precise:

| function | role |
|---|---|
| `FUN_0042b909` | starts frame allocation by zeroing `DAT_004910ac`, every slot count/array/link, and the per-function local set |
| `FUN_0042ba3f` | walks only the current function's operand lists and admits stack-resident descriptors |
| `FUN_004367f0` | keeps those descriptors ordered by size at +0x20, then descending occurrence count at +0x34; exact ties retain discovery order |
| `FUN_0042bbb9` | assigns dense local IDs in that list order |
| `FUN_004401cd` | builds live/interference sets from the current function's CFG and operands |
| `FUN_00440cbd` | reuses a compatible slot or appends a new one |
| `FUN_0043f93b` | colors the slots and writes the final frame offsets |

The complete external-compiler address and meaning index is
[cl5-c2-function-map.md](cl5-c2-function-map.md).

No TU symbol handle, 511-bucket state, or `/Og` epoch feeds this chain. Its complete
ordering input is the optimized current function: local size, occurrence/discovery order,
and live-range interference. Consequently, declarations elsewhere in the TU cannot swap
`rowBytes` and `rowSrcA` unless they first change `RenderTile`'s optimized graph; the 129
campaign states do not.

A patched-back-end control confirmed ownership of that baseline residue. At VA
0x00436875, changing the equal-size
descriptor frequency stop from `jge` to `jle` reverses that local ordering policy. Through
`/B2`, unchanged `RenderTile` changes from 747 bytes / hash `e7b93ee4b9f7c019` to 752
bytes / hash `83ae40bf73433abb`, with its frame homes broadly recolored. This is deliberately
not a source fix; it proves that the reset, function-local frame-coloring chain owns the
residue.

The structural route subsequently succeeded. Two caller-local forward byte-copy loops
were repetitions of one TU-local inline `CopyBytes(u8*, const u8*, i32)` operation. The
inline form changes the function's optimized operand/lifetime graph and emits the retail
747-byte body exactly. Direct CRT `memcpy` emits `rep movsd`/`rep movsb` and falls to
93.0085%; a cursor-owning macro falls to 97.12%. Thus the 129-state result was not a claim
that no source fix existed: it proved only that declaration state could not alter the old
graph and correctly routed the work to structural reconstruction. See
[`../patterns/scalar-byte-copy-is-an-inline-helper.md`](../patterns/scalar-byte-copy-is-an-inline-helper.md).

That is why a campaign that finds one island must say so and route the next search to
structure. The broad `REGALLOC/SCHEDULING` label hides two different causes here:
`ProbeHeadSoft` is an earlier `/Og` expression-order choice with TU-global phase input;
the old `RenderTile` graph was a later, reset-per-function frame-coloring choice with no
such input, and only a real source boundary changed that graph.

## Reverse-use rule

1. First prove the body and classify the first divergence.
2. Run a bounded mixed-kind campaign and census normalized target states, not just scores.
3. If multiple islands exist, replay adjacent/minimal states through the IL tap and use
   pass-disable controls. An exact island is evidence of missing authentic TU structure;
   bank it for the unchanged fingerprint and discard the probes.
4. If only one island exists across a real multi-state campaign, stop compiler-state
   search. Report **single island; next search should be structural**.
5. Never retain declarations, includes, fake locals, or patched compiler artifacts. They
   are measurement instruments, not source fixes.
