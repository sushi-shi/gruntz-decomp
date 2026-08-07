# A tiny helper sitting immediately AFTER each of its callers is that class's CONSTRUCTOR
tags: cpp:ctor cpp:inline cpp:class | asm:call asm:new | topic:codegen-idiom topic:identity
symptoms: a family of near-identical 0x31-byte functions, one per `Set<Type>` / `Add<Type>` caller, each starting 8 bytes past the end of that caller; the call is guarded by `test eax,eax; je` around a preceding `operator new`; the callee's return in `eax` is immediately `push`ed
confidence: 9/10

An inline constructor emits an out-of-line COMDAT copy the first time some call
site does not expand it. cl5 lays that COMDAT down **immediately after the
function that forced it**, so an address-ordered listing shows a regular
`caller, helper, caller, helper` rhythm. That rhythm is the giveaway: the helper
is not a `Set*`/`Init*` API, it is `??0Class@@QAE@...`.

Three independent confirmations, all cheap:

1. **The null guard.** `push <size>; call ??2; add esp,4; mov [esp+N],eax;
   test eax,eax; mov [esp+state],K; je skip; <push args>; mov ecx,eax; call H`
   is the *whole* codegen of `new T(args)` - the ctor is skipped when the
   allocation fails and the EH state guards the raw cell. `p = new T; p->Set(...)`
   would need an explicit null test in source and would not set an EH state.
2. **`push eax` after the call.** A ctor returns `this` in `eax`, and cl5 reuses
   it as `&local` at a stack construction site.
3. **The trylevel moves AFTER the call**, which is when a constructed object
   becomes unwind-live.

`CButeMgr::SetValue` 0x173dd0 clinched it for the whole Bute family: its helper
slot resolved to `??0CButeValue@@QAE@W4ButeType@@PAU0@@Z`, already correctly
pinned, while its eight siblings had the same slot modelled as phantom
`CButeValue::SetInt/SetDword/...` methods (and the Int/Dword/Float trio was
rotated by one slot on top of that). Replacing all eight definitions with
`RVA_COMPGEN` pins on the real ctors is byte-neutral for the helpers and removes
eight functions that never existed.

## The spelling of the ctor body matters

`pValue = new i32(v)` and the expanded `p = new i32; if (p) { *p = v; pValue = p; }
else pValue = NULL;` produce the same 49 bytes **except on the null arm**: the
new-expression leaves its result register live, so retail ends
`xor eax,eax; mov [esi+4],eax` where the expanded form emits
`mov DWORD PTR [esi+4],0`. Two bytes - but the compact spelling also has smaller
IL, and switching to it moved the nine `CButeMgr::Set*` callers by 5-19 points
each because it changes what the /Ob1 inline budget can afford.

## What does NOT close

cl5's per-caller inline budget still differs: retail expands the ctor at the first
three sites of `CButeMgr::SetInt` and calls it at the remaining four; ours expands
five. The residue is the extra callee-saved register cl allocates for a hoisted
zero constant, which shifts every `esp`-relative displacement by 4. See
`docs/patterns/ob1-inline-budget-divergence.md` - a mem-init-list spelling of the
ctor was measured byte-identical and does not move it.

## Related

- `docs/patterns/ob1-inline-budget-divergence.md`
- `docs/patterns/new-null-guarded-ctor-opened.md`
- `docs/patterns/comdat-inline-ctor-no-standalone.md`
