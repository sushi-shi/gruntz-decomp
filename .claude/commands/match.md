---
description: Run the Gruntz matching campaign as a fan-out pipeline — N matchers always in flight across a reused worktree pool, every result integrated SERIALLY into a single linear main history. No module argument (Gruntz is one binary).
argument-hint: [n-matchers]
allowed-tools: Agent, Bash, Read, Write, Edit, Grep, Glob
---

You are the **parallel match orchestrator** in the top-level session (so you CAN
dispatch subagents). Drive the campaign per **`.claude/agents/parallel-orchestrator.md`**
(fan-out pool + serial integration) on top of **`.claude/agents/orchestrator.md`**
(target selection, attribution reasoning, STOP-EARLY, `@early-stop`, COMMIT-EACH-MATCHER)
and **`.claude/agents/matcher.md`** (the reconstruction doctrine). There is **no module**
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
(see `.claude/agents/orchestrator.md` § build-loop).

In short (full rules in the two agent docs):

1. **Pool (persistent, reused across restarts):** the slots are long-lived worktrees
   named **`matcher-1 … matcher-N`** under `.claude/worktrees/`. On startup, for each slot:
   if it already exists, **reuse it as-is** — `git -C .claude/worktrees/matcher-N reset --hard main`;
   its provisioned `build/` survives, so NO cold re-provision. Only **create + build-provision**
   the slots that don't exist yet (`.claude/agents/parallel-orchestrator.md` § "Pool setup").
   So a restart of this command does NOT regenerate the pool. Verify one can `gruntz build`
   before dispatching; add/remove slots to match N.
2. **Queue:** regenerate the worklist in a build shell —
   `nix develop .#build --command python3 -m gruntz.analysis.gen_match_queue` — then read
   `config/match-queue.md`. **Filter out already-reconstructed RVAs** (orchestrator.md §2
   cross-check: `grep -rlE 'RVA\(0x' src --include=*.cpp | grep -v /Stub/ | xargs grep -ohE '0x[0-9a-f]{8}' | sort -u`),
   and skip anything already `@early-stop`. Order leaf/middle-small first.
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
4. **Integrate SERIALLY (the heart):** process completed matchers one at a time —
   guard main clean → apply only that matcher's file(s) → `gruntz build` → confirm % →
   `gruntz status update` (`--accept-regressions` only for a migration's LOST stub or
   trivial cross-fn fuzzy drift) → commit ONLY those files + `config/match_baseline.tsv`
   as `match: <fn> -> <result>` with the Co-Authored-By trailer. One matcher = one commit.
   Never integrate two at once (one `build/`, one HEAD). **Refill immediately:**
   `git -C .claude/worktrees/matcher-N reset --hard main` (its `build/` survives), pick
   the next target, dispatch.
5. **Stop** when the queue is dry/parked or the user winds down: let in-flight matchers
   finish, integrate them, then print the ledger (`fn -> result -> commit`) + a regressions
   summary. **Leave the `matcher-N` worktrees in place** so the next run reuses their build
   state; `git worktree remove` only if the user asks.

Keep your context SMALL: hold only the ledger — never pull a matcher's disassembly,
diffs, or source into this session beyond the file(s) you integrate.
