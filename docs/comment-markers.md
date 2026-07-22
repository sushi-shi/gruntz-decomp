# Comment markers — the blessed vocabulary

Every ADDRESS/SYMBOL binding is a macro from `include/rva.h` (`RVA`, `DATA`,
`VTBL`, `VTBL_ABSENT`, `SIZE`, `SIZE_UNKNOWN`, `SYMBOL`, `RVA_COMPGEN`,
`DATA_SYMBOL`) — no label ever lives in a comment. Comments carry exactly one
other kind of machine-visible content: **state markers**, a `// @name` leading a
comment line. The vocabulary is CLOSED and gated (`gruntz.audit.label_style`,
FATAL in the build tail): a comment line leading with any other `@name` fails the
build — ad-hoc markers rot into pseudo-conventions no tool reads (the 2026-07-22
sweep retired `@orphan`, `@flag`, `@fold-TODO`, `@identity-recovered`,
`@emission-TODO`, `@reloc-TODO`, `@name-conflict`, `@undefined-data` into plain
prose). Mid-line `@name` mentions are prose and stay free.

| Marker | Read by | Meaning |
|---|---|---|
| `// @stub` | `gruntz.match.verify_stubs` (gate) | An empty, not-yet-reconstructed body. The contiguous `// @tag:` lines directly above it form its evidence block; `@confidence:` and `@source:` are REQUIRED. |
| `// @confidence: high\|med\|low` | `verify_stubs` (required stub tag) | How solid the stub's identity evidence is. |
| `// @source: <evidence>` | `verify_stubs` (required stub tag) | The identity evidence itself (`xref`, `vtable-slot`, `rtti-vptr`, `winapi:<fns>`, `string-xref`, …). |
| `// @early-stop` | `gruntz.audit.stale_walls`; the final-sweep worklist is `rg '@early-stop' src` | A COMPLETE reconstruction parked below 100% match. The reason goes on the next comment line (regalloc wall, scheduling wall, …). |
| `// @identity-TODO` | `stale_walls`, doctrine in `CLAUDE.md` | An unproven class/owner identity — leave it, never fabricate. State what was tried / what would prove it. |
| `// @interleaver <sym> …` | none (structured record) | A linker-pooled out-of-line member emitted INSIDE another unit's contribution range (see `docs/` interleaver notes + the `interleavers` tool that detects them from layout). Records the placement proof at the site. |

Rules of use:

- A marker LEADS its comment line (`^\s*// @name`); everything after it on the
  line is free prose.
- `@stub` / `@early-stop` / `@identity-TODO` are the three FUNCTION-STATE
  markers (`CLAUDE.md`): a reconstructed method is either ~100% (unmarked) or
  `@early-stop`; a stub is `@stub`; an unproven identity is `@identity-TODO`.
- Anything else you are tempted to tag — a TODO, a recovered identity, an
  orphan note, a naming conflict — is PROSE. Write the fact in words. The
  vocabulary is meant to stay this size: 99 times out of 100 the answer to the
  gate firing is "delete the @, write prose", not a new entry here.
