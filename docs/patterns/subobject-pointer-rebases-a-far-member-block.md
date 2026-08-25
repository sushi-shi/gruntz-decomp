# `add reg,0xNNN` before a run of disp8 stores = the block is a SUB-OBJECT reached by pointer

tags: cpp:member cpp:struct cpp:local | asm:add asm:lea asm:mov | topic:codegen-idiom topic:identity
symptoms: retail spends one extra `add eax,0xNNN` and then uses `[eax]`/`[eax+0x4]`/`[eax+0xc]`
where you emit `[eax+0x290]`/`[eax+0x294]`/`[eax+0x29c]`; retail's block is a few bytes
SHORTER than yours (`gruntz walls diagnose <rva>` shows the byte-length delta); the FIRST
store of the block still uses the full disp32 on both sides
confidence: 9/10
variants: member-aggregate-copied-not-field-by-field.md, i64-zero-store-batching-reveals-subobjects.md

Writing several far members of one object (`h->m_timerWindow = …; h->m_timerBase = …;`)
gives every store a disp32. Retail instead materializes the block's base ONCE and reaches
the rest with disp8 — a size win cl will NOT take on its own from flat member accesses.
The construct that produces it is a **pointer to a named sub-object**: cl folds the
sub-object's offset into the FIRST access's displacement and materializes the pointer only
when the second access would need it, which is exactly retail's `store; add; store; store;
store` shape.

```cpp
// before - four disp32 stores, no rebase
CTriggerMgr* h2 = g_gameReg->m_triggerMgr;
h2->m_timerWindow = 0x3e8;              // +0x298 / +0x29c
h2->m_timerBase   = (u32)g_frameTime;   // +0x290 / +0x294

// after - the {base,window} pair is a real sub-object, reached by pointer
struct CueTimer { i64 m_base, m_window; };   // in the owner's header, at +0x290
CueTimer* tm = &g_gameReg->m_triggerMgr->m_cueTimer;
tm->m_window = 0x3e8;
tm->m_base   = (u32)g_frameTime;
```

```asm
base:   mov [eax+0x298],0x3e8 | mov [eax+0x29c],ecx | mov [eax+0x290],edx | mov [eax+0x294],ecx
target: mov [eax+0x298],0x3e8 | add eax,0x290 | mov [eax+0xc],ecx | mov [eax],edx | mov [eax+0x4],ecx
```

STEERABLE, and it is an IDENTITY finding as much as a codegen one: the rebase names the
struct boundary, so keep the old flat names alive as an anonymous-struct union arm and the
other TUs that use them need no edit. Evidence (2026-07-28, `src/Gruntz/Warlord.cpp` +
`include/Gruntz/TriggerMgr.h`): `CWarlord::FinishJoyAnimation` 95.85 -> **100 EXACT** and
`CWarlord::BuildFortSplashParticles` 98.07 -> **100 EXACT**, both filed as an
"addressing-mode wall ... nothing in the source selects it".
