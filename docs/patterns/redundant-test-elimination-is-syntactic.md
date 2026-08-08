# cl5 drops a re-test only when it is spelled the SAME way — reverse the operands
tags: cpp:branch cpp:loop cpp:expr | asm:cmp asm:jcc | topic:codegen-idiom
symptoms: exactly ONE conditional branch missing on the base side; retail has two consecutive `cmp <same reg>,<same imm>` / `jcc <same target>` pairs a few bytes apart where the base has one; `--blocks --diff` shows one target block split in two (`3i [jcc | fall]` against the base's `1i [fall]`); every other block `==`
confidence: 9/10
variants: redundant-sibling-guard-retest.md, guard-skip-loop-not-early-return.md

`if (n < 0x37) { for (i = n; i < 0x37; i++) … }` gives cl5 two comparisons on the same
value, and its peephole deletes the second — leaving the loop with no entry test. Retail
keeps both. The peephole is **syntactic**: write either comparison with the operands the
other way round and it no longer recognises the pair, even though both lower to the same
`cmp esi,0x37`.

```cpp
// ONE pre-loop test - cl elides the loop's entry check
if (counter < 0x37) { for (i32 i = counter; i < 0x37; i++) { … } }
// TWO, which is retail
if (counter < 0x37) { for (i32 i = counter; 0x37 > i; i++) { … } }
```

```asm
; TARGET (and the reversed spelling)      ; BASE (both written `<`)
  cmp  edi,0x37                             cmp  edi,0x37
  jge  <after the loop>                     jge  <after the loop>
  cmp  edi,0x37                             mov  [esp+0x10],edi
  mov  [esp+0x10],edi                     LOOP:
  jge  <after the loop>                     …
LOOP:
```

Measured 2026-08-08: `CPlay::BuildHelpReveal` @0xd72c0 **97.92 -> 100.00 EXACT** on that
one operand swap.

## The four spellings, measured

| spelling | pre-loop tests |
|---|---|
| `if (n < 0x37) { for (i = n; i < 0x37; i++) … }` | 1 |
| `if (n < 0x37) { i = n; while (i < 0x37) { … i++; } }` | 1 |
| `if (n < 0x37) { i = n; if (i < 0x37) { do { … } while (++i < 0x37); } }` | 1 |
| `if (0x37 > n) { for (i = n; i < 0x37; i++) … }` | **2** |
| `if (n < 0x37) { for (i = n; 0x37 > i; i++) … }` | **2** |

An explicit re-test does NOT survive; only the differing SPELLING does. In a standalone
probe a guard reading the member (`if (s->n < 0x37)`) against a loop bound reading the
copy also kept both, but that did **not** reproduce inside the real function - the
operand order is the reliable lever.

## Finding the rest

The signature is a target-side adjacent branch pair with the same mnemonic, the same
destination and an identical preceding compare, that the base does not have:

    python -m gruntz.audit.dup_compare          # the worklist
    python -m gruntz.audit.dup_compare --near N # only pairs within N bytes (the peephole case)

Tree-wide 2026-08-08 the sweep found **exactly one** instance (`BuildHelpReveal`) and
**17 look-alikes** that the tool's clobber screen rejects: two `testl %eax,%eax` a few
bytes apart with a `mov eax,<field>` between them is `if (p && p->next)`, not a re-test.
The screen is what makes the sweep usable - without it the signal is 6% of the rows.

Read the gap between the two offsets first. **Within ~20 bytes** the two tests are
adjacent and this pattern applies. **Far apart, with calls in between**, it is the
different mechanism in
[redundant-sibling-guard-retest](redundant-sibling-guard-retest.md) — a flag held in a
callee-saved register across calls, fixed by DE-NESTING the two guards, not by reversing
an operand.

## Do not confuse with a real extra condition

One missing branch can equally be a whole `&&` clause we dropped. Check that the two
retail compares are on the *same* operands before reaching for the swap; if they differ,
the source is missing a test, not a spelling.
