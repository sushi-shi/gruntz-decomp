# `if (p) { delete p; p = 0; }` — cl elides delete's redundant null-guard; the zero-store is GUARDED

- confidence: 9
- tags: cpp:dtor cpp:branch cpp:member | asm:test asm:je asm:call asm:mov | topic:codegen-idiom
- status: steerable

## Symptom

An owned-child teardown run (`test/cmp; je; push 1; call [eax+4]; mov [this+X],0` per
child) plateaus with ONE of two off-by-one-instruction shapes:

1. **Spelled `p->Destroy(1)` through a local vtable-view struct + `if (p) { ...; p = 0; }`**
   (the manual scalar-delete hack): the block ORDER matches retail but the /O2
   register allocation recolors (e.g. retail ebx=0/edi=cached vs cl edi=0/ebx=cached) —
   misread as a "zero-register-pinning wall".
2. **Spelled plain `delete p; p = 0;`**: the registers now match (the compiler's own
   delete temp restores the true allocation) but every zero-store swaps with the NEXT
   child's load — base hoists `mov ecx,[esi+NEXT]` above `mov [esi+CUR],ebx`, retail
   stores first (2-line reorder per child in the diff).

## Cause

Retail's per-child shape is `cmp p,0; je NEXT_LOAD; <scalar-delete call>; mov [p],0;
NEXT_LOAD:` — the je lands ON the next child's load, i.e. the zero-store is **guarded**:
the devs wrote

```cpp
if (m_child) {
    delete m_child;
    m_child = 0;
}
```

and MSVC 5.0 /O2 **elides `delete`'s own null-guard** when the pointer is flow-known
non-null in the same extended basic block (the source `if` already tested it) — so
there is exactly ONE test per child, the delete call, then the guarded store. The
unguarded `delete p; p = 0;` spelling instead keeps delete's own guard, places the
store on the fallthrough+skip join, and frees the scheduler to hoist the next load.

## Fix

Model the children with REAL virtual destructors (canonical classes, dtor at its true
slot — no `Destroy(u32)` view) and spell the teardown `if (p) { delete p; p = 0; }`.
Both residual shapes close simultaneously (order AND regalloc).

## Evidence

`CDDrawSurfaceMgr::Cleanup_155e20` (0x155e20, 209 B, 11 children incl. a slot-0-dtor
SoundStream and a non-virtual-dtor `delete m_ptrColl` → `call ??1; push; call ??3`):
shape-1 spelling was parked `@early-stop` at ~96% as a regalloc wall; shape-2 hit
95.8% (10 store/load swaps); the `if (p) { delete p; p = 0; }` spelling → **100.0%
EXACT** (view-burndown session 2026-07-05). Refutes the former
`zero-register-pinning` diagnosis for this fn.
