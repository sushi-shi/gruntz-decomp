# A precompiled-header boundary is a cl 5.0 TU-state input, not a byte-neutral cache

tags: cpp:include msvc5:c1 msvc5:pch | asm:mov asm:lea asm:call | topic:tu-state topic:regalloc topic:flags
symptoms: an exact function rotates address-calculation and virtual-call receiver registers
after `/YX` or `/Yc`+`/Yu`, with the same size, calls, relocations, branches and returns
confidence: 10/10 (2 exact TUs, every direct-include boundary swept, creator/user/automatic controls, independent `_MBCS` reproduction, retail contribution-order audit)
variants: large-sdk-header-in-a-shared-prelude.md, tu-state-probe-family-decides-reachability.md

VC5 can emit different optimized instructions from the same post-include C++
when the prefix was parsed normally versus restored through a PCH. The boundary
is part of the front-end state: test it on exact functions before treating
`/YX`, `/Yc`, or `/Yu` as build-only switches.

## Controlled A/B

Both TUs were 100% exact without PCH:

| TU and boundary | unit result |
|---|---:|
| `directinputmgr2`, `/YcDirectInputMgr2.h` creator | 100% |
| `directinputmgr2`, `/YuDirectInputMgr2.h` user | 100% |
| `directinputmgr2`, automatic `/YX` | 100% |
| `grunthealthsprite`, `/YcGruntHealthSprite.h` creator | 99.46793%, 17/18 |
| `grunthealthsprite`, `/YuGruntHealthSprite.h` user | 99.46793%, 17/18 |
| `grunthealthsprite`, automatic `/YX` | 99.46793%, 17/18 |
| either TU, `/Yc` or `/Yu` at the first minimal `rva.h` include | 100% |

Only `CGruntHealthSprite::HealthUpdate` changes. It remains 180 bytes and
95.072464% fuzzy with equal call, branch, return, and relocation sets. The first
changed region is a register/addressing rotation:

```asm
; retail and no-PCH base
mov  edx, [g_gameReg]
...
add  ecx, eax
mov  eax, [edx+68h]
mov  edi, [eax+ecx*4+1Ch]
...
mov  edx, [esi]
call dword ptr [edx+40h]

; own-header PCH and automatic /YX
...
add  eax, ecx
mov  ecx, [g_gameReg]
mov  edx, [ecx+68h]
mov  edi, [edx+eax*4+1Ch]
...
mov  eax, [esi]
call dword ptr [eax+40h]
```

An independent `/D_MBCS` compile emits the **same 180 function bytes** as the
PCH variant. `/W3`, `/DWIN32`, `/DNDEBUG`, and `/D_WINDOWS` are each exact, and
the Release define bundle without `_MBCS` is exact. Two unrelated changes to
the prefix therefore land on the same codegen island; no semantic change to
`HealthUpdate` is involved.

## Reverse-use rule

- A PCH is not automatically byte-neutral on cl 5.0. Its boundary belongs in a
  TU-state audit alongside include order, declaration kinds, and the C1 IL tap.
- To test a suspected historical PCH, compile both the creator and user forms
  at each evidence-backed header boundary. Use already-exact functions in that
  TU as negative controls. A boundary that moves unrelated exact functions is
  not established merely because it improves one wall.
- A minimal early boundary being neutral does not generalize to a later or
  automatic boundary. The `rva.h` controls above were exact while the main
  class-header boundary moved `HealthUpdate`.
- Do not retain a fake PCH, macro, or include solely to steer registers. Bank a
  reachable MAX if useful, remove the diagnostic, and recover the real
  project/header structure independently.

Evidence run: `build/compiler-flag-audit/pch/`,
`build/compiler-flag-audit/pch-boundaries/`, and
`build/compiler-flag-audit/define-panel/`, 2026-08-26.

## What survives into the executable

The direct PCH record does **not** survive this link:

- a VC5 `.pch` starts with `VCPCH0` and contains its PDB path, the `/Yc`
  boundary spelling, the source path, and traversed header paths;
- the corresponding `.obj` contains no `VCPCH0`, `.pch`, `vc50.pch`,
  `stdafx`, or `precomp` string, and its `.drectve` is identical to the
  non-PCH object's linker directives;
- retail `GRUNTZ.EXE` has `PointerToSymbolTable = 0`, no COFF symbols, no debug
  directory, no overlay after the final `.reloc` byte, and none of those PCH
  strings. VC5 C/C++ objs also supply no `@comp.id`, so the Rich header's 1,057
  anonymous input-object count cannot partition PCH creators and consumers.

There is nevertheless an **indirect linked trace**. PCH restore changes C1's
deferred COMDAT materialization order. LINK 5.10 preserves an object's input
section order (proved independently in `docs/link-text-layout.md`), so reordered
header inlines, vtables, RTTI and EH metadata retain their order in `.text` and
`.rdata` even when every individual contribution is byte-exact.

The full direct-include sweep found two stable boundary classes:

| TU | boundary class | byte comparison | retail-order comparison |
|---|---|---:|---|
| `directinputmgr2` | no PCH or `/Yc`/`/Yu` after only `rva.h` | 56/56 exact | `.text`: 53 inversions over 45 common functions; `.rdata`: **0** over 13 |
| `directinputmgr2` | `DirectInputMgr2.h` or any usable later direct include | 56/56 exact | `.text`: 198 inversions; `.rdata`: **8** |
| `grunthealthsprite` | no PCH or boundary after only `rva.h` | 18/18 exact | late-boundary codegen island absent |
| `grunthealthsprite` | `GruntHealthSprite.h` or any of the remaining 17 usable direct includes | 17/18 exact | `HealthUpdate` is the same 95.072464% island at every boundary |

The DirectInput `.rdata` witness is especially clean. Retail and no-PCH have
the same 13-item order: five vtables followed by the EH funcinfo/unwind-map
pairs for `CKeyboardDevice`, `CInputDevBase`, `CJoystickDevice`, and
`CMouseDevice`. A boundary at `DirectInputMgr2.h` moves the `CInputDevBase` EH
pair after the joystick and mouse pairs. Objdiff still reports 100% because it
compares the contributions by identity, not their eventual linked addresses;
the executable layout distinguishes the states.

Creator versus consumer is not a universal marker. At the own-class-header
boundary, `/Yc` and `/Yu` objects for both TUs are byte-identical after masking
the COFF timestamp. At the early `rva.h` boundary they can differ in section
and RTTI order while retaining identical function bytes; on
`grunthealthsprite`, `/Yc` and `/Yu` produce distinct `.rdata` orders. Thus a
selected, unique contribution can sometimes rank creator versus consumer, but
there is no explicit bit to read and some boundaries collapse to one object.

## Recoverable scope

From the executable we can recover **constraints and equivalence classes**:

- exclude a boundary whose code island contradicts an exact retail function;
- rank surviving boundaries by the address order of unique `.text`, `.data`,
  `.rdata`, RTTI/vtable, and EH contributions;
- occasionally distinguish `/Yc` from `/Yu` when a controlled pair changes a
  unique surviving contribution's order;
- cluster TUs that react alike to one candidate shared-prefix boundary.

We cannot recover the deleted `.pch` path or header name, prove a boundary when
all linked contributions are neutral, or identify a conventional dedicated
`StdAfx.cpp` creator that contributed no surviving unique bytes. Shared-inline
COMDATs kept from an earlier object are also false witnesses: attribute the
selected retail copy before reading its order. The low-RVA
`CGruntHealthSprite` inline group is one such case, so its naive target-object
order is not evidence for `GruntHealthSprite.cpp`'s own PCH state.

Extended evidence run: `build/compiler-flag-audit/pch-prefix-sweep/` and
`build/compiler-flag-audit/pch-trace/`, 2026-08-26.
