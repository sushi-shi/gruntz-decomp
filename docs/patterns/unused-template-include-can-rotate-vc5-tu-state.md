# An unused template include can rotate VC5 translation-unit state

tags: cpp:include cpp:template cpp:inline | topic:tu-state topic:codegen-idiom

During the #79/main integration, `ButeMgr.h` included `ZTools/ZDArray.h`
although it declares and uses no `zDArray` or `_zdvec` entity. The needed
`zSymTab` owner is supplied by `ZTools/PTree.h`. The dynamic-array include
was removed as a real dependency correction, not as an inert steering probe.

The controlled full-build A/B kept every Bute function body and template
definition unchanged. With the extra include, `CButeMgr::SetInt` at 0x171b80
was 59.3753%: its first differing switch arm omitted a retail cleanup call,
and the pair had 43 versus 30 calls. Removing only the unused include restored
`SetInt`, `SetDouble`, `SetDword`, `SetFloat`, and `SetPoint` to strict 100%; the
overall current result rose from 3,812 exact / 93.67% fuzzy to 3,819 exact /
93.78% fuzzy. The final full build passed the MAX and fast/normal gates after
banking the independently reviewed template-source changes.

This signature is broad codegen movement after a header-dependency change,
not evidence that the affected functions need rewritten logic. First prove
the include is unused by the header's declarations and that a direct required
owner remains, then run a whole-build A/B and inspect the first real retail
divergence. Do not add or keep an unused declaration solely to steer C1.
