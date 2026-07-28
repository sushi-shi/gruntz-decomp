# `recomp/` — run RETAIL's own code as the oracle

Byte-matching asks "do our bytes equal retail's?". This asks a different and stronger
question: **"does retail's own machine code, executed, agree with our reimplementation?"**

`harness/recomp.h` maps `GRUNTZ.EXE`, applies its `.reloc`, and hands out callable
addresses; each harness CALLs one retail function through it. The second half matters as
much: `build.sh` links OUR compiled object out of `build/objdiff/base/`, so the
comparison is retail's bytes against the bytes we actually ship, not against a
transcription of our source into the harness.

Five functions are covered so far. Four agree with retail on every input tried. The
fifth, `CGrunt::RectSegProbe`, **did not** - it carried two logic bugs that byte-matching
at 78.77% never showed, and returned the wrong answer on 6.5% of random inputs. Both are
fixed. See `docs/harnesses.md`.

## Layout

    harness/recomp.h   the shared core: PE map + .reloc, __thiscall bridges,
                       a deterministic RNG, and the pass/disagree tally
    harness/build.sh   build.sh <name> [unit ...] - builds any harness and links
                       our compiled objects in
    harness/*.c(pp)    one harness per reachable function or family
    docs/              what each harness assumes, and why a target is reachable

One harness per reachable function or family. Keep them small and independent — a harness
that needs the CRT stood up has stopped being a harness.

## What is reachable, and how to find out

    python -m gruntz.audit.recomp_islands

Two conditions must BOTH hold, and they are separate questions:

1. **Self-contained code** — `ISLAND` (no relocs, no calls), `SELF-CALL`, or `DATA-ONLY`
   (relocs only to constant tables / static scratch, which you map beside the code).
   `IMPORTS` and live-global `RELOC` are out of reach.
2. **Cheap state** — measured as the count of distinct struct offsets the body actually
   dereferences. **The parameter types do not tell you this.** `CMapMgr::UpdateDiagonals`
   takes a `CGruntzMgr*`, which reads as "the whole engine", and touches exactly eleven
   fields; `CSaveGame::Encode`/`Decode` are members that touch zero.

Current census: 752 functions have self-contained code, 171 of those touch ≤12 fields,
and **65 of those are not yet exact** — the worklist.

This is **not** limited to serialization. Serialization is merely the easiest case,
because its state is a byte buffer. The qualifying set also holds crypto
(`Blowfish_decipher`, 1 field), area queries (`CAreaMgr::SameGroup`, 1), list surgery
(`CMapMgr::Unlink`, 2), table lookups (`CTriggerMgr::ByteTableHas`, 2), geometry
(`RectSegProbe`, `PolyIsConvexCW`, 3) and colour matching (`FindNearestColor`, 3).

Known under-count: a `__thiscall` member whose body never dereferences `this` is as
reachable as a free function — that is exactly why `RunDecode1` worked — but the audit
still charges it. So the worklist is a lower bound.

## The point is not the percentage

A function at 54% that is *behaviourally identical* to retail and one that is *computing
the wrong answer* look the same on the scoreboard. The oracle is the only thing that
tells them apart, and that is worth more than the score.
