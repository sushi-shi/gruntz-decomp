# A one-shot registrar inlines (unrolled) in some leaf TUs but is an out-of-line call in others — make the body visible per-TU
tags: cpp:ctor cpp:inline | asm:call asm:jne | topic:codegen-idiom
symptoms: a shared helper the leaf ctors fold (e.g. BuildLogicTypeTable) appears as a `call <helper>` (compact) in most leaves but as the FULL inlined block (a 3-way Find/Register loop) in a few; same RVA family, different prologue size

A small one-shot helper that every leaf's folded base ctor calls (here the built-in
logic-type registrar `CUserLogic::BuildLogicTypeTable`) was compiled TWO ways across
the retail TUs: most leaves emit a `call` to the out-of-line copy (the **compact**
prologue), but the leaves whose TU had the helper's BODY in scope let MSVC5 /O2 INLINE
it (the **unrolled** prologue — the registrar's whole `if(!reg->Find(key)) reg->Register(...)`
×3 block expands into the ctor). The two are byte-distinct; pick per-leaf by counting the
registrar's key-string references in the target (compact = each key once for the later
AddLogic calls; unrolled = each key 3× — Find + Register + AddLogic — plus the inlined
`Find`/`call [vtbl+0x24]` pair).

To reproduce the UNROLLED form, define the helper `inline` in a header the unrolled TUs
include BEFORE the leaf ctor (model the registry the inlined body walks — `Find` is the
registry's own thunk-called method, `Register` its vtable slot); the compact TUs just
leave the helper DECLARED (out-of-line `call`). MSVC inlines it even with virtual calls
inside, as long as the leaf body stays small (a big tail makes /O2 keep the `call` — then
it's compact again, re-classify).

```cpp
// include/Gruntz/LogicTypeTableInline.h — included by the UNROLLED leaf TUs only:
inline void CUserLogic::BuildLogicTypeTable(CLogicTypeBuilder* ctx) {
    if (!ctx->m_0c->m_14->Find("LogicHit"))   ctx->m_0c->m_14->RegisterType((void*)LogicHitFactory,   "LogicHit",   2);
    if (!ctx->m_0c->m_14->Find("LogicAttack"))ctx->m_0c->m_14->RegisterType((void*)LogicAttackFactory,"LogicAttack",2);
    if (!ctx->m_0c->m_14->Find("LogicBump"))  ctx->m_0c->m_14->RegisterType((void*)LogicBumpFactory,  "LogicBump",  2);
}
```
```asm
; unrolled: per key  push key; mov ecx,[reg]; call Find; test eax,eax; jne skip; ... call [edx+0x24]
; compact:  cmp [g_flag],0; jne; mov ecx,this; push ctx; call BuildLogicTypeTable; mov [g_flag],1
```
STEERABLE (per-TU body visibility flips compact↔unrolled). Evidence: CWayPoint/CGuardPoint/
CLevelTime/CLightFx ctors → unrolled 96% (the eh-ctor-vptr-restamp wall ceiling); CDoNothing
mis-included the header → 22% (it is COMPACT — `call 0x39c2` present), reverting the include → 90%.
