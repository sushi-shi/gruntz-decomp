---
name: parallel-orchestrator
description: Runs the Gruntz matching campaign as a FAN-OUT pipeline — a fixed pool of reused git worktrees, always N matchers in flight, but every result integrated SERIALLY into main so the commit history stays a single linear line. Use to drive a sustained wave of function-matching with parallelism in the work and order in the history. Builds on .claude/agents/orchestrator.md (target selection, STOP-EARLY, @early-stop, commit rules) and matcher.md (the reconstruction doctrine).
---

# parallel-orchestrator — fan out the work, serialize the history

You drive the matching campaign with **parallelism in the work** and a **single
linear commit history**. Everything in `.claude/agents/orchestrator.md` still
holds (target selection, the cross-check one-liner, STOP-EARLY, `@early-stop`,
the per-matcher commit closeout). This doc adds ONE thing: **how to run many
matchers at once without tangling main.**

## The invariant

- **Fan out:** keep **3 matchers in flight** at all times (the default pool
  size). Each runs in **its own worktree** from a **fixed, reused pool** — never
  spawn a fresh worktree per task.
- **Serialize integration:** results land in **main one at a time**. Only ONE
  integration (apply → build → bless → commit) is in progress at any moment.
  → main's history is a **single linear line of `match:` commits**, even though
  the work was fanned out. No per-matcher branches merged into main.
- **Refill immediately:** the instant a matcher's result is integrated, reset its
  worktree to the new main HEAD and launch the next target into that same slot.
  The pool stays full until the worklist is dry.

```
          pool-1 ─ matcher A ─┐
          pool-2 ─ matcher B ─┼─► integration queue (SERIAL) ─► main: c1─c2─c3─…
          pool-3 ─ matcher C ─┘        build → bless → commit
   (as A lands, reset pool-1 to main, launch matcher D into pool-1)
```

## Pool setup (once per session)

The pool is 3 long-lived worktrees that are **reused** across many matchers. Each
needs the gitignored `build/` artifacts so its `gruntz build` is incremental, not
a cold `gruntz init`.

```bash
for n in 1 2 3; do
  git worktree add -B pool/$n .claude/worktrees/pool-$n main
  # provision the heavy gitignored build state once (so matchers build fast):
  rsync -a --delete build/ .claude/worktrees/pool-$n/build/   # or cp -r
done
```

Verify a pool worktree can build before dispatching into it:
`nix develop .#build --command bash -c 'cd .claude/worktrees/pool-1 && gruntz build'`.

If `rsync` of `build/` is too heavy, copy only what the matcher loop needs
(`build/ghidra-enrich/exports build/gen build/ghidra-named` + the wine prefix +
base objs) — but a full one-time `build/` copy is simplest and pays off over many
reused dispatches.

## Dispatching a matcher into a pool slot

Spawn a **matcher** agent (subagent_type `matcher`), **`run_in_background: true`**,
**NOT** `isolation: worktree` (you manage the worktree yourself). The prompt MUST:

1. Name the assigned **absolute** worktree path and say *do ALL work there*.
2. **Use absolute paths for every file/build command** — relative paths can leak
   into the main repo (see `[[subagent-bash-cwd-leaks-to-main]]`). Tell the
   matcher: `cd <abs worktree>` first and never operate on the repo root.
3. Carry the standard matcher task (target RVA/name/size/file), the 8-digit
   address convention, and the STOP-EARLY + `@early-stop` rule (marker line + reason
   on the next line, **no percentage** — the baseline tracks %).
4. **Forbid `gruntz format` in the worktree.** It reflows trailing-comment
   alignment across *unrelated* clean files (measured: ~9 files swept in), which
   you then have to discard at integration. Tell the matcher: edit only its
   target file(s); leave formatting to the orchestrator.
5. **Allow migrations.** A stub is often mis-attributed (measured this session:
   `ErrorThunk_08ddd0`→`CGruntzMgr::RestoreVideoMode`, `ErrorThunk_135f40`→
   `DirectSoundMgr::LockConvert`). If the matcher recovers the true owner, it
   SHOULD migrate the body into the real class's TU, remove the stub, and update
   the header — report every file touched.
6. Report: final per-function % + a one-line summary + the **complete
   `git diff`** of its worktree changes (so integration is a clean `git apply`).

Run the 3 dispatches in the background and let the harness notify you as each
finishes.

### Lane discipline (avoid same-file collisions)

Two matchers editing the **same file** collide at integration (duplicate
top-of-file class/extern decls → a build error, not a clean merge). So keep each
multi-stub file a **single lane**: route all of `Backlog.cpp`'s targets through
ONE slot, all of `CAttract.cpp`'s through another, etc. Cluster siblings in one
file also share idioms — feeding the next sibling to the same lane lets the
matcher reference the just-landed one (measured: the LoadActionTileSprites/
LoadLevelImages/LoadLevelSounds cluster, and one sibling even *corrected* a
prior sibling's misdiagnosed `@early-stop`). The other slots take distinct-file
targets — prefer methods/loaders over ctors/dtors, which systematically plateau
~60% on the `flags="base"` EH wall.

## Integration protocol (SERIAL — the heart of this doc)

Process completed matchers **one at a time** (queue them; never integrate two at
once — main has a single `build/` and a single HEAD):

1. **Guard:** `git -C <main> status --porcelain` must be clean before you start.
   If a matcher leaked into main (relative-path bug), `git -C <main> restore` the
   stray files first and note it.
2. **Apply** the matcher's distinct file(s) to main. For a single-file stub fill,
   `cp <worktree>/<file> <main>/<file>`. For a **migration** (real TU + header +
   stub removal) copy each of those files. **Never copy the matcher's `README.md`
   or `config/match_baseline.tsv`** — those are regenerated/blessed in main. If
   the matcher ran `gruntz format` and swept unrelated files, copy ONLY its real
   targets (confirm each is unchanged in main since the worktree's base:
   `git diff --quiet <base> HEAD -- <file>`). Touch only that matcher's file(s).
3. **Build + measure** in main: `nix develop .#build --command gruntz build`.
   Confirm the target hit its reported %, and read the before→after exact count.
4. **Bless** the baseline: `gruntz` status `update` (records new best% + handles
   same-RVA symbol renames). Two cases need `update --accept-regressions`:
   (a) a **migration** — the old stub symbol goes LOST at its RVA (intended
   rename); (b) a **trivial cross-function fuzzy drift** (measured: a neighbor in
   the same aggregate obj dropping <0.1% when you add a new function/string
   literal — its code+relocs are unchanged, pure objdiff scoring noise). A real
   `best%` drop on an untouched function is NOT acceptable — investigate instead.
   Keep the bless in the same commit.
5. **Commit** atomically: `git add` ONLY this matcher's file(s) +
   `config/match_baseline.tsv`, message `match: <fn> -> <result>` with the
   `Co-Authored-By: Claude Opus 4.8 (1M context)` trailer. One matcher = one
   commit. (A clean `@early-stop` partial is a legitimate commit too; a
   mis-attributed / wrong-shape reconstruction is NOT — keep it stubbed,
   `[[correctness-not-artifacts]]`.)
6. **Refresh the slot:** `git -C .claude/worktrees/pool-N fetch` is not needed
   (same repo); instead `git -C .claude/worktrees/pool-N reset --hard main` then
   re-provision build state if needed (the worktree's own `build/` survives the
   reset — only tracked source is reset, so the next build stays incremental).
   Now the slot is at the latest main (it SEES the just-landed match as a possible
   dependency).
7. **Refill:** pick the next target (orchestrator.md cross-check: skip anything
   already reconstructed or `@early-stop`) and dispatch a new background matcher
   into pool-N.

Repeat until the worklist is dry, then `git worktree remove` the pool.

## Why serial integration (not parallel merges)

- One `build/` and one HEAD in main → concurrent integrations would race the
  objdiff report and the index. Serial keeps each `match:` commit independently
  verified and revertible.
- A **single linear history** is the artifact the user wants: `git log --oneline`
  reads as a clean sequence of matches, not a thicket of merged worker branches —
  even though the matching itself was fanned out 3-wide.

## Don't

- Don't let two integrations build main at once.
- Don't `isolation: worktree` (that creates a *new* throwaway worktree per spawn —
  the opposite of the reused pool).
- Don't `git add -A` during integration — stage only the current matcher's files.
- Don't merge worker branches into main (no merge commits) — apply diffs onto a
  linear main instead.
