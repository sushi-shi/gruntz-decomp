# A wall review's count certification is a property of (source x TU state), not of the source
tags: topic:wall topic:tooling topic:regalloc | topic:scoring-artifact
symptoms: walls recheck BROKEN on a `current` row; src_hash unchanged; base branch/instruction count moved with no source edit; the same function sits below its banked MAX
confidence: 9/10

`gruntz walls recheck` re-measures every count a wall review asserts. Two kinds of
row break. A **STALE** row broke because the body was edited - expected, re-review
it. A **`current`** row broke with the per-function `src_hash` unchanged, which
looks impossible and is not: the fingerprint hashes the FUNCTION's source extent
and nothing else, so TU composition is deliberately outside it. Any later edit
elsewhere in the same TU (or in a header it includes) can rotate cl 5.0's register
allocation and block placement and move the emitted branch/instruction counts for
untouched source. The review's "base and retail agree at N" was true of the
(source x TU state) pair it was measured on, not of the source alone.

```
  0x051c00 StepCompassMove   certified 129 branches both sides, now base 131  (bank 63.30, cur 58.48)
  0x0b1ee0 CSpotLight::Update certified  90 insns   both sides, now base  91  (bank 78.94, cur 74.27)
```

Detection signature, and it is exact: the row is `current`, the broken quantity is
a COUNT (not a referent), and the same rva appears in the MAX ledger's carried
below-bank list. Both readings are the same event - the review sees the shape
move, the ledger sees the score move. Neither is a defect and neither is a
regression to investigate; re-certifying the review against today's pair is the
normal maintenance act, exactly as re-banking is for the score. State the
TU-state dependence in the new evidence so the next reader does not chase it.

Corollary for authoring a review: prefer certifying quantities that survive TU
rotation (call multiset, `ret` count, ordered relocation count, referent set) and
write a branch or instruction count as a stated divergence rather than an
agreement when the row is already below its bank. Measured 2026-08-23 on the first
`recheck` sweep: 289 claims, 285 held, and 2 of the 4 breaks were this mechanism.

**STALE has TWO causes, and the `cpp:` prefix is what tells them apart.** A row
whose recorded fingerprint is a real per-function hash goes STALE only when that
function's own extent changed. A row whose fingerprint starts with `cpp:` is the
UNIT-LEVEL FALLBACK - clangd could not resolve that function, so the hash is the
whole `.cpp` - and it goes STALE on *any* edit to the file, including a comment
nowhere near the function. Do not reason about one from the other; the two rows
below were measured under the same one-line comment inserted at line 1 of
`GruntzCmdMgr.cpp`:

```
  triggermgr/PlaceObjectFull          f96c455cab0e  ->  f96c455cab0e   per-function, unmoved
  gruntzcmdmgr/BuildRockBreakInGameText  cpp:8303dec3ac24 -> cpp:cd876c3a8779   fallback, moved
```

So "a comment above an `RVA()` pin cannot make a review STALE" is true for a
per-function row and FALSE for a fallback row, and a proof on one is not a proof
about the other. `fingerprints.real_edit` already encodes this - it calls a
difference a genuine source edit only when BOTH sides are non-fallback.
