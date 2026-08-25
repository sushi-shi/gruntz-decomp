# `mov r,[m]; or r,imm; mov [m],r` instead of `or [m],imm` means the source used a named temp

**Symptom.** One row in the diff:

```
- and dword ptr [eax],0xfffbffff        <- base
+ mov ecx,dword ptr [eax]               <- retail
+ and ecx,0xfffbffff
+ mov dword ptr [eax],ecx
```

Three instructions in retail where cl gave us one. It looks like a pointless
pessimization, so it gets ignored — but it is a two-instruction cost on every
occurrence and it perturbs the register assignment for everything downstream.

**Cause.** cl5 folds a compound assignment on an lvalue (`p->f |= K`,
`a[i] &= K`) into a single memory read-modify-write. It does **not** fold a
read into a named local, an operation on the local, and a store back. Retail's
source therefore reads:

```cpp
i32 flags = cell[i];
flags &= ~0x40000;
cell[i] = flags;
```

not `cell[i] &= ~0x40000;`.

## Where it shows up

Bit-flag updates on a struct/array element that the surrounding code also reads
for other reasons — the dev had the value in a variable anyway. Both live
examples in this codebase are exactly that:

- `CGrunt::FinishToobMoveAnimation` clearing the tile-grid flag bit,
- `CGruntCreationPoint::CGruntCreationPoint` setting `m_object->m_flags |= 0x20000`
  (where the same `m_object` pointer is live from the `m_sortKey` test just
  above — retail keeps it in a register and does load/or/store through it, while
  the compound form made cl *reload* `m_object` and RMW).

## Caveat

The reverse is equally real: plenty of retail sites DO emit `or [mem],imm`.
Read the target before rewriting - this is a per-site readout of how the source
was spelled, not a global rule. A member the surrounding code does not otherwise
touch usually stays a compound assignment.
