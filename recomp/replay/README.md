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

## Using it

    recomp/replay/build.sh capture                # -> SFMAN32.DLL (the recorder)
    recomp/replay/build.sh replay                 # -> replay.exe   (the replayer)

    python recomp/replay/reach.py                 # what is reachable, and what blocks the rest
    python recomp/replay/reach.py --probe-cfg     # arm the whole runnable set
    python recomp/replay/capture.py --seconds 100 probe    # which of them a session calls

    python recomp/replay/verdict.py '?Expand@CMapMgr@@QAEHPAUBrickzNode@@HHHH@Z' --control

`verdict.py` is the whole loop in one command: bake the unit's object with its
relocations bound to the restored image, hook the target's call sites, run the game,
snapshot entry/entry-again/exit around one call, restore, run RETAIL, run OURS, and diff
every writable byte. `--control` then re-runs the same snapshot with a one-sided
perturbation and **requires** the verdict to flip to DISAGREE.

A capture is kept under `build/replay/snap/<name>_<rva>/`, so everything after it —
re-running, mutating with `--set`, fuzzing, cross-running another function on the same
state — costs seconds rather than another game launch.

## Where OUR code comes from: a run-time-bound object, not a link

The first version LINKED `build/objdiff/base/<unit>.obj` into `replay.exe`. That resolves
every global the function touches to replay.exe's OWN copy of that variable instead of the
game's, and every intra-image call to a `/FORCE:UNRESOLVED` null — so only bodies with
**zero relocations** could be run at all. That restriction, not the tier and not the size
of the state, was the ceiling: 44 of the 173 CLEAN not-yet-exact functions of 0x40 bytes
or more.

`objbind.py` removes it. It lays the object's sections out at a fixed base and binds each
relocation to `game_base + rva`, looking every symbol up by its MANGLED name in
`build/gen/symbol_names.csv` (ours, authoritative) and then `config/library_labels.csv`
(FLIRT, HIGH confidence and unambiguous only — `_memcpy` matches two bodies, so it names
neither). `replay.exe` then VirtualAllocs the pre-relocated blob and memcpy's it; no
loader logic runs in the replay process, which matters because its CRT is unusable for
most of the sequence.

**Retail wins, always.** A call from our function to another of OUR functions binds to
retail's copy of the callee, so a second unmatched body cannot contaminate the verdict on
the first. The only symbols that fall through to our own copy are the ones with no retail
identity: intra-function jump-table labels (benign) and `$SG` string literals (a caveat).
Both are counted per function and printed. A symbol that resolves to neither is
UNRESOLVED and the function that contains it is **REFUSED**, not run — a function that
calls through an address nobody chose does not fail honestly, it corrupts the comparison.

    $ python recomp/replay/objbind.py lightfxrender --list
      ok  000a3dc0 -> ours 51000860  reloc 67 (retail 67, intra 0, foreign 0, unresolved 0)  ?Shape1@CLightFxRender@@QAEHXZ
      ok  000a4890 -> ours 510010e0  reloc 49 (retail 49, intra 0, foreign 0, unresolved 0)  ?Shape2@CLightFxRender@@QAEHXZ
      ...

Because targets are now resolved by name out of the loaded module, `replay.exe` links
nothing and adding a target needs no rebuild of anything. `targets.h` is gone.

Three checks run before a single instruction of ours executes, and each one closes a
silent failure: the module's image base must equal the snapshot's (otherwise every global
it reads is off by a constant); the symbol's recorded retail address must be the function
the capture hooked (otherwise this is the wrong `--fn` for this snapshot); and the
symbol's section must carry no unresolved relocation.

## The rules — what the harness declines to compare

Four, all stated, all bounded, none of them growing to cover a difference.

**1. The stack, below entry ESP — and the incoming argument area above it.** Our compiled
function's frame is not retail's, so those bytes are dead scratch on both sides by
construction. The four bytes AT entry ESP are the return slot the replay redirects.

The argument area was added the first time a function with stack arguments was replayed,
and it was a genuine false DISAGREE: retail's `ComputeCellFlags` leaves its 8-neighbour
loop counters in its own incoming `x` and `y` slots and our build leaves different ones
there. Neither is observable — under the MSVC x86 ABI the argument area is dead the
instant the callee returns, since a `__thiscall`/`__stdcall` callee POPS it and a
`__cdecl` caller's `add esp,N` abandons it. Parameter-slot reuse as scratch is ordinary
MSVC codegen, and comparing it compares register allocation.

Its width is **measured, not assumed**: the callee's own `ret N`, read as
(exit esp − entry esp − 4) out of the recorded exit snapshot, printed on every run. A
`__cdecl` callee pops nothing, so this comes out 0 and the argument slots ARE compared —
the conservative direction. An out-parameter is unaffected: the SLOT holds a pointer, and
the memory it points at is somewhere else and still compared in full.

Everything else above the boundary is compared, which is the point: the caller's frame is
where out-parameters live, and dropping the whole stack would have hidden the entire
measured effect of the reference target.

**2. The writable data of other loaded modules.** wine's ntdll, kernel32, DDRAW, DSOUND,
and the game's proprietary MSS32/SMACKW32. Those pages are the runtime the harness is
itself executing on — writing the game's kernel32 `.data` over the replay process's
kernel32 `.data` killed it on the first attempt — and their addresses are assigned per
process by the loader, so the same address is not the same thing. For a CLEAN-tier target
(`gruntz.audit.iat_tiers`) the whole call closure never leaves the game image, so nothing
the function can read or write lives there. **This rule is the tier boundary made
executable**: a target that does reach those modules is IMPORT tier and needs recorded
import call sequences, not a wider restore.

**3. The observer's own footprint — measured, not asserted.** The capture takes the entry
snapshot TWICE, back to back, with the program frozen and nothing in between. Every byte
that differs between them is what the act of observing costs. The replay masks exactly
those and prints the count. It is deliberately not a list of addresses: it is re-measured
in every capture, so it cannot quietly grow to cover a real difference, and if the
observer stops perturbing a byte that byte is compared again next time.

**4. `eax`, and only when the mangled name proves a void return.** For a void function eax
is whatever the last instruction left there, so comparing it compares codegen. The
decision comes from the NAME (`objbind.void_return`), never from a guess, and it
**under-claims**: a constructor mangles with no return type at all and really does hand
`this` back in eax, so it is not treated as void. Checked against the independent `ret`
field of `build/gen/functions.json` over the whole binary: **0 false positives**, and all
442 under-claims are exactly the ctors and dtors.

## The three comparisons, and what each one proves

    [1] RETAIL re-run vs the RECORDED exit   a SELF-TEST OF THE HARNESS. The same code on
                                             the same state must reproduce the same
                                             answer; if this fails, nothing else counts.
    [2] OURS vs the RECORDED exit            the oracle verdict, on the input the game
                                             actually produced. Ground truth.
    [3] OURS vs the RETAIL re-run            the only verdict available once the state has
                                             been mutated or the function swapped.

Every run also prints the **measured effect** — how many bytes the recorded call changed —
because "IDENTICAL" means nothing until you know the comparison had something to compare.
A void function with a zero-byte effect is a test that cannot fail, and the harness says
so rather than reporting a pass. A non-void function with a zero-byte effect is a
different thing: its observable is the return value, which IS compared, plus the negative
claim that neither side wrote anything else in ~130 MB of restored state.

## `--cross`: one capture, eight verdicts

`CLightFxRender::Shape1..Shape8` are eight `QAEHXZ` methods that `BuildShape` dispatches
between by index. A play session picks one shape and calls only that one, so seven of the
eight are uncapturable however long the session runs — not because the harness cannot
reach them, but because the level did not ask for them.

Yet Shape1's entry state IS the state Shape7 would have seen: same object, same surface,
same call site, no arguments, and the shape index is not an input to `ShapeN` at all — it
selected which `ShapeN` was called. So `--cross` lifts the "this symbol must be the one
the snapshot captured" check, under exactly the terms `--set` already had: the recorded
exit answers a different question, so [1] and [2] are suppressed and only [3] is a
verdict.

    python recomp/replay/verdict.py '?Shape7@CLightFxRender@@QAEHXZ' \
        --cross '?Shape1@CLightFxRender@@QAEHXZ'

It costs the [1] self-test, which is why the captured function should be run on the same
snapshot first: that run validates the state and the machinery, and the cross-runs ride on
it. The one thing `--cross` does NOT lift is ABI compatibility — both names must agree on
everything after the function's own identifier (class, access, calling convention, return
type, argument list), because the recorded registers and the recorded stack ARE the
arguments.

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
actually calls, and guessing costs a 100-second game launch per guess. It generates a
16-byte counting stub per candidate at run time and patches every call site at once: 129
candidates, 436 sites, one launch, and the game runs on undisturbed.

An **in-game** snapshot is 114–130 MB where a menu one is 33 MB, so the capture DLL's
buffers and the replay's arena are both sized for it: the DLL reports the exact
requirement rather than "raise cap", and the arena is sized from the input files and
lives in the window between the recorded regions and wine's DLLs. Only the FIRST side run
is copied out — the second side's result is compared in place, which is what makes a
130 MB snapshot fit at all.

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

And one that was found by a false DISAGREE rather than a crash: comparing the incoming
argument area, which MSVC reuses as scratch. See rule 1.

`--set-ours` is the negative control: it perturbs the OURS side only, and a run with it
MUST report DISAGREE. `verdict.py --control` runs it automatically against eight dwords of
the caller's frame and **checks the result**, reporting an AGREE there as a HARNESS
FAILURE — because it means that verdict's comparison could not have gone red. Everything
the config reader can silently truncate now says so too: the probe list outgrew an 8 KB
buffer and a 512-byte line by margins of 1 KB and 110 characters, and neither would have
mentioned it.

## Claiming the addresses (`--spawn`)

A fresh wine process has already mapped things. A read-only section view landed squarely
inside where `GRUNTZ.EXE`'s `.text` has to go, and a mapped section view can be neither
`VirtualProtect`ed to RWX nor moved. Saving and clobbering works for private memory; it
does not work for someone else's section.

So `--spawn` creates a child `CREATE_SUSPENDED` — only ntdll is mapped, its loader has
not run — reserves the recorded ranges in it with `VirtualAllocEx` (plus the object
module's range), and resumes it. Every later loader/CRT allocation then goes elsewhere.
The ranges must be reserved as a **merged union**: two recorded regions routinely share a
64 KB granule, and reserving the second overlapping range fails, which silently left a
hole in the middle of `.text` for the child's loader to map an NLS section into.

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
`--arg K` names the K-th incoming stack argument, which is how a pure query function whose
only observable is its return value gets swept over its own argument space with no game
launch.

## What is reachable — measured

Three independent conditions, each answered by a different tool, and `reach.py` reports
them separately because each failure has a different remedy:

* **TIER** — `gruntz.audit.iat_tiers` says CLEAN: the transitive call closure never
  reaches the IAT, so nothing the body does leaves the process. CLEAN 1345 (31.6%),
  UNKNOWN 1501 (35.2%), IMPORT 1417 (33.2%). Two thirds never reach the IAT because the
  CRT and MFC are statically linked in this binary.
* **HOOKABLE** — `hookgen` finds a direct `call rel32` site to patch.
* **BOUND** — `objbind` resolves every relocation in the function's COMDAT.

| set | total | hookable | binds cleanly | BOTH (runnable) |
|---|---|---|---|---|
| all CLEAN | 1345 | 766 | 1313 | **738** |
| CLEAN, not yet exact | 218 | 166 | 215 | **163** |
| CLEAN, not exact, ≥ 0x40 B | 172 | 131 | 169 | **128** |

The verdicts produced so far are in `../docs/verdicts.md`: seven functions AGREE with
retail on the state the real game produced (five of them `@early-stop`-parked bodies whose
correctness was previously unknown, one of them scoring 0.00%), and the whole
`CLightFxRender::Shape1..8` family DISAGREES.

So **128 not-yet-exact functions of 0x40 bytes or more work today**, against 44 before the
relocations were bound. What blocks the remaining 44 of that set:

    41  no direct call site (reached only through virtual dispatch - needs a prologue detour)
     2  unresolved: _inflate_mask
     1  unresolved: _MoveSubDispatch12@16

`?Activate@CGrunt@@UAEXXZ` is one of the 41: it has an ILT thunk and **zero** direct call
sites, so there is nothing to patch.

## What limits this, honestly

**The limiter that is not about the binary at all: does a play session call the target?**
Of the 129 armed candidates, **49 fired** in one 100-second session and **53** in a
200-second one — but not the same 53. The two probe sessions agree on the level-load and
per-frame code and disagree completely on the rest: one called
`CDDrawShadeBlit::ConvertRow` 6057 times and the other never; one called
`CMapMgr::Expand` 1080 times and the other 14144. Nothing drives the game's input, so how
far a session gets, and which content it renders, is not under the harness's control.
`verdict.py` reports `NOT REACHED` and retrying is worth roughly one coin flip.
`mode=probe` at least measures the whole candidate set in one launch instead of one launch
per guess, and `--cross` converts one lucky capture into a whole family of verdicts —
eight `CLightFxRender::Shape` verdicts came out of a session that called exactly one of
them, exactly once.

**Two failures that looked like session variance and were not.** Worth naming because both
cost real time before being found, and both are now impossible to have silently:

* A **stale wineserver** — left behind whenever a session is SIGKILLed, which a watchdog
  or an interrupted batch does routinely — makes the next `wine GRUNTZ.EXE` exit
  IMMEDIATELY: no window, no DllMain, no log line, and a `grab` that reports "the session
  never called it". Six consecutive targets were written off as unreachable that way.
  `capture.py` now runs `wineserver -k` on the play prefix before every session, and
  `provision`'s `wineserver --wait` does not substitute for it: it waits for an exit that
  is not coming.
* **`capture.py probe` reported a THREE-HOUR-OLD `hits.txt`** as that run's measurement,
  because it never deleted the previous answer — so a session that died before DllMain
  ran still printed a confident, plausible, complete candidate census. It now deletes
  `hits.txt` first and fails loudly with the wineserver hint if the run does not recreate
  it. (`grab` always deleted its snapshots; `probe` was the one that did not.)

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
    objbind.py     lay a compiled .obj out and bind its relocations to the game image.
    objmod.h       the baked module's format, shared by objbind.py and replay.cpp.
    replay.cpp     restore, call, full-diff. --obj/--fn, --cross, --set, --set-ours,
                   --spawn, --map, --vehtest.
    reach.py       what is reachable, split by which condition fails.
    verdict.py     the whole loop in one command, with the negative control.
    fuzz.py        many mutated entry states, one child process each.
    build.sh       builds either half with the period toolchain under wine.

## What is next, in order

1. **A prologue detour for targets with no direct call site**, reached only through
   virtual dispatch: 41 of the remaining CLEAN not-exact functions of 0x40+ bytes,
   `CGrunt::Activate` among them. Needs instruction lengths for the first ≥5 bytes, which
   `gruntz sema disasm` already knows.
2. **A session that reaches gameplay reliably**, so the probe's candidate set stops being
   a coin flip. Nothing drives the game's input today.
3. **Import stubs**, for the IMPORT tier — record and replay the call sequence, and make
   the sequence part of the compared observable.
