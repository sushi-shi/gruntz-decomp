---
description: Run the Gruntz matching campaign as a fan-out pipeline — N matchers always in flight across a reused worktree pool, every result integrated SERIALLY into a single linear main history. No module argument (Gruntz is one binary).
argument-hint: [n-matchers]
allowed-tools: Agent, Bash, Read, Write, Edit, Grep, Glob
---

You are the **parallel match orchestrator** in the top-level session (so you CAN
dispatch subagents). Drive the campaign per **`.claude/agents/orchestrator.md`**
(fan-out pool + serial integration; target cross-check, STOP-EARLY, `@early-stop`,
COMMIT-EACH-MATCHER) and **`.claude/agents/matcher.md`** (the reconstruction doctrine
+ the `permute` skill at walls). There is **no module**
to pick — Gruntz is a single binary; go straight to building the queue.

Pool size / concurrency: **N = $1** (if empty, default **4** — the provisioned
`matcher-1..4` pool). This caps how many matchers run at once; the run processes the
WHOLE queue regardless. **Always keep N matchers in flight** until the worklist is dry
or the user says wind down.

**Builds run cd-first, ideally inside one open shell.** Each worktree is self-contained
(`GRUNTZ_DIR=$PWD` → its own `build/` + wineprefix), so every build/queue command must
`cd <worktree>` (or main) BEFORE entering the shell — `cd <dir> && nix develop .#build
--command gruntz build`, never `nix develop … --command 'cd <dir> && …'` (that resolves
`REPO` to the launch dir). **Prefer keeping a `nix develop .#build` shell open and running
`gruntz build`/status inside it** rather than paying `nix develop` startup per command
(see `docs/build-system.md`).

In short (full rules in the two agent docs):

1. **Pool (persistent, reused across restarts):** the slots are long-lived worktrees
   named **`matcher-1 … matcher-N`** under `.claude/worktrees/`. On startup, for each slot:
   if it already exists, **reuse it as-is** — `git -C .claude/worktrees/matcher-N reset --hard main`;
   its provisioned `build/` survives, so NO cold re-provision. Only **create + build-provision**
   the slots that don't exist yet (`.claude/agents/orchestrator.md` § "Pool setup").
   So a restart of this command does NOT regenerate the pool. Verify one can `gruntz build`
   before dispatching; add/remove slots to match N.
2. **Queue:** regenerate the worklist in a build shell —
   `nix develop .#build --command python3 -m gruntz.match.residual_queue` — then read
   `config/match-queue.md`. **Filter out already-reconstructed RVAs** (orchestrator.md target cross-check: `grep -rlE 'RVA\(0x' src --include=*.cpp | grep -v /Stub/ | xargs grep -ohE '0x[0-9a-f]{8}' | sort -u`),
   and skip anything already `@early-stop`. **Target priority — drain these BEFORE any
   %-recovery of already-matched functions:** (1) the `@stub` backlog (`src/Stub/` —
   biggest files first: ApiCallers, Backlog, Discovered, then the per-class tail), and
   (2) the `(unmatched)` bodies (the `engine_unmatched` FUN_ unit). Only once BOTH are dry
   do you climb the `@early-stop` near-misses. Within a tier, leaf/middle-small first. For a
   `@stub`, the matcher reconstructs the body to exact AND re-homes it into its real class
   TU (deleting the emptied stub file) so `src/Stub/` shrinks toward empty — but ALWAYS
   reproduce the body before moving an RVA (a bare move can silently destroy a match; in
   ApiCallers/Backlog touch `@stub` only, never the real bodies).
3. **Fan out:** keep N background matchers (`subagent_type="matcher"`, `run_in_background: true`,
   **NOT** `isolation: worktree` — you own the pool). **Batch ≥20 related functions per
   matcher** (matcher cost is ~flat regardless of batch size — bigger batches = more yield
   per dispatch; if one TU/cluster can't supply 20, extend to a sibling TU to reach it).
   Each prompt: absolute worktree path +
   `cd` there first, absolute paths everywhere, the target RVAs/names/sizes/file, the 8-digit
   address convention, STOP-EARLY + `@early-stop`, **forbid `gruntz format` in the worktree**,
   allow stub→real-TU migrations, and report the final % + a one-line summary + the full
   `git diff`. **Lane discipline:** route all targets of one multi-stub file through ONE slot
   (avoids same-file integration collisions); other slots take distinct-file targets.
   **Matchers never give up / never abandon a target:** if a function hits an unclimbable
   codegen wall, the matcher STILL commits/leaves its highest-achievable-% reconstruction
   marked `// @early-stop` (with the wall reason) — zero output is never acceptable. Every
   dispatched function comes back at 100% or a maximized `@early-stop`; "I couldn't finish"
   means "I banked my best %", not "I produced nothing".
4. **Integrate SERIALLY (the heart):** process completed matchers one at a time —
   guard main clean → apply only that matcher's file(s) → `gruntz build` → confirm % →
   `gruntz status update` (`--accept-regressions` only for a migration's LOST stub or
   trivial cross-fn fuzzy drift) → commit ONLY those files + `config/match_baseline.tsv`
   as `match: <fn> -> <result>` with the Co-Authored-By trailer — **always include the
   build-refreshed `README.md` stats block in that commit** (`gruntz build` regenerates it;
   never skip or batch-away the refresh, so README always matches HEAD). One matcher = one commit.
   **ALWAYS integrate the matcher's banked work — never hold or revert for a small net-exact
   regression.** When a body added to a shared aggregate TU (e.g. `engine_label_stubs`/`All.cpp`)
   reshuffles neighbors and knocks one off 100% (aggregate codegen-leak), still integrate it and
   `gruntz status update --accept-regressions`. The **best-%/Fuzzy Max** baseline column retains
   each function's prior high, so nothing is truly lost — that metric exists precisely to capture
   these cases. **Net % movement (recovering such neighbor regressions, usually by re-homing the
   new body out of the shared aggregate into its own unit) is deferred to the end-of-campaign
   final sweep, once ALL functions are reconstructed.** Note the regression in the commit message;
   reconstructing/banking a stub body takes priority over protecting a neighbor's current exactness.
   Never integrate two at once (one `build/`, one HEAD). **Refill immediately:**
   `git -C .claude/worktrees/matcher-N reset --hard main` (its `build/` survives), pick
   the next target, dispatch.
5. **Run to exhaustion.** The campaign is NOT done until BOTH backlogs are drained:
   `rg -c '@stub' src/Stub` → **0** (every stub reconstructed AND re-homed/deleted so
   `src/Stub/` is empty) AND the `(unmatched)` row → **0**. Keep N matchers in flight,
   integrating + refilling each slot the moment its matcher lands, until then. Post a
   progress line per wave (stub count + unmatched count shrinking). **Stop** only when both
   are dry/parked or the user winds down: let in-flight matchers finish, integrate them, then
   print the ledger (`fn -> result -> commit`) + a regressions summary. **Leave the
   `matcher-N` worktrees in place** so the next run reuses their build state; `git worktree
   remove` only if the user asks.
6. **Phase 2 — improve general % toward 100% (only AFTER step 5 is fully dry).** Once
   `@stub`→0 and `(unmatched)`→0, re-attack the deferred walls to drive the whole engine to
   byte-exact. Worklist: `rg '@early-stop' src` — each carries its asm-level wall reason per
   **`docs/wall-instructions.md`**, so concentrate on the RECOVERABLE walls (e.g. re-home an
   aggregate body that knocked a neighbor off 100% into its own unit; fix a pipeline reloc-name
   artifact) and skip the genuinely-unsteerable. Keep going until the match reaches 100%. Do
   NOT start this phase while any stub/unmatched remains — stubs/unmatched come first.

Wall doctrine: when a matcher (or you, integrating) sees a % move — up OR down — from an
UNRELATED change, document the trigger + the assembly-level mechanism per `docs/wall-instructions.md`.
That corpus is what makes Phase 2 (and every `@early-stop`) fast and evidence-based.

Keep your context SMALL: hold only the ledger — never pull a matcher's disassembly,
diffs, or source into this session beyond the file(s) you integrate.
