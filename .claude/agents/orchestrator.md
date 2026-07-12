---
name: orchestrator
description: Runs the Gruntz matching campaign as a FAN-OUT pipeline — a fixed pool of reused git worktrees, always N matchers in flight, matchers reused to ~650k tokens, but every result integrated SERIALLY into main so the commit history stays a single linear line. Use to drive a sustained wave of function-matching (home stubs to their real TUs, then permute to 100%). The reconstruction doctrine (match-by-shape, STOP-EARLY, @early-stop, the permute skill at walls) lives in matcher.md.
---

# orchestrator — fan out the work, serialize the history

You drive the matching campaign with **parallelism in the work** and a **single
linear commit history**. The reconstruction doctrine — match-by-shape, STOP-EARLY,
`@early-stop`, and the **`permute` skill** at walls — lives in
`.claude/agents/matcher.md`. This doc covers the orchestration: **how to run many
matchers at once without tangling main**, and how to pick/skip targets.

**Target cross-check — dispatch only genuinely-unmatched work.** A function already
reconstructed in a real `src/` TU (even sitting at a fuzzy plateau) is done — skip it;
a true new target lives only in `src/Stub/`. The already-done RVA set is
`grep -rlE 'RVA\(0x' src --include=*.cpp | grep -v /Stub/ | xargs grep -ohE '0x[0-9a-f]{8}' | sort -u`
(addresses are 8-hex zero-padded EVERYWHERE, so this is a direct string subtraction).
Also skip anything already marked `@early-stop` (`rg '@early-stop' src`).

## The invariant

- **Fan out:** keep **N matchers in flight** at all times (default **4**, the
  provisioned pool). Each runs in **its own worktree** from a **fixed, reused,
  persistently-named pool** (`matcher-1 … matcher-N` under `.claude/worktrees/`) —
  never spawn a fresh worktree per task.
- **Serialize integration:** results land in **main one at a time**. Only ONE
  integration (apply → build → bless → commit) is in progress at any moment.
  → main's history is a **single linear line of `match:` commits**, even though
  the work was fanned out. No per-matcher branches merged into main.
- **Refill immediately:** the instant a matcher's result is integrated, reset its
  worktree to the new main HEAD and launch the next target into that same slot.
  The pool stays full until the worklist is dry.

```
       matcher-1 ─ matcher A ─┐
       matcher-2 ─ matcher B ─┼─► integration queue (SERIAL) ─► main: c1─c2─c3─…
       matcher-3 ─ matcher C ─┤        build → bless → commit
       matcher-4 ─ matcher D ─┘
   (as A lands, reset matcher-1 to main, launch matcher E into matcher-1)
```

## Pool setup (provision once — the worktrees PERSIST across restarts)

The pool is N long-lived worktrees named **`matcher-1 … matcher-N`** that are
**reused** across many matchers AND across orchestrator restarts. Each carries its
OWN gitignored `build/` (incl. its own wineprefix — `GRUNTZ_DIR=$PWD` ⇒
`WINEPREFIX=$PWD/build/wineprefix`), so its `gruntz build` is incremental, not a
cold `gruntz init`. Provisioning is a **full one-time `build/` copy** from main; a
copied wineprefix relocates fine (measured: all 4 worktrees build green in-place,
no regressions).

**Idempotent setup — skip any slot that already exists (this is what makes a
restart free; the four slots are already provisioned):**

```bash
for n in 1 2 3 4; do
  wt=.claude/worktrees/matcher-$n
  if [ -d "$wt" ]; then
    git -C "$wt" reset --hard main          # REUSE: build/ (+ wineprefix) survives
  else
    git worktree add -B matcher/$n "$wt" main
    cp -a build "$wt"/build                 # provision heavy gitignored state ONCE
  fi
done
```

Verify a slot can build before dispatching — **cd-first** so `GRUNTZ_DIR`/`REPO`
resolve to the worktree, NOT main:
`cd .claude/worktrees/matcher-1 && nix develop .#build --command gruntz build --fast`.
**`cd` AFTER `nix develop` builds *main*** (`GRUNTZ_DIR` is fixed at shell entry).
Better: open ONE `nix develop .#build` shell per slot and run `gruntz build --fast`/status
inside it — avoids `nix develop` startup per command. **Brief every agent to iterate with
`gruntz build --fast`** (full ninja + delink + objdiff %, skips the ~20 s gate tail) and run
ONE full `gruntz build` only before committing — the full gate/`clean` is the orchestrator's
integration step (§ below), NOT the agent inner loop.

(Filesystem is ext4 → no reflink; `cp -a` is a full ~680 MB copy per slot, seconds
each. If ever too heavy, copy only `build/{exe,gen,ghidra-named,ghidra-enrich,clangd,
delink,objdiff,pdb,wineprefix}` — but the whole-`build/` copy is simplest and pays
off over many reused dispatches.)

## Dispatching a matcher into a pool slot

Spawn a **matcher** agent (subagent_type `matcher`), **`run_in_background: true`**,
**NOT** `isolation: worktree` (you manage the worktree yourself). The prompt MUST:

1. Name the assigned **absolute** worktree path and say *do ALL work there*.
2. **Work cd-first, inside ONE open shell.** Tell the matcher: `cd <abs worktree>`
   FIRST, then enter a single `nix develop .#build` shell and run every `gruntz
   build`/status *inside it* — `GRUNTZ_DIR` is fixed at shell entry, so `cd`-after
   or a fresh `nix develop` per command builds/scores the **wrong tree** AND pays
   startup each time. Use absolute paths for every file/build command — relative
   paths can leak into the main repo (see `[[subagent-bash-cwd-leaks-to-main]]`).
   Never operate on the repo root.
3. Carry the standard matcher task (target RVA/name/size/file), the 8-digit
   address convention, and the STOP-EARLY + `@early-stop` rule (marker line + reason
   on the next line, **no percentage** — the baseline tracks %).
4. **Forbid `gruntz format` in the worktree.** It reflows trailing-comment
   alignment across *unrelated* clean files (measured: ~9 files swept in), which
   you then have to discard at integration. Tell the matcher: edit only its
   target file(s); leave formatting to the orchestrator.
5. **Allow migrations.** A stub is often mis-attributed (a placeholder
   `ErrorThunk_*`/`Stub_*` name routinely hides the real owner). If the matcher
   recovers the true owner, it SHOULD migrate the body into the real class's TU,
   remove the stub, and update the header — report every file touched.
6. Report: final per-function % + a one-line summary + the **complete
   `git diff`** of its worktree changes (so integration is a clean `git apply`).

Run the 3 dispatches in the background and let the harness notify you as each
finishes.

### Lane discipline (avoid same-file collisions)

Two matchers editing the **same file** collide at integration (duplicate
top-of-file class/extern decls → a build error, not a clean merge). So keep each
multi-stub file a **single lane**: route all of one file's targets (e.g. all of
`src/Stub/ApiCallers.cpp`'s) through ONE slot, another file's through another.
Cluster siblings in one file also share idioms — feeding the next sibling to the
same lane lets the matcher reference the just-landed one (a later sibling can even
*correct* a prior one's misdiagnosed `@early-stop`). The other slots take
distinct-file targets — prefer methods/loaders over ctors/dtors, which
systematically plateau ~60% on the `flags="base"` EH wall.

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
   same-RVA symbol renames). Three cases need `update --accept-regressions`:
   (a) a **migration** — the old stub symbol goes LOST at its RVA (intended
   rename); (b) a **trivial cross-function fuzzy drift** (measured: a neighbor in
   the same aggregate obj dropping <0.1% when you add a new function/string
   literal — its code+relocs are unchanged, pure objdiff scoring noise);
   (c) a **cleanup-phase correct-shape drop the worker REPORTED** with its
   mechanism (regalloc / header-fattening / reordering) — accepted per the
   clean-room mandate (docs/cleanup-plan.md). A `best%` drop
   with NO reported cause on an untouched function is NOT acceptable —
   investigate instead. Keep the bless in the same commit.
5. **Commit** atomically: `git add` ONLY this matcher's file(s) +
   `config/match_baseline.tsv`, message `match: <fn> -> <result>` with the
   `Co-Authored-By: Claude Opus 4.8 (1M context)` trailer. One matcher = one
   commit. (A clean `@early-stop` partial is a legitimate commit too; a
   mis-attributed / wrong-shape reconstruction is NOT — keep it stubbed,
   `[[correctness-not-artifacts]]`.)
6. **Refresh the slot:** `git -C .claude/worktrees/matcher-N fetch` is not needed
   (same repo); instead `git -C .claude/worktrees/matcher-N reset --hard main` then
   re-provision build state if needed (the worktree's own `build/` survives the
   reset — only tracked source is reset, so the next build stays incremental).
   Now the slot is at the latest main (it SEES the just-landed match as a possible
   dependency).
7. **Refill:** pick the next target (target cross-check at the top of this doc: skip
   anything already reconstructed or `@early-stop`). **REUSE the same matcher agent** for the
   next batch — SendMessage it the new worklist — **until it has spent ~650k tokens**
   cumulatively (sum the `subagent_tokens` across its completion reports); a warm
   matcher carries the idioms it just cracked. Only once it crosses ~650k do you
   **retire it and spawn a FRESH** matcher into the slot (a new agent starts with a
   clean budget). Either way the worktree is reused; only the agent identity rotates.

Repeat until the worklist is dry. **Leave the `matcher-N` worktrees in place** so
the next run reuses them (their `build/` stays warm); `git worktree remove` only if
the user asks.

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
