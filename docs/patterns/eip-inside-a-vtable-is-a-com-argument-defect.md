# EIP inside a `??_7` is wine's ddraw calling slot 0 on a non-COM pointer — read the caller off the popped return address
tags: cpp:class cpp:call cpp:cast | asm:call asm:ret asm:xchg | topic:runtime topic:identity

symptoms: `wine: Unhandled page fault ... at address 005DF0EF`, EIP inside `.rdata` at
`??_7<Class>@@6B@ + 0x17`, four dispatches at one EIP, `info[0]=1` (write) to an address
that equals `eax`, `ebp`/`edi` holding heap pointers, `fixme:ddraw:ddraw_surface1_Flip`
as the last line before the fault.

An EIP inside a vtable is never a transfer *target*: the reported address is where the
CPU, already executing vtable words as instructions, hit a faulting one. Decode the words
as code — a run of `00 00` is `add byte ptr [eax],al`, which is why the fault is a WRITE
to `eax`. Then walk the stream backwards to the only entry that fits: the vtable BASE,
because the only `.rdata` address a live program computes is a **vptr**. Something did
`call *(object)` — one dereference short of a virtual call.

In wine that "something" is ddraw. `unsafe_impl_from_IDirectDrawSurface()` validates a
surface argument as

```asm
    mov  edx,[eax]                 ; eax = the pointer the game passed
    cmp  edx,OFFSET ddraw_surface1_vtbl
    je   fast_path
    ...
    call DWORD PTR [edx]           ; QueryInterface == lpVtbl[0]
```

so if the game passes `p` where `*p` is **another object pointer** rather than a vtable,
`call [edx]` jumps to `*(that object)` — the C++ vptr — i.e. into `??_7`.

**Recovering the caller with no debugger.** The garbage stream itself preserves the
evidence, and every register is checkable:

| observation | reads as |
|---|---|
| a `pop` in the stream | the **return address** ddraw pushed — subtract the module base from `+loaddll` and it names the exact `call` instruction |
| `xchg eax,ebp` at vtable+0 | `ebp_final` == `eax` at entry == the argument ddraw was validating |
| `esp` after the stream | ddraw's frame: `esp = ebp_helper - 0x14`, so `ecx = ebp_helper + 8` pins the whole frame |
| callee-saved `esi`/`edi`/`ebx` | the *wrapper's* locals — which arg went into which register identifies WHICH `ddraw_surface1_*` called the helper |
| `sbb ebx,[ebx]` / `add bh,bl` in the stream | invertible: recover `lpSrcRect`, and its distance above the return address gives the `lea reg,[esp+N]` in the game-side caller |

Worked once end to end: `esi` = a heap pointer and `edi` == the validated argument fits
only `ddraw_surface1_BltFast` (Flip puts `flags` in `esi`); the recovered `lpSrcRect` was
`retaddr + 0x38`, which is `lea edx,[esp+0x20]` — unique among the 22 `CDDSurface::BltFast`
call sites, naming `LayerBlitFrame`.

Not a codegen pattern: it is a DATA defect. Adjudicate it statically — every function on
that path (`LayerBlitFrame` 0x115300, `CDDSurface::BltFast` 0x13ef90, `CDDSurface::Flip`
0x13e850, `CPlay::BuildHelpReveal` 0xd72c0) diffed byte-identical to retail, so the bug is
the value handed in, not the code. Always run `run.sh retail` as the control: retail logs
**0** access violations where the candidate logs ~1400.

**Do not read the emulated register values as data.** Continuation steps each faulting load
OVER, so a destination register keeps its previous contents and a two-instruction indexed
read reports the BASE pointer at every index — see
[`continue-on-fault-retains-the-base-register`](continue-on-fault-retains-the-base-register.md).
Here that turned "`GetAt(1)` and `GetAt(2)` both returned 0x01e2b3a0" into "`m_items.m_pData`
is 0x01e2b3a0", which is a different search. The `CDDrawWorker`/`CImage`/`CObArray` graph
behind it was then swept clean (sizes, offsets, vtable slots, ctor, writer set) — the
elimination table is in `docs/gotchas.md` § "Runtime triage"; do not redo it.
