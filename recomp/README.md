# `recomp/` — run RETAIL's own code as the oracle

Byte-matching asks "do our bytes equal retail's?". This asks a different and stronger
question: **"does retail's own machine code, executed, agree with our reimplementation?"**

`harness/recomp.h` maps `GRUNTZ.EXE`, applies its `.reloc`, and hands out callable
addresses; each harness CALLs one retail function through it. The second half matters as
much: `build.sh` links OUR compiled object out of `build/objdiff/base/`, so the
comparison is retail's bytes against the bytes we actually ship, not against a
transcription of our source into the harness.

**Nothing here runs as part of the build.** It is opt-in and invoked by hand.

Five functions are covered. Four agree with retail on every input tried. The fifth,
`CGrunt::RectSegProbe`, **did not** — it carried two logic bugs that byte-matching at
78.77% never showed, and returned the wrong answer on 6.5% of random inputs. Both are
fixed. See `docs/harnesses.md`.

## Scope: FABRICATED inputs only — no game sessions

A harness stands its target's inputs up synthetically (a buffer, a palette, a few
scalars) and calls retail's copy and ours on the same bytes. Nothing launches
`GRUNTZ.EXE`, and nothing may.

There was a `replay/` half that captured state from the running game and replayed it at
its original addresses. **It has been removed** (2026-07-28, user ruling): it launched
real game windows repeatedly, which is disruptive, and the campaign's return per unit of
effort is far better in ordinary byte-matching. Do not rebuild it, and do not add a
harness that needs a live process.

What it proved is kept in `docs/verdicts.md` as a frozen record — seven functions
verified behaviourally correct (including one scoring 0.00% and four `@early-stop`-parked
bodies whose correctness was previously unknown), and the `CMinimap::Shape1..8`
family shown to drop RGB565 colour channels. That last one is a live lead and is
chaseable **statically**: the sibling `CDDrawShadeBlit` names the three `g_clut` planes
(`+0x20002` R, `+0x2` G, `+0x10002` B), and a missing `+0x10000`/`+0x20000` on a channel
read produces exactly the observed signature.

## Layout

    harness/recomp.h   the shared core: PE map + .reloc, __thiscall bridges,
                       a deterministic RNG, and the pass/disagree tally
    harness/build.sh   build.sh <name> [unit ...] - builds any harness and links
                       our compiled objects in
    harness/*.c(pp)    one harness per reachable function or family
    docs/harnesses.md  what each harness assumes, and why a target is reachable
    docs/verdicts.md   frozen: the verdicts the removed replay half produced

One harness per reachable function or family. Keep them small and independent — a harness
that needs the CRT stood up has stopped being a harness.

## What is reachable, and how to find out

    python -m gruntz.audit.recomp_islands   # can we FABRICATE the state?

Two conditions must BOTH hold, and they are separate questions:

1. **Self-contained code** — `ISLAND` (no relocs, no calls), `SELF-CALL`, or `DATA-ONLY`
   (relocs only to constant tables / static scratch, which you map beside the code).
   `IMPORTS` and live-global `RELOC` are out of reach.
2. **Cheap state** — measured as the count of distinct struct offsets the body actually
   dereferences. **The parameter types do not tell you this.** `CMapMgr::UpdateDiagonals`
   takes a `CGruntzMgr*`, which reads as "the whole engine", and touches exactly eleven
   fields; `CSaveGame::Encode`/`Decode` are members that touch zero. A `__thiscall` member
   whose body never dereferences `this` is as reachable as a free function — 311 of 693
   self-contained members turn out never to.

This is **not** limited to serialization. Serialization is merely the easiest case,
because its state is a byte buffer. The qualifying set also holds crypto
(`Blowfish_decipher`, 1 field), area queries (`CAreaMgr::SameGroup`, 1), list surgery
(`CMapMgr::Unlink`, 2), table lookups (`CTriggerMgr::ByteTableHas`, 2), geometry
(`RectSegProbe`, `PolyIsConvexCW`, 3) and colour matching (`FindNearestColor`, 3).

`python -m gruntz.audit.iat_tiers` remains as a static census of the binary — transitive
import reachability, which established that two thirds of the engine never touches the
IAT because the CRT and MFC are statically linked. That fact is independent of any oracle.

## The point is not the percentage

A function at 54% that is *behaviourally identical* to retail and one that is *computing
the wrong answer* look the same on the scoreboard. `FindNearestColor` is behaviourally
exact at 64.32%; `RectSegProbe` was broken at 78.77%. An oracle is the only thing that
tells those apart — but it is a supplement to byte-matching, not a substitute, and
byte-matching is what the campaign runs on.
