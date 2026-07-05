# A /GX builder's `this`-register (esi vs ebp/edi) is decided by the WHOLE body's register pressure
tags: cpp:eh cpp:new cpp:ctor cpp:member | asm:mov asm:push | topic:regalloc topic:eh topic:codegen-idiom
symptoms: a big /GX per-item builder (`new CSBI_X; Configure; on fail delete; AddTail`, ~37
  sites across a `switch`) plateaus in the low 20s% while a PARTIAL reconstruction. Adding the
  out-of-line base ctor raises the retail `push -1/fs:0` /GX frame but lands `this` in ebp/edi
  (Order-B) - retail is `mov esi,ecx` (Order-A). Every esi-relative access in retail is
  ebp/edi in the recompile, so the whole body mis-scores.
confidence: 8/10

## Shape

`CStatusBarMgr::LoadTabSprites` (0x102250, 7629 B) / sibling `CGameMenuMgr::BuildGameMenu`
(0x101580): a `switch(tab)` that, per tab, `new`s each widget, stamps its vtable+tag,
`Configure`s it from a key+rect, and `AddTail`s it, under a /GX EH frame (the just-created
item is EH-rolled-back if a later `Configure` throws). The base ctor must be OUT OF LINE
(declared-only `CSbConfigItem();`) so the `new`-expr's opaque may-throw `call ??0base`
registers the operator-delete-on-throw cleanup and raises the frame — see
[gx-frame-outofline-ctor](gx-frame-outofline-ctor.md).

Retail's prologue is Order-A:

```asm
mov  esi,ecx              ; this -> esi (loaded FIRST)
mov  eax,[esi+0xc]        ; code
mov  [esp+0x10],eax       ; SPILL code to memory
mov  ebp,[esi+0x14]       ; by -> ebp        (reused as an item ptr in the loops)
mov  ebx,[esi+0x10]       ; bx -> ebx
xor  edi,edi             ; edi = 0-constant (reused ~50x: null-checks, m_30 stamps, push 0)
```

A bare partial + out-of-line ctor instead lands `this->ebp` then `this->edi`, keeping `code`
in a REGISTER (esi) — because with few esi uses and few 0-uses there is a spare callee-saved
register, so the allocator has no reason to spill `code` or dedicate edi to `0`.

## The steer (STEERABLE, not a wall)

Reproduce retail's register PRESSURE by reconstructing the WHOLE body, in particular:

1. **The esi-relative cache stores** the builder stashes each created widget into
   (`m_204[i]`, `m_218`, `m_364..m_628`, ...). Each `this->m_slot = it` is an esi use; a
   few dozen of them make `this` the hottest pointer -> the allocator pins it to a
   callee-saved reg. (Even WITHOUT the /GX frame this alone flips `this->esi`.)
2. **The full item runs** (every `new`/null-check/stamp/`push 0`). The 0-constant is used
   ~50x across the body; once there are enough uses, cl dedicates edi to `0`, which removes
   the last spare register and forces `code` to SPILL to `[esp+0x10]` (re-read per Configure,
   exactly retail) — freeing esi for `this`.

Verified on LoadTabSprites: bare partial 24% (this->ebp) → add cache stores 24.4% (this->esi
w/o /GX) → out-of-line ctor 25% (this->edi w/ /GX, code still in esi) → reconstruct the Game
WARPSTONE run + Resource BELT + Multiplayer HEAD slots/loop 44.8% with the prologue now
byte-for-byte Order-A (`mov esi,ecx`). Each reconstructed item run stepped the % up
(+8.5/+5.3/+3.5/+1.9) AND rebalanced the allocator toward retail's choice.

Corollary: do NOT judge the out-of-line-ctor / register layout on a partial (it looks like a
regression, 24%->20.6%); it is a whole-body property. Reconstruct the runs first, THEN read
the prologue. Residual after the flip: stack frame 0x28 vs retail 0x34 (the still-missing
ConfigureEx/ebp-reuse loop induction locals shift `[esp+N]`; they close as those loops land).

## See also

[gx-frame-outofline-ctor](gx-frame-outofline-ctor.md) (the /GX-frame half),
[identical-return-epilogue-tailmerge](identical-return-epilogue-tailmerge.md).
