# `recomp/replay/` — record the real game's state, replay it against our code

`recomp/harness/` can only reach a function whose input state we can FABRICATE, so
`recomp_islands` ranks candidates by how few struct fields a body dereferences. Anything
holding a real `CGruntzMgr`, a `CGrunt`, or the tile grid is out of reach.

Record-and-replay removes that constraint. Stop the real game at a chosen function's
entry, dump memory and registers; let it run to the matching return and dump again. Then
restore the entry snapshot **at its original addresses**, call our compiled version, and
diff every writable byte against the recorded exit.

Restoring at the original addresses is the whole trick: every recorded pointer is valid
by construction, so there is no closure to compute and no pointer fix-up. It also kills
the false differences that plague synthesized state — the allocator's own free lists are
in the snapshot, padding is identical because neither side writes it, and a field the
function does not touch keeps the snapshot value on both sides. So the comparison can be
a **full memory diff**, which is stronger than any hand-written output check because it
catches effects nobody thought to look for.

## Status

Working, on two functions, both already 100% byte-exact — which is the point: any
disagreement is then provably a harness bug, and three were found that way (below).

    _ParseWaveChunks                     0x00137110   capture + replay, verified
    DSoundList::Unlink                   0x001391e0   in targets.h, capture pending

The reference run:

    replay: _ParseWaveChunks
      snapshot   rva=00137110 image=00400000 hit=0 regions=328 32708404 bytes
      entry      esp=0024f680 ret=00536968 ecx=0024f6ac
      stack      00150000..00250000  EXCLUDED below 0024f680 (+4: the return slot)
      code       retail=00537110  ours=30001030
      noise      measured (entry.snap vs entry2.snap: the observer's own footprint is masked)
      effect     the recorded call changed 9 byte(s) in 1 region(s)
      [1] RETAIL re-run vs the RECORDED exit   IDENTICAL
      [2] OURS vs the RECORDED exit            IDENTICAL
      [3] OURS vs the RETAIL re-run            IDENTICAL
      VERDICT: agree on every compared byte

The nine bytes are the three out-parameters `ParseWaveChunks` writes into its caller's
stack frame. They are above the callee's entry ESP, so they are compared — which is the
reason the stack boundary is where it is.

## Using it

    recomp/replay/build.sh capture                       # -> SFMAN32.DLL (the recorder)
    recomp/replay/build.sh replay <unit> [unit ...]      # -> replay.exe  (the replayer)

    python recomp/replay/capture.py census               # the game's address space
    python recomp/replay/capture.py probe --seconds 90   # which candidates a session calls
    python recomp/replay/capture.py grab 0x00137110 --name _ParseWaveChunks

    wine recomp/replay/replay.exe <snapdir> --spawn      # the verdict
    wine recomp/replay/replay.exe <snapdir> --spawn --set-ours 0x0f71003c=0x61746164
                                                         # the NEGATIVE CONTROL

    python recomp/replay/fuzz.py build/replay \
        --field 0x0f71003c --pool-scan 0x0f710030:0x30 --oob 0xdeadbeef

`--spawn` is not optional in practice: see "claiming the addresses" below.

## Files

    provision.py   stands retail GRUNTZ.EXE up under wine. A thin driver over
                   scripts/archive/dynamic-trace/provision_trace.py - that tool was
                   retired for CLASS ATTRIBUTION, which says nothing about its asset
                   provisioning, so the archive.org fetch, the CD-check tree and the
                   software-GL wine env are reused rather than re-derived.
    capture.c      the recorder, an injected DLL. census / probe / capture modes.
    hookgen.py     the static `call rel32` site list for a target.
    capture.py     configure the DLL, run the game, collect.
    snapshot.h     the format (C). snapshot.py reads it from Python.
    replay.cpp     restore, call, full-diff. --map, --vehtest, --spawn, --set, --set-ours.
    targets.h      the replayable functions and where OUR version of each lives.
    fuzz.py        many mutated entry states, one child process each.
    build.sh       builds either half with the period toolchain under wine.

## Capture: an injected DLL, not Frida and not gdb

wine here is 11.8 **wow64**: a 32-bit PE runs inside a 64-bit host process. A ptrace/gdb
stub therefore sees a 64-bit process whose `/proc` maps carry Linux protections rather
than Win32 ones, and whose register file has to be read through the compat-mode view.
From inside, everything needed is one `VirtualQuery` away — and it is the *same* API the
replay restores through, so the region table means literally the same thing on both
sides. It is also the same toolchain as everything else here.

The door is the one the archived Frida tracer used: `SFManager_SelectBestDevice`
(0x0f8970) opens `SFMAN32.DLL` unconditionally at its head, and is called once from
`CGruntzMgr::Run+0x9c8`. Everything after that point — the whole game — is hookable;
anything strictly before it is not. One catch the gadget did not have: when
`GetProcAddress(h,"SFManager")` fails the game immediately `FreeLibrary`s us, which
would unmap the hook stubs the patched call sites point at, so DllMain pins the module
with a self `LoadLibraryA`.

Hooking is by **call-site patching**, not a prologue detour: rewrite the 4-byte
displacement of `call rel32` and the target's bytes stay pristine, with no
instruction-length decoding anywhere. The stub is entered with exactly the state the
callee would have seen, overwrites the return address at `[esp]` with its own trampoline,
and `jmp`s on. It runs on its own scratch stack, so it never writes below the callee's
entry ESP. All patches are removed before the snapshot, so the recorded image is retail's
bytes.

`mode=probe` exists because the expensive question is *which* candidate a play session
actually calls, and guessing costs a 90-second game launch per guess. It generates a
16-byte counting stub per candidate at run time and patches every call site at once: 145
candidates, 730 sites, one launch, and the game runs on undisturbed.

## The three rules

Everything the harness declines to compare is one of these. They are stated, bounded, and
none of them grows.

**1. The stack, below entry ESP.** Our compiled function's frame is not retail's —
different spills, a different frame size — so those bytes are dead scratch on both sides
by construction. The four bytes AT entry ESP are also skipped: that is the return-address
slot, which the replay overwrites with its trampoline and retail's `ret` only reads.
Bytes at entry ESP + 4 and above ARE compared: the incoming arguments and the caller's
frame, where out-parameters live. Dropping the whole stack would silently hide the most
common out-param idiom in this codebase — and would have hidden the entire measured
effect of the reference target.

**2. The writable data of other loaded modules.** wine's ntdll, kernel32, DDRAW, DSOUND,
and the game's proprietary MSS32/SMACKW32. Those pages are the runtime the harness is
itself executing on — writing the game's kernel32 `.data` over the replay process's
kernel32 `.data` killed it on the first attempt — and their addresses are assigned per
process by the loader, so the same address is not the same thing. For a CLEAN-tier target
(`gruntz.audit.iat_tiers`) the whole call closure never leaves the game image, so nothing
the function can read or write lives there. **This rule is the tier boundary made
executable**: a target that does reach those modules is IMPORT tier and needs recorded
import call sequences, not a wider restore. 84 of 334 regions, 708 KB against 41.8 MB of
game heap.

**3. The observer's own footprint — measured, not asserted.** The capture takes the entry
snapshot TWICE, back to back, with the program frozen and nothing in between. Every byte
that differs between them is what the act of observing costs. The replay masks exactly
those and prints the count. It is deliberately not a list of addresses: it is re-measured
in every capture, so it cannot quietly grow to cover a real difference, and if the
observer stops perturbing a byte that byte is compared again next time. On the reference
capture it is 4 bytes — two counters in a wine bookkeeping page at `00120400` that move
by 0x106 per snapshot.

## What the harness's own self-test caught

`[1] RETAIL re-run vs the RECORDED exit` runs retail's own bytes on the restored state.
The same code on the same state must reproduce the same answer; when it does not, the bug
is in the harness. It failed three times, and each was real:

* **11492 bytes across 4 heap regions**, for a 48-byte function that writes twelve. The
  game is multithreaded: the DirectSound mixer and wine's workers kept writing during the
  47 MB snapshot copy and between entry and exit. A snapshot of a running process is not
  an observable. Fixed by suspending every other thread for the whole window — entry
  snapshot, call, exit snapshot (`froze=6 other thread(s)` in the capture log).
* **4 bytes at `00120400`** that survived the freeze, because they are the observer's own
  footprint. Fixed by rule 3, which measures them instead of arguing about them.
* A **silent no-op**: `--set` parsing wrote its `=` terminator into `argv[]` in place, so
  `--spawn` forwarded a truncated argument and the child ran with no mutation at all —
  and cheerfully reported that everything agreed. Every fuzz result before that fix was
  vacuous.

Two more bugs, found by crashing rather than by the self-test, are worth naming because
they are not obvious: the naked call/return pair destroyed the C caller's callee-saved
registers (the compiler was keeping the snapshot pointer in `esi`), and `prepare()` has to
run **on the scratch stack**, because a save of our own stack taken before the switch
predates the frame we later return through.

`--set-ours` is the negative control: it perturbs the OURS side only, and a run with it
MUST report DISAGREE. It does, down to `eax ours=00000000 recorded=00000001`. A green
verdict from a comparison that cannot go red is worth nothing.

## Claiming the addresses (`--spawn`)

A fresh wine process has already mapped things. A read-only section view landed squarely
inside where `GRUNTZ.EXE`'s `.text` has to go, and a mapped section view can be neither
`VirtualProtect`ed to RWX nor moved. Saving and clobbering works for private memory; it
does not work for someone else's section.

So `--spawn` creates a child `CREATE_SUSPENDED` — only ntdll is mapped, its loader has
not run — reserves the recorded ranges in it with `VirtualAllocEx`, and resumes it. Every
later loader/CRT allocation then goes elsewhere. The ranges must be reserved as a
**merged union**: two recorded regions routinely share a 64 KB granule, and reserving the
second overlapping range fails, which silently left a hole in the middle of `.text` for
the child's loader to map an NLS section into.

That also delivers what fuzzing needs and wine cannot: **fork-per-case isolation and a
watchdog**. A mutated input that faults comes back as a fault code; one that does not
terminate is killed on a timeout. Neither stops the run.

## Fuzzing, and the two kinds of result

The recorded exit is the answer to ONE input — the one the recorded run happened to take.
`--set` mutates the restored entry state and re-runs BOTH sides from their own pristine
restore, because a mutated input has no recorded answer; only `[3] OURS vs RETAIL` is a
verdict then, and the harness suppresses `[1]` and `[2]` rather than let them look
meaningful.

A mutation can produce state the program could never reach, and a disagreement there may
be two implementations differing on an impossible input rather than a bug. So `fuzz.py`
keeps two buckets and never blurs them: **in-distribution** values come from
`--pool-scan BASE:LEN`, the distinct 4-byte values the RECORDED buffer actually contains
at that alignment; **out-of-distribution** values are supplied explicitly with `--oob`.

On `_ParseWaveChunks` the recorded corpus takes one arm of six static branches
(`gruntz sema disasm 0x00137110 --branches`). Three mutations reach three others, and the
effect measurement shows it — 9 bytes/1 region un-mutated, 10/2 when the first chunk tag
is any non-`fmt ` value, 4/1 (nothing written at all) when the `RIFF` magic is broken.
Ours and retail agree on every one.

## What limits this, honestly

**Reachability is no longer "how big is the state". It is "does the function touch
anything outside the process?"** `gruntz.audit.iat_tiers` answers that by walking the
call graph: CLEAN 1348 (31.6%), UNKNOWN 1505 (35.3%), IMPORT 1410 (33.1%). Two thirds
never reach the IAT because the CRT and MFC are statically linked in this binary.

Measured over the CLEAN tier, against the two constraints this harness actually has —
a direct `call rel32` site to patch, and code our linked-in object can run without
binding anything. (The totals drift by one or two as the tree moves; these are from the
build this commit was made on.)

| set | total | self-contained (leaf, no relocs) | hookable (direct call site) | both |
|---|---|---|---|---|
| all CLEAN | 1347 | 885 | 768 | **423** |
| CLEAN, not yet exact | 224 | 96 | 172 | **64** |
| CLEAN, not exact, ≥ 0x40 B | 177 | 68 | 136 | **44** |

So **44 not-yet-exact functions of 0x40 bytes or more work today with no further
tooling** (64 counting the small ones), against 65 for the synthesis-based ranking — but
a different 65, and one that includes bodies whose state could never have been
fabricated. The ceiling in the CLEAN tier alone is 177; the gap is two pieces of work
named below.

**The limiter that is not about the binary at all: does a play session call the target?**
The probe measured 33 of 145 candidates called in a good 90-second menu session and 9 in
a degraded one, and `DSoundList::Unlink` fired in one session and never again once the
audio device stopped initialising. That is a property of the session, not of the harness,
and `mode=probe` measures it in one launch instead of one launch per guess. Reaching
in-game code needs a session that gets into a level.

**What genuinely cannot be replay-tested: anything whose effect leaves the process.**
DirectDraw surfaces (video memory, driver-owned), DirectSound buffers, `HFILE`, `HKEY`,
sockets. A memory diff cannot see them, and the snapshot boundary does not contain them.
The answer is not a wider snapshot but a *recorded import call sequence* — name,
arguments, return, bytes written — replayed to the function and then **compared as part
of the observable**. That is strictly stronger than a memory diff for I/O code: it catches
"we called DirectDraw with the wrong rect", which no amount of memory comparison can.

**Determinism hooks are all imports, hence all hookable**: `rand`, `timeGetTime`,
`GetTickCount`, `QueryPerformanceCounter`. Replaying the RECORDED sequence is stronger
than reseeding, because it reproduces the exact values the recorded run saw rather than a
different-but-deterministic set. None of the CLEAN tier needs them.

**Single-step coverage does not work in the replayed context, and I could not fix it.**
`replay.exe --vehtest` proves this wine delivers BOTH `INT3` and trap-flag single-step
exceptions to a 32-bit vectored handler (it also proves wine reports `Eip` AT the `int3`,
where Windows reports it one past). But arming either inside the restored state kills the
child with no exception report — the last stage marker written is `call`. TF set through
the trampoline's `popfd`, and a breakpoint written over the restored target's first byte,
both fail the same way; wide TEB stack bounds (needed anyway: "Exception frame is not in
stack limits" is otherwise fatal) did not change it. `--trace` is left in place and does
not run by default. Until it works, coverage is the STATIC branch inventory from
`gruntz.core.branches` next to the observed change in effect size — which distinguishes
arms, but does not name them.

## What is next, in order

1. **Bind the linked-in object's relocations to the restored image.** Today our object is
   linked into `replay.exe`, so a global it references resolves to OUR `.data` and an
   intra-image call goes to a `/FORCE:UNRESOLVED` null. That is the only reason the
   "self-contained" column exists. Loading the `.obj` at run time and binding each
   relocation to `game_base + rva` from `build/gen/symbol_names.csv` lifts CLEAN,
   not-exact, ≥0x40 from 44 to 136 — every function with a call site.
2. **A prologue detour for targets with no direct call site**, reached only through
   virtual dispatch: 41 of the remaining CLEAN not-exact functions of 0x40+ bytes. Needs instruction
   lengths for the first ≥5 bytes, which `gruntz sema disasm` already knows.
3. **Import stubs**, for the IMPORT tier — record and replay the call sequence, and make
   the sequence part of the compared observable.
4. **A session that reaches gameplay**, so the probe's candidate set stops being menu code.
