# An inlined multi-return helper duplicates its caller continuation

tags: cpp:inline cpp:return cpp:local cpp:rand | asm:ret asm:idiv asm:mov | topic:codegen-idiom topic:cfg topic:regalloc
symptoms: a hand-expanded range/selector has the right algorithm and branches, but retail repeats the caller's following stores at each helper return and carries each selected value through one result register
confidence: 10/10

## Signature

A caller computes a value through a small selector with several returns, then
performs the same stores with that result. Retail may duplicate those caller
stores into every inlined return path. A hand transcription of the selector in
the caller can have the same semantics and branch count while C2 instead hoists
or merges part of the continuation.

`CRandomAmbientSound::InitCycleTiming` is the controlled example. Retail contains
the complete closed-range random helper topology:

- `range = hi - lo + 1` and a zero-range split;
- one inlined `GetRandomNumber()` in the endpoint coin-flip arm and another in
  the modulo arm;
- the selected endpoint copied through EDX;
- `m_playPhase = true; m_countdownMs = result` repeated in both endpoint arms
  and once after the modulo result.

The hand-expanded source scored 75.6923%, emitted 74 instructions/0xd6 bytes,
and stored `m_playPhase` only twice because C2 hoisted the equal endpoint-arm
store. Restoring a TU-local `RandRange(i32 lo, i32 hi)` overload using
`GetRandomNumber()`, then spelling the caller as a named result followed by the
two stores, reached 79.6795% and 77 instructions/0xe1 bytes against retail's
78/0xe5. Calls, branches, returns, ordered relocations, and all ten stores are
now exact; semantic diff retains only retail's `lea; test` versus base `inc`
and the associated whole-body register rotation.

## Why the abstraction matters

The helper's return is not merely its arithmetic. Inlining connects each return
edge to the caller continuation before C2 performs tail merging. Transcribing
the helper body directly gives the optimizer a different graph and different
value identities, even when the final source repeats the same assignments.

This is the control set on the same function:

- changing the hand transcription to an early-return first arm was byte-flat;
- one shared result plus a common tail fell to 71.0897% and merged three returns
  to two;
- one shared result with textually duplicated tails returned byte-for-byte to
  the 75.6923% baseline;
- a separate setter helper, helper ownership in the shared header versus this
  TU, const locals, an initialized result, and two-step range expressions were
  byte-flat on the restored range-helper state;
- a 34-state target-adjacent C1 forest found one compiler island, all at
  79.6795%.

## Reverse use

When retail repeats a caller-side continuation after multiple arms and every
arm carries its value through the same result register, look for a small
multi-return inline helper before manually duplicating the stores. Strong
corroboration is recurrence of the complete selector algorithm or an existing
helper family with a different backend, as here: the TU already had a
`RandRange(CGruntzMgr*, lo, hi)` inline, while this site uses the same closed
range policy with the literal surviving `GetRandomNumber()` LCG.

Restore the helper call as one source statement and compare the complete
continuation topology. Do not require an out-of-line call or symbol: under
`/Ob1` the boundary can disappear completely while still determining C1's
return edges and result identity.
