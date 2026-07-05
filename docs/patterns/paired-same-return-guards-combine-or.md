# Paired guards with the SAME return: combine with `||` to share the exit block

**Tags:** cpp:branch cpp:return | asm:test asm:jne asm:je | topic:codegen-idiom

## Symptom

Two adjacent early-guards that return the **same value** are written as separate
`if`s:

```cpp
if (m_hudSuppressed != 0) return 1;   // guard A
if (m_guts == 0)          return 1;   // guard B
...
if (m_overlayDrag != 0)          return Dispatch();  // guard C
if (g_sbiMgr->m_68->m_400 == 0)  return Dispatch();  // guard D
```

Retail (/O2) **tail-merges** each pair into ONE shared exit block — every guard
`jne`/`je`s *forward* to a single epilogue:

```
mov eax,[edi+0x484]; test eax,eax; jne  0xce9b3   ; A -> shared "return 1"
mov ecx,[edi+0x2dc]; test ecx,ecx; je   0xce9b3   ; B -> same block
mov eax,[edi+0x4fc]; test eax,eax; jne  0xce995   ; C -> shared "return Dispatch"
... m_400 ...       ; test eax,eax; je   0xce995   ; D -> same block
```

Your /O2 recompile instead **inlines a full epilogue at each guard** (guard true ->
fall through to an inline `mov eax,1; pop…; add esp,N; ret`; guard false -> `je`
skip):

```
mov eax,[edi+0x484]; test; je L1; mov eax,1; pop…; ret   ; INLINE
L1: mov ecx,[edi+0x2dc]; test; jne L2; mov eax,1; pop…; ret ; INLINE (dup)
```

Plateau ~50% — the top of the function fully diverges (this is the mirror of
`identical-return-epilogue-tailmerge`, where retail inlines and you tail-merge).

## Cause

MSVC5 /O2 shares an exit block when the *source control flow* already funnels there.
Two separate `if(a) return X; if(b) return X;` statements each get their own return
block and /O2's cross-jump does not always re-merge them; a single `if(a||b)return X;`
lays down ONE return block that both short-circuit tests jump to.

Confirming tell: a guard **already** written with `||` (e.g.
`if(m_dragInhibit1||m_dragInhibit2) return Vslot0e();`) already tail-merges (both
tests jump to one block) even in the un-fixed recompile — proof the `||` shape is the
lever, not the optimizer level.

## Fix (steerable)

Combine each same-return pair with `||` (value-identical short-circuit):

```cpp
if (m_hudSuppressed != 0 || m_guts == 0) return 1;
if (m_overlayDrag != 0 || g_sbiMgr->m_68->m_400 == 0) return Dispatch();
```

Each pair emits two tests (`test;jne` / `test;je`) that both branch to the one shared
epilogue — exactly retail's shape. A field loaded for the second test that the body
reuses (e.g. `m_2dc` in ecx) stays live, matching retail. `CPlay::HandleMousePress`
(0xce660): 51.7 -> 84.2 (residual = an unrelated 4-byte stack-coalesce wall).

Do NOT reach for /O1: the tail-merge looks like an /O1 "favor size" tell, but /O1 also
adds an ebp frame + zero-register pinning that retail's /O2 lacks (regressed 51.7->45).

## Confidence

c8 — reproduced live (gamemousehandler). Only pairs returning the *same* value merge;
heterogeneous returns stay distinct.
