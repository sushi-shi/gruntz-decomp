# Re-read a member-view pointer per access — don't cache it in a local
tags: cpp:local | asm:mov | topic:codegen-idiom topic:regalloc
symptoms: target re-loads `mov reg,[this+N]` before every sub-access of the same member-pointer chain; your `T* v = this->m_N;` cached-local recompile keeps it pinned in one callee-saved reg and pushes an extra reg, dropping a teardown/walk to ~70–80%
confidence: 8/10

When a method walks the same member pointer (`this->m_c`) several times — e.g. a
resource-teardown that calls `this->m_c->m_10->Release(...)`, `this->m_c->m_28->Release(...)`,
`this->m_c->m_c->Dispose()` — MSVC5 /O2 re-emits `mov reg,[this+0xc]` fresh before EACH access
rather than caching it. A cached source local (`CView* v = m_c; v->m_10->...; v->m_28->...;`)
forces cl to pin `m_c` in a callee-saved reg across the whole body (an extra `push edi`/`push esi`)
— a different register schedule that diverges. Re-deref the member in each statement instead.

```cpp
// MATCHES — re-read m_c each statement (no cached local):
((CView*)m_c)->m_10->Release("MENU", "_");
((CView*)m_c)->m_28->Release("MENU", "_");
((CView*)m_c)->m_c->Dispose();
// EXCEPTION: when ONE block reuses the same loaded value (test + use), cache THAT block's
// leaf only: `CRes* r = ((CView*)m_c)->m_28->m_2c; if (r) r->Free();` (retail holds it in ecx).
```
```asm
mov  eax,[esi+0xc]    ; m_c re-read
mov  ecx,[eax+0x10]   ; ->m_10
call Release
mov  ecx,[esi+0xc]    ; m_c re-read AGAIN (not cached in a callee-saved reg)
mov  ecx,[ecx+0x28]   ; ->m_28
call Release
```
STEERABLE. Evidence: CBootyState::ReleaseResources 79.9→98.4% (code byte-identical) and
CMenuState::ReleaseResources 79.4→99.5% by dropping the `CStateResView* v = m_c;` cache; the
m_1b4-delete block in the same method is the inverse (cache the leaf — retail pins it in edi).
