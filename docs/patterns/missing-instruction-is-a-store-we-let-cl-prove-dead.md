# An instruction retail has and we DON'T, which is provably redundant, means our source made it provable
tags: cpp:local cpp:assign | asm:mov | topic:codegen-idiom
symptoms: the diff's only content is extra TARGET lines — a `mov [esp+N],0x0` before an out-param call, or a `mov reg,[p+N]` … `mov [p+N],reg` pair that writes a field back unchanged; both look like dead code retail "should" have removed
confidence: 9/10

Scan for these deliberately: classify each diff by whether the plus-lines are a strict
superset of the minus-lines. A missing instruction is never a scheduling wall — it is a
statement our source does not have, or one cl deleted because our spelling let it prove
the store redundant. Two recurring shapes:

**1. An out-param the caller zeroes.** `Lookup(key, out)` idioms in this codebase are
often preceded by an explicit `out = 0` even though the callee writes it.
```cpp
CObject* outOb = 0;      // retail: mov DWORD PTR [esp+0x18],eax  (eax==0) before the call
reg->m_imageRegistry->m_workersByName.Lookup(buf, outOb);
```

**2. A save/restore around a state change.** cl deletes `p->f = p->f` when both accesses
go through the SAME expression, and keeps it when they do not — so retail keeping the
pair tells you the intervening store reached the object by a different path.
```cpp
CWwdGameObjectA* h = m_object;
i32 keep = h->m_drawFillCmd;
m_object->m_drawActive = 1;   // through the MEMBER, not `h` -> the pair survives
h->m_drawFillCmd = keep;      // retail: mov ecx,[esi+0x50] … mov [esi+0x50],ecx
```
The two loads still CSE to one register, so nothing else in the function moves.

STEERABLE, both. `CSBI_ImageSet::SerializeFields` @0xe74f0 99.18 -> **100 EXACT** (1),
`CGrunt::LoadCellAnimNames` @0x48470 99.64 -> **100 EXACT** (2).
