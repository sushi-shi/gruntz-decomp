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

## The ~99.6% variant is a LOAD HOIST, not a sunk store (measured 2026-07-28)

When the base ctors are all INLINE (so the whole init is a straight store run) the same
family caps at **~99.6% with a single ADJACENT TRANSPOSITION**, and the direction is the
opposite of the paragraph above — the vptr store is where it belongs and the BODY'S FIRST
LOAD hoists over it:

    retail   mov [esi+0x3c],eax  /  mov [esi],<leaf vtbl>  /  mov eax,[esi+0x38]
    ours     mov [esi+0x3c],eax  /  mov eax,[esi+0x38]     /  mov [esi],<leaf vtbl>

Proof that the store is NOT sunk: replace the body with a pure STORE (`m_prevAnimSetNode = 7;`)
and cl emits `mov [esi],<vtbl>` FIRST, then the store. The vptr store sits at the canonical
end-of-initialization position in both compiles; only a load moves. (cl also emits leaf
MEMBER-inits *before* that store, not after — see the CPathHazard listing below.)

The hoist is one slot and no source spelling reaches it. Tried and all identical on
CGuardPoint::CGuardPoint @0xae5f0: a local temp for the receiver (`CWwdGameObjectA* o = m_38;`),
an explicit read/modify/write pair, routing the `|=` through an inline member function, adding a
trailing store, swapping the mem-init list order (`: CWapX(obj), CUserLogic(obj)`), and
permuting the CWapX base ctor's own store order (putting `m_3c` first only re-blocks the hoist
against the `m_38` store — it still clears the vptr store). the retired permuter found
**0 applicable AST mutations** (single-statement body) and 384 candidates incl. 48 TU-state
trials moved nothing.

**What DOES suppress it: the leaf having data members of its own.** `CPathHazard::CPathHazard`
@0xb35a0 has the identical first statement (`m_38->m_flags |= 0x2000002;`) and matches, because
its eight CHazardTimer member-init stores sit between the last base store and the vptr store:

    mov [esi+0x3c],eax / mov [ebp],0x108(esi) ...x8 / mov [esi],<vtbl> / mov eax,[esi+0x38]

There the load could legally hoist ~9 slots and does not move at all — i.e. cl only performs
the *adjacent* swap, and only in the low-pressure straight-line case. A leaf whose SIZE is
exactly `base + base` (0x54 == CUserLogic 0x34 + CWapX 0x20, `new` size-proven) has nowhere to
put a member, so this is unreachable for that family and the transposition is a hard wall.

WALL (99.58-99.68%): ??0CWayPoint@@ 0xae3f0, ??0CGuardPoint@@ 0xae5f0, ??0CLevelTime@@ 0x9b8b0,
??0CWarpStonePad@@ 0x10d650, ??0CSingleAnimation@@ 0xae7f0 — all five differ from retail by
exactly this one transposition and nothing else. Retail's RTTI base-class arrays were re-checked
(`.?AVCWapX@@` at mdisp 52 in each COL) so the two-base model is not the cause.
