# The EH unwind map is a C1 fingerprint: dead TRACEs allocate states and caller cb

tags: cpp:trace cpp:temp cpp:inline | asm:funcinfo | topic:codegen-idiom topic:wall
symptoms: `gruntz.audit.eh_band` STATES reports ours N vs retail N+k with the
extra retail states carrying NULL actions and no `mov [state],k` stores
anywhere in the body; or a retail inline-ctor expansion (during-ctor states in
the caller's map) that our identical-looking caller declines to expand
confidence: 9/10

`_s_FuncInfo.maxState` counts C1's destructible-scope allocations, and C2
cannot subtract from it: a scope whose code is eliminated keeps its unwind-map
entry with a NULL action and no state stores. Release MFC defines
`TRACE` as `1 ? (void)0 : ::AfxTrace(...)` - the arguments are parsed, typed,
and STATE-ALLOCATED, then die in the false arm. Two consequences, both
measured here:

1. **A dead destructible temp leaves a map-only state.** A TRACE argument that
   builds a `CString` (a by-value `GetName()`, `CString(...) + "..."`)
   allocates one state per temp; two temps in one full expression NEST
   (`26 -> 25`). Retail functions carry these states with no code trace:
   the ONLY reachable evidence is the FuncInfo. Compare with the
   eh_states_diff dump, not the funclet list - shared funclets collapse there.
2. **A dead TRACE still adds caller cb.** The inline budget is
   `clamp(2*cb(caller), 1000, 35000)` (inline-budget-emits-ool-comdat.md), and
   cb is a front-end statement-mass estimate that counts the dead statement.
   `CGruntzMgr::TransitionState` declined the `CCreditsState` inline-ctor
   expansion that retail performed; one dead
   `TRACE("TransitionState %d\n", stateId)` at entry re-enabled it - states
   20-22 (base guard, ~CRgn@+0x1e8, ~CString@+0x1f0) materialized, branch
   counts landed 21/21, and the function jumped 86.42 -> 93.22 NEW MAX.

Reading the map (`scratchpad eh_states_diff.py`, or eh_band internals): each
`new T` with a throwing ctor = one delete-guard state (`??3@YAXPAX@Z`); an
inline-EXPANDED ctor chains its during-ctor states under that guard in member
declaration order, and the funclet body's `add ecx,<disp>` names the member
OFFSET - a direct layout oracle. `??_M@YGXPAXIHP6EX0@Z@Z` (vector dtor helper)
with a pushed dtor callback = an ARRAY member `T[n]`; the funclet pushes
(dtor, count, sizeof(T), &member).

Costing caveats (measured on `CKeyedList::AddNode`, 154 B, at the zero-reg
threshold): a TRACE temp spelled `static_cast<LPCTSTR>(x.GetName())` or any
named-temp form adds one lifetime-flag zero-init that can flip cl's
zero-register heuristic (`test esi,esi` -> `cmp esi,edi` + extra push). VC5
passes an LVALUE CString through varargs bitwise (no temp, no state, no cost),
which clang's `-Wnon-pod-varargs` refuses - in a function that small no
spelling satisfied both the state and the bytes; larger functions absorb the
flag (Run, ValidateLevelTiles: ~-0.08 for a retail-proven map). Dead string
literals do NOT leak into the string pool - safe.

Statically-unreachable code after `return` allocates NOTHING (C1 drops it
before state allocation) - it cannot substitute for the TRACE.
