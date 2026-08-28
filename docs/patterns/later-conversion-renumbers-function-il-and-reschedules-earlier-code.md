# A later explicit conversion can renumber a function's IL and reschedule earlier code
tags: cpp:cast cpp:int cpp:expr | asm:add asm:mov | msvc5:c1 msvc5:c2 | topic:tu-state topic:regalloc topic:codegen-idiom
symptoms: a function has the correct call/branch/return/relocation topology, but independent
  member loads or additions near its beginning are ordered differently from retail; changing
  a genuine type conversion much later in the same body moves that earlier schedule
confidence: 9/10
variants: tu-state-probe-family-decides-reachability.md, narrowing-cast-vs-mask-changes-value-lifetime.md

MSVC 5.0 does not optimize a function as a source-order stream whose earlier instructions
are sealed before it parses later statements. A genuine explicit conversion near the end of
a function can add a C1 symbol handle and make C2 choose another legal schedule for an
independent expression near the beginning.

`CNetSession::ComputeChecksum` at `0x000c0590` is the controlled instance. Its accumulator
and return type are signed `i32`, while `g_frameTime` is an unsigned `u32`. The two inputs
differ by one source expression, after the function's large pickup-successor switch:

```cpp
// A
+ g_frameTime + IDX(next)

// B: the real signed checksum boundary
+ static_cast<i32>(g_frameTime) + IDX(next)
```

The affected instructions are more than eighty source lines earlier. Form A emits the three
independent object-field additions in displacement order:

```asm
add ebp,[eax+0x3ec]
add ebp,[ecx+0x5c]
add ebp,[ecx+0x60]
add ebp,[ecx+0x74]
```

Form B emits retail's order:

```asm
add ebp,[eax+0x3ec]
add ebp,[ecx+0x60]
add ebp,[ecx+0x74]
add ebp,[ecx+0x5c]
```

This is not attribution from fuzzy movement. The pinned `/d1il` capture gives:

| stream | no cast | explicit cast | delta |
|---|---:|---:|---:|
| `ex` | 325,761 B | 325,764 B | +3 B |
| `gl` | 99,201 B | 99,201 B | 0 B |
| `in` | 27,904 B | 27,904 B | 0 B |
| `sy` | 106,820 B | 106,820 B | 0 B |
| `gl` handle high-water | `0xcd40` | `0xcd41` | +1 handle |

Both resulting objects are 23,907 bytes. Apart from the timestamp, they differ at eleven
object bytes. Feeding each captured IL set through the same unchanged `c2.exe` reproduces
the same eleven-byte split, so the later source conversion changes the C1 input consumed by
C2; it is not objdiff pairing, linking, or an edit in the earlier expression.

The boundary is specific, not a license to scatter casts. A 16-cell matrix replacing the
four existing enum `IDX(...)` conversions with `static_cast<i32>(...)` emitted one compiler
island. Ten alternative clock-expression boundaries, followed by 170 mixed source/TU-state
combinations, found no better authentic state than the direct conversion of `g_frameTime`.
The cast is retainable because the unsigned engine clock really crosses into a signed
checksum domain; an unused cast or an invented conversion remains a compiler-state probe and
must be deleted.

## Reverse-use rule

1. First prove equal calls, branches, returns, relocations, and semantic operands. This
   mechanism explains a schedule, not missing behavior.
2. Do not restrict the source search to the first assembly divergence. Audit genuine signed,
   narrowing, enum, pointer, and aggregate boundaries through the whole function, including
   statements after a large switch or loop.
3. Change one evidence-backed boundary at a time and capture `/d1il`. A handle or target-IL
   change plus same-C2 replay is causal evidence; a fuzzy score alone is not.
4. Retain the conversion only when it expresses the real source domain and moves a baseline-
   absent feature toward retail. Never add fake casts, declarations, or locals to land a
   handle phase.
5. If the IL change is real but the target object is flat, or a mixed state campaign produces
   one island, route back to structural reconstruction rather than multiplying conversions.

