# A 0.00% objdiff row is not always an unwritten body: cross-jump grouping floors the score

**Tags:** `cpp:switch` `cpp:control-flow` | `topic:wall` `topic:scoring-artifact`
**Confidence:** 10/10 — `CGameObject::Play` 0x151150, verified instruction-by-instruction
against retail, a 192-variant wall-breaker run, and a source-shape correction that moved
the same body from 0.0000% to 92.9655%.

## Symptom

A function reads **0.0000%** in `gruntz sema match` / `report.json` and therefore looks
like a `@stub` — an empty body nobody has reconstructed yet. It is not. The body is
complete, and `objdiff-cli diff` shows the two sides plainly aligned at the top:

```
left  n 224 {'NONE': 45, 'DIFF_REPLACE': 3, 'DIFF_ARG_MISMATCH': 14,
             'DIFF_INSERT': 79, 'DIFF_DELETE': 83}
```

45 instructions match exactly, including the whole prologue. objdiff's cost model
charges INSERT and DELETE at full instruction weight, so once ~70% of the stream is
insert/delete the normalized score saturates at zero. **0.00% means "the streams do not
align", not "there is no code".**

## Mechanism

Four `switch` arms each end in the same shape — notify a worker, then conditionally
restore a saved field:

```cpp
case SERIAL_PRESAVE:
    ...
    saved = w->m_actKey;
    w->SetWorkerAct(ACT_PREPARE_SAVE);   // -> mov ebx, 0x50
    m_animWorker->m_notify(this);
    w = m_animWorker;
    if (w->WorkerAct() == ACT_PREPARE_SAVE) { w->m_actKey = saved; }
    break;
```

The act id lives in `ebx`, so after the `mov ebx, 0x5N` **the post-call suffix of every
arm is byte-identical**. cl's cross-jumper is free to merge any subset of them, and the
subset it picks decides the whole layout, because whichever arm is left falling through
gets the shared tail placed immediately after it.

| | merged group | falls through to the tail | tail lands at |
|---|---|---|---|
| retail | C + D | A (PRESAVE) | the MIDDLE, +0x73 |
| previous candidate | A + B + C | D (POSTLOAD) | the END, +0x13e |

One grouping decision, and every subsequent block is at a different offset with a
different jump direction (retail's arms jump *backwards* to the tail, ours *forwards*).
That is the 79 inserts and 83 deletes. Retail could not merge A with B because their
notify calls load the vtable through different registers (`edx` vs `eax`) — it merged
C and D, which both use `ecx`.

## What this is NOT

Before parking one of these, rule out the things that DO look like this and are real:

- **A wrong `switch` domain.** Read retail's jump table and check the case→arm map.
  Here retail's six entries are `A, B, TAIL, TAIL, C, D`, which says the arm values are
  3, 4, 7, 8 with 5 and 6 falling to the default — and `SerialMode` in
  `include/Gruntz/SerialArchive.h` already spells exactly that. A mismatch here would be
  a genuine behaviour defect (arms firing on the wrong mode), not a wall.
- **A wrong `RVA()` size.** `0x190` here is right: 373 B of code, 3 B of alignment, then
  the 24-byte jump table, all inside the COMDAT.
- **A delinker duplicate symbol.** A name that is BOTH undefined and defined in the same
  target obj pairs against the size-0 undefined copy and scores 0. `canonicalize_coff`
  already renames those to `$dup$…`; confirm the normalized obj, not
  `build/objdiff/target/`, when checking. Measured 2026-08-10: exactly one function in
  the tree hits the duplicate, and it is repaired before objdiff sees it.

## Steerability

The original conclusion that this instance was unsteerable was false. `gruntz permute
variants --state-trials 48 --max-depth 2 --limit 192` did produce **192 candidates that
all compiled to the identical 380-byte body**, but that sweep searched declaration and
parser-state axes while the missing lever was the function's real CFG.

Retail has only three notify call sites: LOAD and POSTLOAD share one notification tail.
Express that join explicitly, let PRESAVE fall through to the dispatch block, and spell
POSTLOAD's worker check as the positive gate whose false edge reaches the final failure
epilogue. Those three structural facts move dispatch to retail's middle position, give
candidate and retail the same 139 instructions, 14 conditional branches, five returns,
and ordered referents, and raise the ordinary build from 0.0000% to 92.9655% without a
state probe.

The remaining residue is narrower: retail keeps lookup-failure and zero-id carrier
stores distinct and assigns the shared tail's saved value/action to `edi`/`ebx`; the
candidate merges the two zero stores and uses `ebx`/`edi`. Treat that as a local CFG and
register-lifetime problem. The general rule is: a cross-jump grouping wall proves the
stream rotation, but does **not** prove that parser state is the only remaining lever.
Recover shared call sites, fallthrough ownership, and failure-epilogue placement first.
