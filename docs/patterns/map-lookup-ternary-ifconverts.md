# `Lookup(k, out) ? out : 0` if-converts — write the STATEMENT form
tags: cpp:ternary cpp:branch mfc:map | asm:neg asm:sbb asm:and | topic:codegen-idiom
symptoms: base emits `neg eax / sbb eax,eax / and eax,<out>` right after an MFC
  `CMap*::Lookup` call where retail emits `test eax,eax / je L / mov eax,[out] / L:`;
  ~3% of the function, and the extra live value shifts a register elsewhere
confidence: 9/10

## Shape

The MFC map API hands the value back through a `void*&` out-param and returns a `BOOL`,
so every "look it up or null" site is naturally spelled as a ternary:

```cpp
CGameObject* found = 0;
CGameObject* obj = MapLookupById(grp->m_map48, id, found) ? found : 0;   // <- if-converts
```

At `/O2` MSVC 5.0 **if-converts** `cond ? x : 0` into the branchless mask idiom:

```asm
    call    <CMapPtrToPtr::Lookup>
    mov     ecx,DWORD PTR [esp+0x18]     ; found
    neg     eax
    sbb     eax,eax                      ; eax = (ret != 0) ? -1 : 0
    and     eax,ecx
```

Retail never does. It keeps the branch and reuses the **FALSE return value already in
`eax`** as the null:

```asm
    call    <CMapPtrToPtr::Lookup>
    test    eax,eax
    je      L                            ; on this edge eax is provably 0
    mov     eax,DWORD PTR [esp+0x18]     ; found
L:  mov     edx,DWORD PTR [eax+0x7c]
```

## The fix

Spell it as a **statement**, not an expression. cl5 does not if-convert an `if` whose
body is a plain assignment:

```cpp
CGameObject* found = 0;
CGameObject* obj = 0;
if (MapLookupById(grp->m_map48, id, found)) {
    obj = found;
}
```

## Measured

| function | before -> after |
|---|---|
| `CExitTrigger::SerializeMove` @0x3f040 (read-side warlord id resolve) | 93.68 -> **96.65** |
| `CExitTrigger::AdvanceAnim` @0x3f5f0 (the claimed-slot warlord resolve) | 76.85 -> **78.22** |

Both had been filed as "branch-vs-branchless coin-flip, not source-steerable". It is
steerable; it is the ternary.

## Residue this does NOT close

cl still materialises `obj = 0` in its own register before the call (`xor esi,esi`)
where retail simply falls out of the `je` with `eax == 0`. That half is a register
colouring choice and stays.

related: identical-return-epilogue-tailmerge.md, positive-gate-enables-shrink-wrap.md,
[default-hoists-into-destination-no-jmp.md](default-hoists-into-destination-no-jmp.md)

BOUNDARY (measured on `CDDrawChildGroup::PruneOrphans` @0x15b1d0, 2026-08-01): when the
resolved pointer's ONLY consumer is a null test, the statement form still if-converts and
the two-compare `||` chain — which is NOT retail's shape — scores higher than any value
spelling: 85.93 (`owner=0; if(hit) owner=found;`) / 85.93 (ternary) / 88.18 (explicit
if/else) vs **93.75** for `if (Lookup(k,found) == 0 || found == 0)`. Keep the `||` there.
`CInGameIcon::Reposition` @0x098a90 repeats it with the pointer DEREFERENCED afterwards
(92.83 / 93.26 vs **97.43** for the `&&` chain), so "only null-tested" is not the
discriminator. Measure both spellings on this shape rather than assuming.

Two more sites, both directions measured 2026-08-23, and neither agrees with the
other:

| function | spelling | fuzzy |
|---|---|---|
| `CGrunt::StepEntranceRelatchB` 0x65c20 | `found=0; if (Lookup(..)==0) found=0;` (the re-assign) | **98.22** |
| | `obj=0; if (Lookup(..)) obj=found;` (statement form) | 90.86 |
| | `obj = Lookup(..) ? found : 0` (ternary) | 96.82 — if-converts |
| | `if (Lookup(..)==0 \|\| found==0) { ... }` | 79.97 |
| `CDDrawChildGroup::Deserialize` 0x15b0e0 | statement form (in tree) | **96.17** |
| | explicit `if/else`, both arms assigning | 91.44 — if-converts |

The `Deserialize` result is the one to remember: the explicit `if`/`else` is NOT a
safe fallback when the statement form is not reaching retail. It if-converts just
like the ternary (`mov ecx,[esp+0x2c] / neg esi / sbb esi,esi / and esi,ecx`),
while the statement form keeps the branch. Retail wants the branch WITH the zero
materialised INSIDE the miss arm (`jne 0x71 / xor esi,esi / jmp 0x75`), which is
what cl emits when the destination register is live across the call with another
value — a register-pressure fact, not a spelling. No spelling reached it.

`StepEntranceRelatchB` says the same thing from the other side: the redundant
`found = NULL` re-assign that this pattern would normally call the bug is the
best-scoring form there by 1.4 points, because `found`'s address has escaped to
the out-param and cl must keep memory coherent either way. **Check the tree's
current spelling and its bank before applying the fix; on a site where the doc's
preferred form is already beaten, it stays beaten.**
