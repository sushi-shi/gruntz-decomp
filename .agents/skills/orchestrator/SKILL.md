---
name: orchestrator
description: Run the Gruntz matching campaign with parallel workers - pick disjoint lanes, launch matcher workers in the reusable worktree slots, run each lane through plain matching then holista then structural passes, and merge every result into main through the merge-preparation procedure (rebase, build, bank, gates, squash-merge). Use when asked to start, steer, or merge matching workers, to push a module or score band toward 100%, or to integrate agent branches.
---

# orchestrator — run the workers, merge the results

You plan lanes, launch workers, relay rulings, and merge. Workers do the
matching; `AGENTS.md` and the `matcher`/`holista` skills are their guide. Keep
the user informed with short, concrete reports: functions closed, MAX changes,
what is open and why.

## Slots and branches

- Three reusable worktrees: `../gruntz-match-1`, `-2`, `-3`, parked detached at
  `origin/main` between lanes, each with its own `build/`. Add a temporary slot
  only when the user asks for more workers, and remove it after its merge.
- Origin holds only `main` and `agent-mathy-local-audit` (PR #72). Lane
  branches are local until their merge PR; delete them after merging.
- Prepare a slot: `git -C <slot> checkout -B <lane-branch> origin/main`, then
  `gruntz build` (use `--reconfigure` if the slot last built another branch).
  A new slot copies `build/` from a built checkout and runs one build.

## Lanes

- Lanes are disjoint: split by module (README table) or by HIST band, never
  overlapping another running lane. List the lane's functions (rva, name, MAX)
  from `config/match_baseline.tsv` in the prompt.
- Good lanes: modules close to 100%, the lowest-HIST band, `HIST > MAX` lost
  matches, a `docs/todos/` task, a holista sweep over random functions.

## Worker prompt

Launch a `general-purpose` Opus subagent, or a separate `claude -p` session in
the slot when this session's project instructions are stale. The prompt holds
only: the slot path and branch, "read AGENTS.md first; it takes precedence",
the skill to start with, the goal and lane list, what other lanes own, and:
do the work yourself, commit on the branch as you go, no push or PR, no
per-step verification, report per function the fix and what is left.
Never add verification requirements (gates, tests, behaviour proofs) to a
worker prompt: checks run once, at merge.

## Passes

1. **Plain matching** (`matcher` skill).
2. When the worker returns with walls: merge its work, then resume the same
   worker (it keeps its context) on the open functions with the `holista`
   skill: inline helpers, accessors, macros, scoped blocks.
3. Still stuck: resume for a **structural** pass (types and ownership, API
   level, function boundaries, control structure, surviving sources), not
   permutation. Stop when the worker reports no open structural lead.
   Before calling a wall permuter-flat, check which `gruntz permute` probe
   families actually ran; one flat family proves nothing.

## Merging a lane

In the lane's slot:

1. `git fetch origin && git rebase origin/main`. For a `README.md` conflict,
   take main's side; the build regenerates it. For a
   `config/match_baseline.tsv` conflict, keep the lane's raised rows: a MAX
   banked under a removed TU-state probe cannot be re-banked from the current
   source. After the rebase, confirm the lane's reported MAX values are in the
   baseline before banking.
   If another merge already landed the same fix, drop the duplicate commit
   (or cherry-pick only the lane's unique commits onto `origin/main`).
2. `gruntz match <edited units>` records byte-neutral MAX resets into
   `docs/todos/syntactic-recovery.tsv`; commit that file if it changed.
3. `gruntz build`, `gruntz verify bank`, commit the baseline and README.
4. `gruntz build verify`: only `REGRESS` (an edited function whose score
   fell) blocks; `RESET` and `DIP` are informational. Run
   `gruntz verify selftest` only if the lane touched `scripts/`.
5. Push, open a PR that lists each function's change, squash-merge, delete the
   remote branch, park the slot at `origin/main`.

Never commit or build in a half-finished rebase.

## Ledgers and rulings

- A 100% match that breaks a project rule, or a rule-breaking spelling
  retail's bytes require, is kept: gate allow entry plus a row in
  `docs/todos/rule-exceptions.tsv`. Never leave such a decision unrecorded.
- Byte-neutral edits that lowered MAX are rows in
  `docs/todos/syntactic-recovery.tsv`; lanes do not chase them unless that
  file is their task.
- Relay user rulings to every running worker. When a tooling fix lands on
  main that affects running lanes (e.g. a gate fix), send each worker the
  commit to cherry-pick.
- Treat worker claims about tools as leads: if a worker avoided a correct
  change because a gate or tool objected, check the tool. A wrong gate steers
  every worker.

## PR #72 (`agent-mathy-local-audit`)

A long-lived source of helpers that main takes piecemeal; never merge it.
When it conflicts with main, merge `origin/main` into it: keep its model
(field aggregation, its helper spellings) and re-apply main's
model-independent match wins on top; take main's generated files; compare
the result against a build of its own head, not its stale ledger; refresh
its README (`gruntz verify readme`); push.
