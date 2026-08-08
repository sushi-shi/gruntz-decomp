# A shared block expanded TWICE in one caller must be a macro, not an inline function
tags: cpp:inline cpp:call cpp:local | asm:push asm:pop | topic:codegen-idiom
symptoms: after folding N open-coded copies into one `inline` helper, every caller that uses it
ONCE stays byte-exact and every caller that uses it TWICE OR MORE drops ~30 points; the diff
shows an extra `push ebp` / `pop ebp` and esi<->edi swaps
confidence: 10/10

MSVC 5 shares one register plan across two inline expansions of the same function in one caller,
but not across two macro expansions. So a helper that some caller expands more than once has to
expand TEXTUALLY — all the way down, including any helper it calls.

Measured folding the 61 act-name registrar blocks into one definition
(`include/Gruntz/ActNameRegistry.h`), four spellings, one build each:

| outer block | inner grow-loop | exact functions |
|---|---|---|
| `inline` function | `inline` function | 3306 (-9) |
| macro | `inline` function | 3306 (-9) |
| `inline` function | textual | 3306 (-9) |
| **macro** | **textual** | **3315** (baseline, byte-neutral) |

Every one of the nine regressions was a registrar that expands the block twice or more — nine
multi-block ones out of ~40, each 100% -> ~70%. Single-block registrars were byte-exact under all
four spellings, which is what makes the effect easy to misread as ordinary regalloc noise: the
tree-wide fuzzy only moved 0.15%.

The cost is one extra callee-saved register. Retail's `CDroppedObject::RegisterActs` uses ebx,
esi, edi; the inline-function version uses ebx, esi, edi AND ebp, and picks the opposite of
esi/edi for the surviving id:

```asm
; retail (and the macro spelling)          ; the inline-function spelling
push ebx / push esi / push edi             push ebx / push esi / push edi / push ebp
...                                        ...
sub  esi,eax                               sub  edi,eax
imul esi,[<m_stride>]                      imul edi,[<m_stride>]
```

Note the third row of the table: making only the OUTER block a macro is not enough. The inner
`ActNameConstructGrownSlots()` was still an inline function expanded twice, and that alone held
the whole family down. The rule is transitive — nothing in the expansion may be a function.

MSVC 5 has no `__forceinline`, so a textual macro is the period-correct device for a block that
must expand independently at every site. That is why the two registrars a previous lane had
already matched — `CGrunt`'s 19-block `REGISTER_KEY_644AF0` and `CWarlord`'s 6-block one — were
written as macros: the same wall, hit from the other direction.

Corollary for reading the target: a function that expands a shared block N times and uses only
three callee-saved registers is telling you the source spelled it as a macro. Count the pushes
before you spend a build.

## Not this

Do NOT reach for the macro when a single-expansion caller regresses — that is a different bug
(usually the block's own statement shape, see
[act-registrar-counter-cse-and-freeloop](act-registrar-counter-cse-and-freeloop.md)). The
signature here is specifically that the split is by EXPANSION COUNT: all the once-callers fine,
all the twice-callers broken, in one build.
