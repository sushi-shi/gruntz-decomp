# Force spill+rewalk of a read-then-write cell: write through the un-cached global

**Tags:** cpp:local cpp:cse | asm:mov asm:lea | topic:codegen-idiom topic:regalloc

## Symptom

A "swap the parked value" body reads a cell then writes the cell through the
**same** long pointer-chain expression:

```cpp
i32 newTok = ((RegM30*)mgr->m_world)->m_24->m_5c->m_cells[ ...->m_rowBase[idx] + grp ];
((RegM30*)mgr->m_world)->m_24->m_5c->m_cells[ ...->m_rowBase[idx] + grp ] = oldTok;
Notify(grp, idx, oldTok);
this->m_34 = newTok;
```

Retail (higher register pressure) **spills `newTok` to a stack local** and
**re-walks** the whole `m_world->m_24->m_5c->m_rowBase/m_cells` chain a second time
for the write:

```
mov edx,[edx+ebp*4]        ; newTok
mov dword ptr [esp+0x1c],edx   ; SPILL newTok to a stack slot
mov edx,edi                    ; edx = mgr (kept in edi)
mov edi,[edx+0x30] ...         ; RE-WALK W->L->V->rowBase/cells
mov [edi+ebp*4],ebx            ; write oldTok
call Notify
mov eax,[esp+0x10]             ; reload newTok
mov [esi+0x34],eax
```

Your recompile instead **CSE-collapses the read and write address into one `lea`**
and keeps `newTok` in a callee-saved register across the call — no spill, no rewalk:

```
lea edi,[edi+4*ebp]        ; ONE cell address
mov ebp,[edi]              ; read newTok into ebp
mov [edi],ebx              ; write oldTok through the same address
...
mov [esi+0x34],ebp         ; newTok held in ebp across the Notify call
```

Plateau ~62% — the whole write half diverges.

## Cause

MSVC5 /O2 CSEs the identical read-address and write-address into a single computed
pointer, which drops the register pressure enough to keep `newTok` in a callee-saved
register. Retail's build had the extra pressure (it also keeps `mgr`, `idx`, `grp`
live), so `newTok` spills and the second address is **rematerialized** (re-walked)
instead of kept live.

## Fix (steerable)

Break the read/write address CSE so the write forces a second walk. Read the cell
through a **cached local** (`mgr`) but write it through the **un-cached global**
(`g_mgrSettings`) — the two spellings are value-equal but MSVC5 does not merge their
address computations, so the write re-walks and the pressure spikes → `newTok`
spills to a stack local exactly like retail:

```cpp
CGameRegistry* mgr = g_mgrSettings;      // cache: raises pressure, one global load
i32 grp = this->m_08, idx = this->m_0c;  // read-once, shared with Notify args
i32 newTok = ((RegM30*)mgr->m_world)->m_24->m_5c->m_cells[ ...->m_rowBase[idx] + grp ];
((RegM30*)g_mgrSettings->m_world)->m_24->m_5c->m_cells[ ...->m_rowBase[idx] + grp ] = oldTok;
Notify(grp, idx, oldTok);
```

`CSlotHolder::DoSwap` (0x1128b0): 54.9 → 62 (cache `mgr` + `idx`/`grp` locals) →
**87.6** (un-cached-global write defeats the CSE). Residual is pure regalloc naming
(mgr=edi/idx=eax/grp=ecx vs base ecx/edx/eax) — a `topic:wall` coin-flip.

## Confidence

c7 — reproduced live (mgrslotswap). The un-cached-global write is the load-bearing
lever; without it the spill never happens.
