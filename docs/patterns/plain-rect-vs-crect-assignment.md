# Direct four-word rectangle copies distinguish RECT from CRect assignment

tags: cpp:global cpp:struct msvc5:mfc | topic:data topic:codegen-idiom topic:correctness

Symptom: a function receives `const RECT*` and copies four consecutive words to
a 16-byte global. The instruction shape can look like an inlined MFC `CRect`
assignment, but VC5 does not emit that shape for either CRect spelling.

A controlled VC5 `/O2 /MT` probe against the shipped MFC headers gives three
distinct results:

```cpp
CRect mfcRect;
RECT plainRect;

mfcRect = *source;   // calls imported CopyRect
mfcRect = source;    // builds a temporary with CopyRect, then copies it
plainRect = *source; // four direct loads and four direct stores
```

`CImage::RenderFrameClipped` has the third sequence exactly, with no call and no
temporary, so the global at `0x002bf28c` is modelled as `RECT`, not `CRect` and
not four unrelated scalar globals. Reverse-use rule: when MFC is in scope, do
not infer CRect from field names or size; compile the exact overload expression
because its Win32 helper call is a strong type discriminator.
