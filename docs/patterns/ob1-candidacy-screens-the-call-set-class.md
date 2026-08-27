# An undefined COFF symbol does not settle inline candidacy
tags: cpp:inline cpp:call | asm:call asm:jmp | topic:codegen-idiom topic:tooling
symptoms: `walls diagnose` says INLINE/CALL-SET with a REPEATED-SITE DELTA, one
side calls the same callee N+1 times, inline-model, /Ob1, budget deficit,
finish-the-caller, `spec JSON missing`
confidence: 10/10 (controlled source A/B plus base-object code and symbols)

`gruntz walls diagnose` reports `INLINE/CALL-SET` whenever the two sides' call
multisets differ, and CLAUDE.md points that class at
`gruntz walls inline-model --gap`. But a call-count delta has TWO causes and
only one of them is an inline decision:

1. the callee was **expanded** on one side and **called** on the other - a real
   /Ob1 budget question;
2. the same callee is **called twice on one side and once on the other** - a
   late **tail-merge / cross-jump** decision, where two arms end in an identical
   suffix on one side and not the other. Nothing about inlining is involved.

The call counts alone do not distinguish these shapes. Reading either one off
the symbol table sends the campaign toward a source lever the evidence did not
select.

## What the symbol table proves—and what it does not

`/O2` implies `/Ob1`: cl 5.0 never auto-inlines an unmarked function at any
definition position. A callee emitted as a **defined COMDAT** in the caller's
object therefore positively proves that an inline body was visible.

The inverse is false. An **UNDEFINED** row can be left by another declined or
nested call to an otherwise visible header inline. A callee can also be absent
from the symbol table because every local site expanded. Retail delinking adds
another reason not to invert the test: selected library and header-inline bodies
are deliberately represented as externals. See
[retail-obj-externals-prove-mfc-inlines-were-off.md](retail-obj-externals-prove-mfc-inlines-were-off.md).

    gruntz walls inline-model --gap <rva>

names the delta from the same normalized pair `diagnose` reads and reports this
one-way symbol evidence. Undefined and absent rows are deliberately
**AMBIGUOUS**; settle them from the source declaration, an `/Ob0` census, the
nested helper boundary, and ordered call-site topology.

## The falsifying integration case

`CGruntzMgr::Run` 0x83450 constructs an encrypted Bute stream. The natural
source abstraction is:

```cpp
g_buteMgr.m_stream = new istrstream(decoded, snk->pcount());
```

MSVC's `ostrstream::pcount()` inline calls
`streambuf::out_waiting()`. In our current TU both levels expand, so the base
object has **no `out_waiting` symbol row**. With the constructor argument written
directly as `output->pcount()`, expansion of `out_waiting`'s conditional also
duplicates the destination `istrstream` constructor; the whole `Run` then has
three constructor sites against retail's two. Naming the accessor result first,
`i32 decodedLength = output->pcount()`, restores the two constructor sites and
moves `Run` 88.65 -> 88.77, while the nested call remains expanded. Retail expands
`pcount()` but declines only its nested `out_waiting()` call. The call-set delta is
therefore a distinct later budget decision, not evidence that the accessor or its
destination construction should be hand-expanded.

Charged inline-helper and repeated real-call controls independently confirmed the
budget reading: small additions left `out_waiting` expanded, while enough preceding
inline cost made cl 5.0 emit the call. Those probes are evidence only and must not
remain in source; the retained lever must be authentic missing caller work.

This also explains why a nested-site model matters: the outer accessor can be
accepted while the callee inside it is declined. Do not replace the authentic
accessor with its low-level expansion just because current C2 recursively
inlines both.

## What to do with shape (2)

Find which two arms retail shares and we duplicate (or the reverse), and read
the ONE instruction that makes the tails differ - the merger is exact, so a
single mismatched instruction in the suffix is enough for it to decline.
`CTriggerMgr::PlaceObjectFull` 0x78a50 is still shape (2), but the proof is its
code rather than its undefined `LoadCursorSprites` symbol. Retail shares the
vehicle preview's two
`LoadCursorSprites`, both arms ending `push <arg>` and jumping to a common
`mov ebp,<world>; mov ecx,ebp; call`. Ours materialises `world` in EBP inside
the true arm BEFORE its call and loads the receiver straight into ECX in the
false arm, reloading EBP after - so the suffixes are not identical and cl
correctly emits both.

The terminator lever from
[retail-duplicates-small-return-epilogues](retail-duplicates-small-return-epilogues.md)
works the other direction: giving a group of arms ONE trailing `return` stops cl
cross-jumping their tails together. It moved this same function's
`pfk >= 0xdf` arm from a `push 0x1; jmp <shared>` back to three full sites.

## Why the tool could not be invoked before

`--gap` only accepted a spec JSON of front-end `cb` estimates - numbers nobody
has for a real row - so `--gap 0x08b960` answered `spec JSON missing` while
CLAUDE.md and every matcher brief named it as THE lever for this class. The
address form does the derivable half and deliberately refuses to guess `cb`: a
guessed deficit printed as a model output is indistinguishable from a measured
one. `--measure-cb` still titrates the real number for the spec form.
