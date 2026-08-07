# One inline body, two instruction streams: the callee it calls is inlined only at depth 1
tags: cpp:inline cpp:ctor | asm:call | topic:codegen-idiom topic:wall
symptoms: a function's standalone copy expands a helper in place, but the SAME function inlined into a caller emits `call <helper>`; a reconstruction that copies either shape into the shared header breaks the other
confidence: 9/10

cl 5 at `/O2` expands one level of inlining per callee. When `A()` is inlined into a caller,
`A`'s own calls to inline `B()` are NOT expanded again - they stay `call B`. But when `A` is
emitted **standalone** (an out-of-line copy in its owning TU), `B` IS expanded into it.

So a single inline source body legitimately produces **two different instruction streams**, and
the reconstruction needs both:

* the header carries the source as written - `if (!cache->Find(key)) cache->CreateWorker(...)`,
* the standalone `.cpp` copy carries the same body with `Find` **already expanded**
  (its `CObject* found = 0; m_workers.Lookup(key, found);` inlined in place).

Do not "reconcile" them. A comment claiming they must be byte-for-byte identical is the bug.

**Evidence.** `CUserLogic::BuildLogicTypeTable`. The standalone copy at `0x8a40` has all three
tests expanded (`mov ecx,[eax+0x14]; add ecx,0x10; call <CMapStringToOb::Lookup>` with the
2-arg out-param and a stack local per test). Inlined into `CWayPoint::CWayPoint` (`0xae3f0`),
`CGuardPoint::CGuardPoint` (`0xae5f0`) and `CLevelTime::CLevelTime` (`0x9b8b0`) all three tests
are `mov ecx,[edx+0x14]; call <CDDrawWorkerCache::Find>` - one arg, result in `eax`, no `+0x10`,
no stack local. Rewriting the shared header from the expanded shape to the `Find()` spelling took
those three ctors from **0 / 0 / 35.1% to 99.7%** each and `CLightFx::CLightFx` from 0 to 94.9%.

**The budget still bites at the boundary.** `CLightFx::CLightFx` (`0x9cf00`) shows retail expanding
`Find` at the FIRST of the three tests and calling it at the other two - a greedy per-function
expansion budget that runs out mid-body. That residue is not a source spelling; it is where the
cl 5 inliner stopped.

**How to tell which shape a given caller wants:** `gruntz sema xref <helper-rva>` lists exactly the
callers that kept the OUT-OF-LINE call. Everything in the family that is absent from that list
inlined the helper, and needs the inline header included in its TU. For
`?BuildLogicTypeTable@CUserLogic@@QAEXPAUCGameObject@@@Z` that is 60 of the 66
`??0C*@@QAE@PAUCGameObject@@@Z` ctors out of line, 6 inlined.

**"One level" is a BUDGET, not a depth cap - do not reach for `#pragma inline_depth(1)`.**
Measured 2026-08-07 on `wwdobjmgr`: our cl expands the depth-2 base ctor where retail calls it
(`CDDrawChildGroup::CreateContainerObject` inlines `CWwdGameObject`'s ctor AND `CGameObject`'s,
retail inlines the first and emits `call ??0CGameObject@@QAE@PAVCDDrawSurfaceMgr@@HH@Z`). Forcing
`#pragma inline_depth(1)` on that TU does move it - **42.52 -> 58.49** - but it simultaneously
craters two functions retail DOES expand two levels deep: `CDDrawChildGroup::SumWeighted`
99.93 -> 58.07 and `CDDrawChildGroup::DestroyChildren` 100.00 -> 65.15, plus
`CreateSpriteObject` 66.73 -> 57.50. So retail's inliner is not capped at one level; it is the
same greedy per-function budget as the `CLightFx::CLightFx` residue above, and a depth pragma is
the wrong knob (and a per-TU device besides - see the no-guard-devices ruling). The same
divergence sits under `_CreateDoNothingNormal` (`0xa9e00`, 39.6%), where retail calls
`??0CUserLogic@@QAE@PAUCGameObject@@@Z` at `0x58cd0` - one of its only three callers, the other
two being `CGrunt`'s and `CProjectile`'s ctors, both of which exhaust the budget on their own
bodies first.
