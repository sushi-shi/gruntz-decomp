# Act registrars: the grow-fail report is OUTLINED, and which sites keep it is cl's inline budget
tags: cpp:inline cpp:template cpp:global | asm:call asm:push | topic:codegen-idiom
symptoms: a TWO-key activation registrar (`?RegisterActs@C*@@SAXXZ` / `RegisterActs_*` /
`RegisterIconActions`, retail size **0x2ac**) stuck at exactly **95.2090%** (or **71.98%** when a
second accessor also outlines); `--blocks --diff` shows three 9-instruction base blocks against
6-instruction target blocks; the diff is always the same six lines
confidence: 10/10

The grow-on-miss tail of `_zvec::IndexToPtr` / `zDArray<T>::ResolveEntry` is a real out-of-line
function - **`zErrHandling::Report`** (`0x34960`, `?Report@zErrHandling@@QAEXPAXH@Z`), whose body is
exactly

```cpp
g_retAddrBreadcrumb = GetRetAddr();
m_errSink->Set(this, sentinel, code);
```

cl5 expands that tail at some lookup sites and leaves a `call` at others. **Which** is decided by
the enclosing function's inline budget, not by anything local:

| enclosing registrar | lookup sites | `Report` expanded |
|---|---|---|
| one-key, `0x18d` (`CLightFx::RegisterActs` @0x9d320) | 2 | **both** |
| two-key, `0x2ac` (all nine of them) | 4 | **only #3** - the SECOND key's name lookup |
| 19-key, `0x9e5` (`RegisterActs_644af0` @0x5be30) | 38 | none - the whole accessor outlines to `IndexToPtr`/`Resolve` |

The two forms are trivially distinguishable in the disasm:

```asm
; EXPANDED (source spells the two statements)          ; OUTLINED (source calls Report)
mov  esi,ds:0x6bf464     ; g_projActCache               mov  eax,ds:0x6bf464
call 0x16d990            ; GetRetAddr                   push 0xc
mov  ecx,ds:0x6bf654     ; m_errSink                    push eax
push 0xc                                                mov  ecx,0x6bf650   ; `this`, an IMMEDIATE
push esi                                                call <Report>
push 0x6bf650            ; this                         mov  ebp,ds:0x6bf664
mov  ds:0x6bf428,eax     ; breadcrumb
call 0x16d850            ; Set
mov  ebp,ds:0x6bf664
```

Our `zErrHandling::Report` is out-of-line in another TU, so cl cannot make the choice for us: the
source has to say which form each site uses. Hence the paired spellings

* `ActNameLookup` / `ActNameLookupCallReport` (`<Gruntz/ActNameRegistry.h>`)
* `zDArray<T>::ResolveEntry` / `zDArray<T>::ResolveEntryCallReport` (`<Gruntz/ActReg.h>`)

and, in a two-key registrar, the mechanical assignment

```cpp
i32 id = ActFindId("A");
if (id == 0) { ...; slot = ActNameLookupCallReport(g_typeCounter); ... }   // #1 outlined
*s_table.ResolveEntryCallReport(id)  = ActA;                               // #2 outlined
i32 id2 = ActFindId("B");
if (id2 == 0) { ...; slot = ActNameLookup(g_typeCounter); ... }            // #3 EXPANDED
*s_table.ResolveEntryCallReport(id2) = ActB;                               // #4 outlined
```

## Do not put a wrapper between the registrar and the accessor

`DroppedObject`/`StaticHazard` route their act-table lookups through a one-line TU-local wrapper
(`DropLookup(id)` -> `s_table.ResolveEntry(id)`). That extra inline level costs enough budget that cl
outlines **the whole accessor** (`push ebp; mov ecx,<&s_table>; call ResolveEntryCallReport`), which
is 8 basic blocks short of retail and holds the function at ~74.7%. Calling
`CActRegPool<T>::s_table.ResolveEntryCallReport(id)` directly from the registrar takes it to 100%.
(The wrapper is fine in the *dispatchers* - `FireActivation` etc. - which are already exact.)

## Result

Nine registrars 95.21% / 71.99% -> **100% EXACT**, plus `zDArray<CActHandler>::Resolve` (see
[`single-return-variable-pins-accessor-regalloc`](single-return-variable-pins-accessor-regalloc.md)).
Every one of them carried an `@early-stop` blaming "A/B inline asymmetry + register-pinning wall";
the asymmetry was real and observable in the disasm, but it is an inline-budget artifact with a
one-line source lever, not a wall.

related: act-registrar-counter-cse-and-freeloop.md, zero-register-pinning.md,
single-return-variable-pins-accessor-regalloc.md
