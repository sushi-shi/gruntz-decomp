# A callee cl NEVER declines is under the cb <= 0x28 exemption — content that emits NOTHING lifts it out
tags: cpp:inline cpp:call cpp:assert msvc5:mfc | asm:call | topic:codegen-idiom topic:wall
symptoms: retail `call`s a tiny leaf helper but cl expands it at EVERY site no matter the caller; making it one in-class inline puts 100% of the sites inline, no base obj emits the COMDAT and the retail RVA leaves scoring; a caller that expands 1 of 3 identical sites; `((void)0)`; ASSERT; ASSERT_VALID
confidence: 9/10
variants: inline-budget-emits-ool-comdat.md, inline-callee-frontend-cost-drives-ob1-budget.md, two-shapes-need-two-entities.md

The /Ob1 budget exempts any callee whose front-end estimate is `cb <= 0x28` (40):
it is expanded at every visible site, in every caller, and no caller size ever
declines it — so it never emits an out-of-line COMDAT. `cb` is C1's estimate of
the SOURCE, not of the emitted code, so a statement the back end folds away still
pays for it. A release-build `ASSERT` is exactly that shape: MFC defines it as
`((void)0)`, C1 counts it, C2 emits nothing. That is how retail gets a leaf helper
that is both budget-limited AND byte-identical to the naive spelling.

```cpp
// cb <= 0x28: exempt. cl expands all 30 of 30 probe sites, no COMDAT, ever.
CObject* Find(const char* key) {
    CObject* found = 0;
    m_workers.Lookup(key, found);
    return found;
}
// cb ~44: budget-limited. IDENTICAL emitted bytes; 8 of 30 sites now decline.
CObject* Find(const char* key) {
    CObject* found = 0;
    ASSERT(key != NULL);            // release MFC: ((void)0)
    m_workers.Lookup(key, found);
    return found;
}
```
```asm
; both spellings emit retail's 0x9cab0 verbatim - the assert leaves no trace
9cab0: push ecx / mov edx,[esp+8] / lea eax,[esp+0] / push eax / push edx
9cabb: add ecx,0x10 / mov [esp+8],0 / call ?Lookup@CMapStringToOb@@
9cacb: mov eax,[esp+0] / pop ecx / ret 4
```

STEERABLE. Titrated on the pinned cl 5.0 with 30 sites in a minimal caller
(`build/inline-model/`, same method as `tools/inline-budget/`): baseline 0
rejections; `((void)0)` x2 -> 8; one unused local -> 8; a dead branch -> 13; a
redundant `if (!Lookup(...)) f = 0;` -> 11 but it emits a `test eax,eax` retail
does not have, so the zero-emission forms are the only byte-safe ones. In-tree:
one `ASSERT` in `CDDrawWorkerCache::Find` made `??0CWayPoint` / `??0CGuardPoint` /
`??0CLevelTime` go 0.00 -> 99.67 each and put 0x9cab0 at 100% emitted by
`waypoint.obj` - i.e. by a caller that DECLINED it - with no per-TU visibility
header at all. Diagnostic value first: an unexplained `call` to a helper cl always
inlines is evidence the original had compiled-out content, so measure `cb` before
concluding the split was per-TU visibility. The specific text is an INFERENCE -
the mechanism is proven, the assert's wording is not recoverable - so comment it
as such and never tune the count to chase a score.
