# A dead `mov [esp+N],0` beside an int slot that feeds fidiv IS a u32 local's staging

tags: cpp:local cpp:cast | asm:fidiv asm:fild asm:mov | topic:codegen-idiom
symptoms: retail writes `mov DWORD [esp+N+4],0x0` that no instruction ever reads,
adjacent to a slot holding a just-computed int that feeds `fidiv`/`fimul`; our
base has neither store; our cl caches a member (e.g. m_mode) in the one free
callee-saved register across the sibling __ftol calls where retail re-reads it
per condition group
confidence: 8/10
variants: fild-qword-proves-an-unsigned-member.md

The member-type oracle (fild-qword-proves-an-unsigned-member) has a LOCAL twin.
`(double)u32local` forces the qword staging pair - lo = value, hi = 0 - and when
the local's only use is that conversion, the hi-zero store is the ONLY visible
trace: retail's cl then folds the divide down to `fidiv DWORD [lo]` (signed!,
correct only below 2^31 - their fold, not reproducible), leaving the hi store
dead. Declaring the local `u32` in our source:

```cpp
u32 arcSpan = arc - m_halfWidth;                 // not i32
colBase = stride - (i32)((double)stride / arcSpan * tail);
```

emits the same staging pair (hi first, then lo - `fild qword` + `fdivp` instead
of the fidiv fold, +1 insn per site) AND, decisively, re-weights the register
allocator: the staging keeps arcSpan out of a register, `arc` wins the free
callee-saved reg at the __ftol return (`mov edi,eax`, retail's shape) and the
member the old i32 spelling let cl hoist across both calls (m_mode in edi) goes
back to a per-condition-group re-read - re-aligning the WHOLE downstream
coloring. Casting the divisor back (`/ (i32)arcSpan`) or parenthesising reverts
everything including the hoist (measured, 9-cell matrix identical).

CFaderShape::RenderWarpTile 0x181e50 60.52 -> 79.74 (with the natural
per-statement loop respellings the fixed coloring then permits). The residual
fidiv-vs-fild-qword fold is not spelling-reachable.
