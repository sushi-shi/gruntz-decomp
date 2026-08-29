# A callee's own `ret N` states its convention and stack-argument bytes, with no pairing

tags: cpp:class cpp:ctor msvc5:mangling | asm:ret | topic:tooling topic:correctness topic:negative-control
symptoms: a signature you cannot check because the function has no compare row, or has one at
  100.00 that a wrong declaration would still reach; `ret 0x10` against a declaration you read
  as three arguments; a whole class of "did we drop or add a parameter" questions
confidence: 10/10

`ret` means the CALLER cleans up (`__cdecl`); `ret N` means the callee pops N bytes of stack
arguments (`__stdcall` / `__thiscall`). Our declaration states the same two facts in its
mangled name. The comparison needs only the retail image - no compare report, no pairing, no
score - so it reaches every function, including the ones already at 100.00.

    gruntz walls retscan [--all] [--blind]     # the callee's own `ret`
    gruntz walls retscan --cdecl [--all]       # retail's caller cleanup

This is the STACK complement of `walls thisscan`, which owns ECX. A dropped RECEIVER is
invisible here by construction (`this` rides in ECX and is not part of `ret N`, which is
exactly how `CBattlezMapConfig::RerouteIdleUnit` stayed wrong while exact); a dropped or added
stack ARGUMENT is invisible there. Run both.

## The five ABI rules (each was discovered by producing a uniform false hit)

1. `this` is NOT in `ret N` for `__thiscall`.
2. `this` IS in `ret N` for a non-static member declared `__stdcall` - every COM `STDMETHOD`.
   `CArchiveStream`'s seven `IStream` methods read as a uniform +4 without this.
3. A constructor of a class with a VIRTUAL BASE carries cl's hidden most-derived flag, which
   the mangling does not record. Eleven iostream constructors (`ifstream`, `ofstream`,
   `istream`, `strstream`, all virtually derived from `ios`) read as a uniform +4, so a
   constructor's expectation is WIDENED to {N, N+4} rather than decided.
4. A by-value UDT return may or may not travel through a caller-supplied hidden pointer - cl
   returns a small POD in EAX:EDX - so that widens too.
5. MSVC mangles a NAMESPACE scope exactly like a class scope, so `::` in a demangled spelling
   does not mean "member". `NetLobby::HostWaitDlgProc` and its five siblings are free `@@YG`
   dialog procs and read as a uniform -4 under a `::` test. The ACCESS SPECIFIER says member.

## Reading the `ret` out of an extent

An extent can decode more than one `ret` immediate, for two reasons that defeat opposite
position rules: a `/GX` funclet cl emits INSIDE the parent's extent contributes a real bare
`ret` BEFORE the function's own (`__ArrayUnwind` at 0x11f6f0: funclet `ret` at +0x4c, its own
`ret 0x10` at +0x67), while a linear sweep through an embedded switch table invents `ret`s
AFTER it (`CButeMgr::SetString` decodes `ret 0xc`, then `ret 0x5733` twice). So the test is
MEMBERSHIP - a spurious decode can add a value but never remove the true one.

A body whose only exit is an INDIRECT tail jump (`jmp DWORD PTR [eax+0x40]`, a virtual
dispatch) states no immediate at all and is the one honest blind spot. A DIRECT tail `jmp` is
not: a tail jump is only legal between functions with the same stack-argument bytes, so the
target's `ret` is the caller's.

## The other half: `ret 0` says nothing about a `__cdecl` arity

A `__cdecl` callee pops nothing, so a trailing argument it never reads is invisible in its own
bytes exactly as a receiver is. `--cdecl` reads RETAIL's caller cleanup instead - still
one-sided, so it needs no reconstructed, paired or scoring caller, which is what separates it
from `walls thisscan --arity`. Four reader rules, each forced by a false hit:

* ONE `add esp,N` is the whole cleanup; a SECOND is the caller releasing its own storage
  (`call dprintfdoprint / add esp,0x4 / add esp,0x100` reads as 0x104 if summed).
* cl 5.0 spells a two-argument cleanup as two `pop ecx` and is free to put an instruction
  BETWEEN them (`pop ecx / test eax,eax / pop ecx`), so the run accumulates across non-esp
  instructions. `DestructElements`, `ConstructElements`, `FindPopupMenuFromID`,
  `AfxDynamicDownCast` and `AfxTimeToFileTime` all read as popping 4 of 8 when it was cut.
* `pop esi` / `pop edi` / `pop ebp` is the EPILOGUE, never an argument - walking past them
  reaches the caller's frame release (`BuildColorChannelTables()` takes nothing and its
  caller's `add esp,0x6c` sits three instructions later).
* a zero-argument callee has nothing to check; skip it rather than screen it.

The cleanup may sit one or two instructions after the call
(`call MakeButeSectionKey / mov eax,[esi+0x5a4] / add esp,0xc`), so the window is four. 130 of
our `__cdecl` declarations and 36 library labels decidable, ZERO disagreements.

## The control, and the result

The exact rows. A function scoring 100.00 is byte-identical to retail, so ITS `ret` is
retail's and a flagged row can only be a parser bug. That cell caught all four defects of the
first run (two parameter-list parses - a template argument that is itself a function type puts
parentheses in the CLASS name, and a function RETURNING a function pointer puts its own
parameters in the middle of the spelling - plus rule 5 twice).

2026-08-23 whole-image census, both populations CLEAN:

|                                 | `src` (our own) | config/retail labels |
|---------------------------------|-----------------|----------------------|
| decidable (a singleton `ret`)   | 3905            | 1764                 |
| UDT-bounded (inequality)        | 57              | 27                   |
| agree                           | 3962            | 1791                 |
| **DISAGREE**                    | **0**           | **0**                |
| blind: no `ret` reachable       | 15              | 20                   |

A by-value UDT parameter's size is not in the mangling, but it occupies at least one 4-byte
slot, so those rows keep the INEQUALITY test rather than being dropped: `ret N` below
(known bytes + 4 per UDT) would be a defect whatever the class sizes are.

The result retires the hypothesis class: no reconstructed declaration in the tree disagrees
with retail's own `ret` about its convention or its stack-argument count.

## Harness note

`llvm-undname` DROPS a final line with no trailing newline (which silently cost
`?EnsureSize@zBitVec@@QAEHH@Z`, the last name of the corpus, its row) and separates records
with a blank line - split on that rather than pairing by "a line starting with `?` is a name".
