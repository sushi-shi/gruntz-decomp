# Non-EH leaf ctor: a hand-rolled vtable stored LAST (after the member stores) must stay a manual `m_vtbl` field — virtualizing forces the implicit vptr stamp FIRST and regresses
tags: cpp:ctor cpp:virtual cpp:member | asm:mov | topic:wall topic:codegen-idiom
symptoms: retail leaf ctor ends with `mov [this],&vtbl` AFTER m_4/m_8/m_c stores; adding `virtual ~T()` moves the store to the ctor entry (before the member stores) and drops the ctor ~100%→~83%; WAP-engine `z*` container node (zPTree/zDArray family), non-RTTI (code-ref-weak) vtable
confidence: 9/10

The WAP engine's `z*` container classes (zPTree, zDArray, keyed-store nodes) use HAND-ROLLED
vtables: the ctor writes the vtable pointer as a plain data-field store (`m_vtbl = &g_someVtbl;`),
and MSVC5 /O2 schedules that store **LAST**, after the member-field stores. Example — `CButeNodeEntry`
ctor (0x16df70): `[eax+4]=desc; [eax+8]=(word)n; [eax+0xc]=0; [eax]=&g_buteNodeEntryVtbl` (vtbl store
last).

If you "upgrade" such a class to a real C++ polymorphic class (`virtual ~T()`, drop the manual
store), `cl` emits its OWN implicit vptr stamp at the **ctor entry**, i.e. FIRST — right after the
`mov eax,ecx` arg setup, BEFORE the member stores. That single store cannot be sunk (it is
compiler-controlled, not source-steerable), so the whole store schedule diverges and the ctor
regresses. Proven by `llvm-objdump -dr` base-vs-target on `??0CButeNodeEntry@@QAE@HPAX@Z`:

    base (virtual ~T):  movw dx,[esp+4]; mov eax,ecx; mov ecx,[esp+8]; mov [eax],&??_7  <-- FIRST
                        mov [eax+4],ecx; mov [eax+8],dx; mov [eax+0xc],0; ret 8   → 100%→82.9%
    retail (manual):    movw dx,[esp+4]; mov eax,ecx; mov ecx,[esp+8]; mov [eax+4],ecx;
                        mov [eax+8],dx; mov [eax+0xc],0; mov [eax],&vtbl; ret 8   <-- LAST → 100%

So: **keep the explicit `void* m_vtbl;` @+0 field + the `m_vtbl = &g_someVtbl;` store** (steerable to
last) for these classes; do NOT declare virtuals. The vtable itself stays a reloc-masked `extern`
(named in `config/vtable_names.csv` if needed for scoring). The `z*` "RTTI" a vtable scan reports is
the engine's own manual type descriptor, not MSVC `/GR` RTTI — these are not compiler-emitted vtables.

Distinct from the companions:
- `explicit-mvptr-no-virtuals.md` — same "keep the m_vtbl field, don't virtualize" conclusion, but
  its mechanism is the +4 OFFSET SHIFT (the implicit vptr pushes every field down). Here the vptr is
  already at +0 (no shift); the wall is purely the store POSITION (first vs last).
- `eh-dtor-implicit-vptr-stamp-first.md` — the CONVERSE for /GX **destructors**, where the
  implicit-stamp-FIRST is exactly what retail wants (94→100). For **ctors** that store the vptr last,
  implicit-first HURTS.
- `eh-ctor-vptr-restamp-position.md` — the /GX-ctor sibling (vptr ~4 instrs late, ~94-95%).

Same family, same wall on the multiply-derived ctor (0x16dff0, `??0CButeNodeBase`): retail stamps the
secondary vptr @+8, then the m_18/m_28 member zeros, then the primary vptr @+0 — member stores
INTERLEAVED between the two vptr stamps, which the MI auto-vptr (both stamps grouped after the base
ctors) cannot reproduce either.
