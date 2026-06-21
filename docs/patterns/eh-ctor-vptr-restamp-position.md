# /GX ctor: the leaf vptr re-stamp lands in EH state 0, not the entry state — vs retail's eager stamp
tags: cpp:ctor cpp:eh cpp:virtual | asm:mov | topic:wall topic:eh
symptoms: body+offsets byte-identical except ONE `mov [esi],<vtbl>` is scheduled ~4 instrs late (after the m_NN arg-load + the EH-state write) instead of right after the base ctor `call`; ~94-95%
confidence: 7/10

In a `/GX` (`flags="eh"`) constructor whose base ctor is OUT-OF-LINE (reloc-masked engine ctor)
and whose body's first throwing call is on a member (`m_38->Foo(...)`, NOT a virtual on `this`),
MSVC5 schedules the leaf's most-derived **vptr re-stamp** (`mov [this],&vftable`) INSIDE the
first throwing call's EH state (state 0), i.e. AFTER the member-load + the `[esp+N]=state` write.
Retail instead emits the re-stamp EAGERLY — right after the base ctor returns, in the ctor's ENTRY
EH state, before the member load. The single `mov` is shifted ~4 instructions, and the downstream
register allocation diverges (e.g. a `|=` that retail does reg-form keeping the base ptr live, ours
does memory-form + a reload), so the function caps ~94-95% with the body otherwise byte-identical.

NOT steerable by source spelling — BOTH spellings of the vptr produce the identical late position:
- POLYMORPHIC class (compiler auto re-stamp via `virtual ~T()` + in-TU `??_7`) — late.
- EXPLICIT manual stamp (`*(void**)this = &g_vtbl` written as the FIRST body statement) — late.
The position is decided by MSVC5's /GX EH-state machine (the vptr store is sunk into the throwing
call's state because the body never observes the vptr — no virtual call on `this`), a cousin of the
eh-state-numbering-base wall. A separate, often-co-occurring residue (the `|=` reg-vs-mem form +
base-ptr reload) IS steerable by caching the base pointer in a local (`T* o = m_10;` → reg-form OR)
— that part is pin-local-for-callee-saved-reg.md; only the vptr-position `mov` is the wall.

WALL. Evidence: the CGruntStaminaSprite/ToyTime/WingzTime ctors (gameobjectctors unit) — out-of-line
CGruntSprite base (0x7eb00), leaf vtbls 0x5e7a44/0x5e79ec/0x5e77cc; flipping from the base-flags
stub plateau (~60%, no /GX frame at all) to 94.76% in the eh unit, residue = ONE late vptr `mov`.
related: eh-state-numbering-base.md (state-ID encoding residue), gx-frame-destructible-local.md
(the flags="eh" trigger), pin-local-for-callee-saved-reg.md (the co-residue that IS steerable).
