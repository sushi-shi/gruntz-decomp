# A receiver evaluated BEFORE the index means the lookup was an inline accessor - and the aggregate goes by REFERENCE
tags: cpp:inline cpp:member cpp:local | asm:mov asm:lea | topic:codegen-idiom
symptoms: retail loads a global or member pointer at the very top of the function, before the register pushes, then defers the SECOND deref past the index arithmetic; our flat `g_global->a->b[i]` sinks the global below the arithmetic instead; `walls diagnose` says regalloc because only registers and order differ
confidence: 8/10
variants: typed-return-lookup-wrapper (memory), struct-return-rvo-idioms.md, inlined-mfc-accessors-transcribed-as-raw-offsets.md

cl 5.0 evaluates an inlined call in call order: **receiver, then arguments, then
the body.** A flat member-chain expression has no such barrier, and cl's
scheduler sinks the global load to its first use. So the two shapes are
distinguishable in the emitted code:

```asm
; retail - an inlined call
0004: mov edx,ds:g_gameReg      ; the RECEIVER, before the pushes
000b: mov eax,[esi+0x54]        ; the ARGUMENT
000e: mov ecx,[esi+0x58]
0011: lea eax,[eax+eax*2]
0014: lea eax,[eax+eax*4]
0017: add ecx,eax
0019: mov eax,[edx+0x68]        ; the BODY - ->m_cmdGrid, only now
001c: mov edi,[eax+ecx*4+0x1c]

; ours - one flat expression, global sunk to its first use
0011: add eax,ecx
0013: mov ecx,ds:g_gameReg
0019: mov edx,[ecx+0x68]
001c: mov edi,[edx+eax*4+0x1c]
```

`CGruntHealthSprite::HealthUpdate` 0x7f180 **95.07 -> 100.00 EXACT**:

```cpp
// BASE, 95.07 - the deref chain written out
CGrunt* e = g_gameReg->m_cmdGrid->m_units[m_cell.m_x * TM_UNITS_PER_PLAYER + m_cell.m_y];

// TARGET - an inline accessor, and the cell passed BY REFERENCE
inline CGrunt* GruntAtCell(CGruntzMgr* reg, const Coord& cell) {
    return reg->m_cmdGrid->m_units[cell.m_y + cell.m_x * TM_UNITS_PER_PLAYER];
}
CGrunt* e = GruntAtCell(g_gameReg, m_cell);
```

## The reference parameter is the load-bearing half

Measured on that one function, every variant from the same 95.07 base:

| spelling | score |
|---|---|
| flat chain (base) | 95.07 |
| registry bound to a local, then the flat chain | 96.67 |
| + the index bound to its own local, either operand order | 96.67 |
| the grid array bound to a local | 95.07 |
| operand order alone (`y + x*15` vs `x*15 + y`) | 95.07 |
| **inline helper taking `(CGruntzMgr*, i32 x, i32 y)`** | **95.07** |
| **inline helper taking `(CGruntzMgr*, const Coord&)`** | **100.00 EXACT** |

A local for the receiver gets the global into the right position but leaves
`->m_cmdGrid` hoisted with it - only the call barrier separates them. And
unpacking the aggregate into two `int` formals throws the whole effect away: the
two scalars are evaluated and coloured at the call site exactly as the flat form
does. **Pass the aggregate, by reference, or the lever does not fire.**

## Sieve — and the confound that made it look big

The signature is: align the pair and look for a first divergence in which the
TARGET side holds a relocated global load (`mov reg,ds:0x0`) that the BASE side
also has, but LATER.

**Rank by the INSTRUCTION INDEX gap, never by the byte offset.** A byte-offset
ranking is confounded by the encoding: `mov eax,moffs32` is the 5-byte `A1` form
and `mov <other>,[disp32]` is 6 bytes, so every later reference in a function
where retail happens to colour the global EAX and we do not reads as "retail is
1 byte earlier" at every site — a position difference that does not exist.

Re-derived on instruction index (2026-08-23, whole tree, 16 sub-100 rows whose
FIRST divergence is a relocated global load):

| class | rows | what it is |
|---|---|---|
| same slot, same symbol, different destination register | **13** | the register-rotation wall, not a hoist |
| genuine hoist (target loads it 1-6 instructions earlier) | 3 | this pattern |

The three real hoists are `CGruntPowerupSprite::Update` 0x080410 (+1, already
banked 100.00), `CLightFxRender::BuildHighRollerzPalette` 0x0a2bb0 (+5) and
`BuildGruntziclezPalette` (+6) — the parked palette family. So the sieve's live
worklist is much smaller than the raw hit count suggests.

## The pointer-receiver accessor is byte-INERT — do not spend a build on it

Measured on three of the 13 colour rows, each with a disposable inline helper
taking the receiver as a POINTER, exactly the `(ptr, i32, i32)` row of the table
above:

| row | base | with `inline T* H(Owner*, …)` | with a plain local |
|---|---|---|---|
| `CPlay::OnExit` 0x0cb400 | 94.79 | 94.79 | 94.79 |
| `CTriggerMgr::LoadGruntResurrectTuning` 0x07be60 | 91.97 | 91.97 | — |
| `CNetSession::Verify(i32)` 0x0c0290 | 89.53 | 89.53 | — |

All three were byte-identical tree-wide (`OVERALL 95.3608`, exact 6574, unmoved
to four decimals). **The inline call is not the lever; the aggregate-by-reference
parameter is.** A row whose lookup takes no aggregate has nothing for this
pattern to apply.

## When it does NOT apply

The gap has to be the receiver's POSITION. Two rows with the same top-of-function
symptom did not move at all:

* `CStatusBarMgr::InsertPtr` 0x108410 - retail homes the `a` parameter in eax at
  entry, above the pool branch. Flat under a two-int inline allocator, the same
  taking `const Coord&`, and a plain entry local. C2-anchored (below).
* `CTriggerMgr::DestroyGroup` 0x798d0 - retail accumulates into the register
  holding the first-loaded term at two sites. Flat under the `const Coord&`
  helper, a `Coord*` helper, a Coord copy local (96.60, worse), the other index
  order, statement reordering and two local-binding forms.

**Check C1 reachability before spending a spelling matrix, and check it with a
control.** A stride-1 `typedef` sweep over the declaration count is 15 s per
state inside one `nix develop` shell. Both rows above are flat across 12 states
while their UNIT total moves, which is the control that proves the probe reached
cl - `grunthealthsprite` moves nothing at all in the unit, and there the proof is
that the base obj hash still changes (the symbol table moves, `.text` does not).
Flat sweep plus a moving control means C2-anchored: the register-picker cursor,
not handle state, and a spelling matrix is the right next step rather than more
states. Do not write the opposite conclusion into a source comment - InsertPtr
carried "C1 handle-state, not source" and the sweep falsifies it.
