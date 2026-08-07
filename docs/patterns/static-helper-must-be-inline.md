# A `static` (non-`__inline`) helper cl emits out-of-line where retail inlined it

**Symptom.** A large, contiguous hunk of retail code has no counterpart in the
base at all — the base has a single `call <tgt>` where retail has a whole loop
or straight-line expansion. Downstream of that point the two bodies also
disagree on *which register holds 0*, on `test cl,cl` vs `cmp cl,bl`, and on
partial-register choices, so the diff looks far worse than "one missing inline".

**Cause.** The reconstruction factored a repeated idiom into a file-local helper
and spelled it `static void Foo() { ... }`. MSVC 5.0 at `/O2` does **not**
auto-inline a plain `static` function; it emits it and calls it. Retail's source
had the same helper marked `__inline` (or wrote the idiom out at each site), so
retail's caller carries the expansion — and, crucially, the expansion's extra
live values change the caller's whole constant/zero-register allocation.

## Fix

Mark the helper `static __inline`. That is all.

```cpp
-static void GruntScratchTeardown() {
+static __inline void GruntScratchTeardown() {
     CString* slot = g_typeColl.Slots();
     i32 cnt = g_typeColl.m_grown;
     while (cnt--) {
         if (slot != NULL) { slot->~CString(); }
         slot++;
     }
 }
```

## Evidence

The CString scratch-teardown loop is defined five times across `src/Gruntz`.
Three copies were `static __inline`, two were plain `static`. Flipping the two
moved five functions in one build:

| function | before | after |
|---|---|---|
| `CGrunt::StepArrivalCommit` | 3.16 | 79.85 |
| `CGrunt::RunEntranceMove` | 81.2 | 89.83 |
| `CGrunt::LoadWingzGruntSprites` | 76.63 | 84.49 |
| `CGrunt::StepCombatReaction` | 74.14 | 78.59 |

`StepArrivalCommit` at 3% is the tell worth remembering: a missing inline in a
helper called once from a small function can look like a completely wrong
reconstruction.

## How to spot it

`gruntz sema disasm <rva> --blocks --diff --lite` shows a base block count far
BELOW retail's with a `!!` kind mismatch where the loop should be, and the flat
`--diff` shows one `call` on the base side against 10-20 retail instructions.
Then grep the TU for `^static ` function definitions and check each against a
sibling TU that already declares the same helper `__inline`.

The dual failure also exists: retail *calls* a function we inlined by hand.
`gruntz sema xref <helper-rva> --depth 1` enumerates exactly which retail
functions call it, so the two directions are decidable per site rather than by
taste. (`FreeNodePool::Push` at 0x000311b0 is the live example — 93 hand-inlined
free-list splices in the tree, and xref names the ones that must be calls.)
