# An INLINED ctor/dtor inside a `$E` helper proves the compiland was built WITHOUT /GX
tags: cpp:ctor cpp:dtor cpp:static cpp:eh | asm:call asm:jmp asm:mov | topic:tu-layout topic:codegen-idiom topic:identity

symptoms: a `$E<n>` dynamic-init helper whose body is `mov ecx,<global>; call <BASE ctor>;
mov <global>,<vtable>; mov <global>+8,<vtable2>; ret` instead of `mov ecx,<global>; jmp
<the class's own ctor>`; a `??_G<Class>` deleting destructor stuck in the 30-45% band while
every other function in its unit is EXACT; retail has NO out-of-line `??0`/`??1` for a class
whose objects are clearly constructed; a `$E` helper carrying a whole loop

confidence: 10/10

MSVC 5.0 emits four compiler-private helpers per file-scope object with a non-trivial
ctor/dtor (see [[msvc-static-object-e-helper-family]]). **Whether the class's own
constructor and destructor are INLINED into those helpers or merely CALLED from them is
decided by `/GX`, and by nothing else.**

| flags | the ctor helper |
|---|---|
| `/O2` | `push <args>; mov ecx,<obj>; call <BASE ctor>; mov <obj>,??_7…; ret` — the derived ctor is **expanded** |
| `/O2 /GX` | `push <args>; mov ecx,<obj>; call ??0<Class>@@…; ret` — an **out-of-line call**, and cl additionally emits the `??0` COMDAT |

Measured with a five-way probe against the pinned cl 5.0 SP3: an in-class ctor body, an
out-of-class `__inline` body, `#pragma inline_depth(255)`, internal linkage, and a TU that
also uses `try`/`catch` **all** leave the call in place under `/GX`; dropping `/GX` alone
flips every one of them to the expanded form. (`/Ob2` is a different shape again — it
collapses the whole four-helper family into a single function, so a retail image that keeps
the 4-part split is `/Ob1`, i.e. plain `/O2`.)

## Why this is a TU-boundary oracle, not a codegen curiosity

`/GX` is a per-compiland flag. So an inlined `$E` helper **proves its object's defining
compiland was non-`/GX`** — regardless of what the neighbouring functions look like. In
`GRUNTZ.EXE` at 0x16e690..0x16e7e7 that reading splits one apparent unit in two:

```
0x16d700  push <str>; mov ecx,g_zBitSetErrorSlot; call ??0CVariantSlot; ret   <- NOT inlined
0x16d9b0  ... g_globalErrorSlot ...                                          <- NOT inlined
0x16de20  ... g_dynamicArrayErrorSlot ...                                    <- NOT inlined
0x16dfe0  ... g_symTabErrorSlot ...                                          <- NOT inlined
0x16e6a0  push 0; push ButeTreeNopFree; mov ecx,g_buteTree;
          call ??0zPTree; mov g_buteTree,??_7CButeTree@@6B@;
          mov g_buteTree+8,??_7CButeTree@@6BzPtrColl@@@; ret                 <- INLINED
0x16e730  ... g_typeColl, _zdvec base ctor + the grown-slot loop ...          <- INLINED
```

The four non-inlined helpers are **interleaved** with functions that carry `/GX` EH frames
(`push -1; push <handler>; mov fs:0,esp` at 0x16d3a0, 0x16d710, 0x16d790, 0x16de30,
0x16dff0). The two inlined ones sit in one block **after** the last of those functions.
Two compilands, cleanly separated by address — `TypeKeyColl.cpp` (`/GX`) and a small
globals-only compiland (no `/GX`) that defines `g_buteTree` and `g_typeColl`. Modelling
that split as `src/Bute/ButeGlobals.cpp` with a `cpp-noeh` flags profile made all four
helper bodies byte-identical to retail.

## The scored payoff is the `??_G` deleting destructor

`??_G<Class>@@UAEPAXI@Z` inlines `~<Class>` under exactly the same rule, so a class whose
destructor retail expands will sit in the 30-45% band forever in a `/GX` unit — the diff
shows a `call ??1…` where retail has the whole loop. Homing the object (and the ctor/dtor
bodies) into the non-`/GX` compiland fixed both at once:

- `??_GCButeTree@@UAEPAXI@Z` 0x16e9c0 — **45.58% -> 100.00% EXACT**
- `??_GCTypeCollRuntime@@UAEPAXI@Z` 0x16ea20 — **31.59% -> 100.00% EXACT**

## Corollary: a missing out-of-line `??0`/`??1` is evidence, not a gap

If retail has no `??0<Class>`/`??1<Class>` body anywhere but its objects are plainly
constructed, the ctor/dtor were inline-only — which under this rule additionally says the
owning compiland was non-`/GX`. Do not go looking for an out-of-line body to pin.

## How to check a whole tree at once

Every retail `$E` body should appear byte-identically (relocations masked at the base
object's own relocation offsets) somewhere in `build/objdiff/base/*.obj`. A row of
`config/retail/compiler-generated-functions.tsv` with no byte-equal counterpart is a real
source defect — a wrong ctor argument, a missing global, or exactly this `/GX` mismatch.
That sweep found 3 defects out of 368 rows; after the split, 368/368 match.

Evidence: probe matrix under `cl /O2 /MT [/GX]`; retail 0x16e6a0 (0x26 B), 0x16e6e0
(0x3e B), 0x16e730 (0x51 B), 0x16e7a0 (0x48 B) all byte-exact after the split;
`config/units.toml` profile `cpp-noeh`. Related: [[msvc-static-object-e-helper-family]],
[[function-local-static-dynamic-init-guard]].
