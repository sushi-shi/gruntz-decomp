# `void`-modeled function won't split epilogues — declare the real `int`/BOOL return
tags: cpp:branch cpp:return | asm:ret asm:mov | topic:codegen-idiom
symptoms: retail emits a SEPARATE inline `pop;ret N` at each early-out plus a `mov eax,1`
  before the success `pop;ret N`; your `void` recompile tail-merges all the identical
  bare `pop;ret N` epilogues into one shared tail and never emits `mov eax,1`, ~82-85%
confidence: 8/10

A guard-heavy registry/loader written `void` plateaus in the low-80s because retail is
actually `int` (BOOL): every guard is `return 0` and the fall-through is `return 1`. With
`void`, all returns are the identical bare `pop esi; ret N`, so cl tail-merges them into one
shared tail and emits no success constant — diverging both the jump displacements and the
tail. Declaring the real `int` return reproduces retail exactly: each guard stays an inline
per-site `pop esi; ret N` (eax already holds the just-tested 0, so NO normalizing
`mov eax,0`/`xor`), and the success path gets `mov eax,1` before its own `pop esi; ret N`.

```cpp
int Loader::Load(int force) {
    if (!self->m_c) return 0;                          // eax==m_c==0 already; bare pop;ret
    if (!force && self->m_c->reg->Has("LEVEL")) return 1;  // jne -> the mov eax,1 tail
    self->m_c->reg->Register("LEVEL", "_");
    g_counter = 0;
    void* s = self->m_28->Lookup("IMAGEZ");
    if (!s) return 0;                                  // eax==Lookup result==0; bare pop;ret
    self->m_c->reg->Install(s, "LEVEL", "_");
    return 1;                                          // mov eax,1; pop;ret
}
```
```asm
test eax,eax / jne +4 / pop esi / ret 4        ; guard: inline epilogue, no mov eax,0
...
mov eax,1 / pop esi / ret 4                     ; success tail
```
STEERABLE. Note the symbol's *mangled* return letter is irrelevant to pairing — Ghidra
demangles `...QAEXH@Z` (void) from the relocs alone, but the RVA-keyed delinker still pairs
the `int`-return body (`...QAEHH@Z`). Evidence: `EngineThisStub::LoadLevelImages` @0xdb7e0
and `LoadActionTileSprites` @0xdb600 (engine_label_stubs) both `void`-plateaued ~82/85% →
100% on the `int`-return rewrite. Inverse subcase of
[identical-return-epilogue-tailmerge](identical-return-epilogue-tailmerge.md): that wall is
the all-`return 0` (no distinct success constant) case where the split is unsteerable; this
is the mixed `return 0`/`return 1` case where the BOOL return IS the steer.
