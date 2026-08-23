# When our side is silent by construction, DELETE it: the one-sided screen

tags: cpp:member cpp:call msvc5:thiscall | asm:mov asm:call | topic:tooling
topic:identity topic:correctness
symptoms: a sieve that reads the base/target pair finds two hits and stalls; the
defect it hunts is invisible in the callee and its best witnesses score 100%
confidence: 9/10 (whole-image run with an ABI-proven control population,
2026-08-23)
variants: masked-diff-hides-branch-target.md

## The shape

`walls thisscan` hunts a dropped `this`: a `__thiscall` member modelled as a
free `__stdcall` with the same stack arguments compiles to IDENTICAL callee
bytes, because the receiver rides in ECX and costs the callee nothing. Only a
CALLER witnesses it, as an ECX definition retail emits and we do not.

Written as a paired screen it needs four filters, and one of them —
`ours-lacks`, "no ECX definition reaches our call site" — is not evidence at
all. **A function we model FREE has no receiver by construction.** Our side
cannot show one; the test can only ever cancel a row. It is a false-positive
filter wearing the costume of a symmetry check.

Deleting it removes three limits at once:

* no compare report is needed, so the screen runs before a build;
* the caller does not have to be PAIRED, so unreconstructed callers count;
* the caller does not have to be sub-100, so the whole image is in scope.

## Why the paired form is structurally blind

An EXACT caller is byte-identical to retail. It therefore CANNOT carry an
asymmetry, and no caller-diff sieve can flag it — which is a statement about
the rule, not about the code. The defect happily lives there: the callee scores
100 under either model, and any caller can be at 100 too.

Measured. `CBattlezMapConfig::TileSwitch` (0x029af0) scored **100.00 EXACT**
modelled as `void __stdcall TileSwitch(CGrunt*, i32, i32, i32, i32, i32)`. It
is a member — retail's sole caller, `CBattlezMapConfig::Step`, ends its
argument pushes with

    03174c: push eax
    03174d: push esi                 ; the CGrunt*, arg 1
    03174e: mov  ecx,edi             ; edi = the prologue's `mov edi,ecx`
    031750: mov  DWORD PTR [esp+0x30],eax
    031754: call ?TileSwitch@@YG...  ; ECX redefined right after, never read

and the callee's first instruction is `mov eax,[esp+0x10]`, so the receiver is
invisible in its bytes too.
The paired screen reported **no asymmetry at all** on that caller: `Step` is at
87%, far enough from retail that our side ALSO has an ECX definition inside the
loose window, so `ours-lacks` cancelled the row. The paired screen's recall
limit is exactly that — it needs the caller to be close enough that the only
ECX difference is the receiver.

## The rule, and the filter that IS the sieve

At every rel32 call site of the callee in retail (ILT `jmp` thunks expanded one
hop), require an ECX definition that

1. reaches the call without crossing a `call`, a `ret` or a branch,
2. is never READ between its definition and the call, and
3. NAMES an object — a member load, a global, a frame slot, a `lea` of a local,
   or a register copy.

Filter 2 is the whole sieve. Image-wide over 473 free-modelled functions:

    reachable by a direct retail call site            268
    retail call sites to them                        1225
    ... with an ECX definition in the window          275
    ... of those DEAD (nothing consumes it)            2      <- 273 removed
    ... and naming an object                           1      <- the hit
    callees flagged at EVERY site                      1
    ... at SOME sites only                             0

The last line is what makes this a RESULT rather than a threshold: no callee in
the population has even a partial receiver pattern. The 268 reachable
free-modelled functions contain exactly one dropped receiver, and it is fixed.
Keep the "some sites only" row in the output anyway - the strict rule stops at
a branch, so a receiver retail materialises before a guard reads as absent, and
a multi-site callee could miss the every-site bar for that reason alone.

The 273 are cl materialising a PUSHED ARGUMENT through ECX (`mov ecx,[ebx+8];
push ecx; call f`) where we happen to use EDX. Filter 3 removes the last false
positive: `?FileExists@@YAHPBD@Z` has `and ecx,0x3` reaching one call site
unconsumed — arithmetic that landed in ECX, not a receiver.

## The coverage gap closes itself

204 of the 473 have no direct `call` site at all, which looks like a blind
spot and is not: **167 of them are ADDRESS-TAKEN in retail** (a DIR32
reference or a `jmp` thunk stores their address) — object factories in a type
table, `*DlgProc` window procedures, `AcceptAlways` callbacks. A `__thiscall`
member cannot be stored as a plain function pointer, so being unreachable by
`call` is itself the evidence that they are free. The residual 37 have neither
a caller nor a reference — `?unexpected@@YAXXZ`, `RezDebugPrintf*`,
`SetDDrawReportModes` — and a function nothing reaches has no witness of any
kind, so the question is undecidable rather than unanswered. Say that; do not
count them as screened.

## Measure the noise floor against an ABI-PROVEN control, not a mirror

The instinct is to run the screen backwards and call equal volumes noise. For a
one-sided screen that is the wrong control, and the numbers say so loudly:
running it inverted (member in source, no receiver in retail) flags 556 of 2854
member-modelled functions. That is not a mirror — it is the rule's RECALL, and
it says absence of a detected receiver means nothing. Which is precisely why
the forward rule demands PRESENCE at every site and never argues from a missing
one.

The right control is a population the ABI proves cannot have the defect. 147 of
the 473 are library/CRT free functions — `_fopen`, `??2`, `AfxCallWndProc` —
which cannot be members under any model. They carry **537 call sites and zero
dead-ECX sites.** cl 5.0 does not write a register it will not read, so the
floor is not argued, it is measured at 0.

## The source-side screen is a candidate generator, not a sieve

The tempting score-free alternative is to read the SOURCE: for every class,
flag the free functions taking that class as a first parameter. Image-wide that
returns 115 rows, 29 of them with a retail call site, and exactly ONE with the
byte evidence. The other 114 are real free functions — object factories stored
as function pointers (`CreateActionArea(CGameObject*)` and forty siblings),
MFC's `ConstructElements`/`DestructElements`, serializers taking `CFileMemBase*`.

Worse, the screen cannot see the shape it was proposed for. Neither
`CSymParser::UnpackTag` nor `CGameLevel::InflateMainBlock` had a leading class
parameter: their receiver was ABSENT from the model, not demoted to an
argument. `void __stdcall UnpackTag(RezTypeTag, char*)` became
`void CSymParser::UnpackTag(RezTypeTag, char*)` — the stack arguments never
moved, which is exactly why the callee bytes did not either.

A leading class pointer is a hypothesis. The receiver-shaped ECX write at the
call site is the evidence.

## Reverse use

Before writing a paired sieve, ask what OUR side contributes. If the answer is
"nothing, by construction" — the model has no receiver, no argument, no store,
no call — then the paired form is strictly weaker than reading retail alone,
and its extra test is a recall tax. Write the one-sided screen, and calibrate
it on a population the ABI already decides.

    gruntz walls thisscan --retail          the screen
    gruntz walls thisscan --probe <rva>...  run it on named addresses
