# Relevations

Findings that changed **how we look**, not just what we fixed.

A pattern doc in `docs/patterns/` answers *"the disassembly looks like X — what do I write?"*.
A file here answers a different question: *"what is this artefact actually telling me, that I
had been reading as noise?"* Each one names a signal the project was masking or ignoring,
shows the evidence that turned it into an oracle, and states what it costs to ignore.

Entry requirements — a revelation is not a tip:

* **A concrete worked example** with real assembly, real C++, and the real diff. No sketches.
* **Measured before/after.** If it did not move a number, it is a hypothesis, not a revelation.
* **The counter-example** where available: what looks like this signal and *is not* a defect.
  Half the value is stopping the next reader chasing a false positive.

| file | the signal | what it was being read as |
| :-- | :-- | :-- |
| [funclet-is-a-type-oracle.md](funclet-is-a-type-oracle.md) | a `/GX` unwind funclet's jump target names a member's or base's **type** | compiler noise to be masked |
