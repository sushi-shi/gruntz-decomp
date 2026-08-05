# A condition that must survive a destructor call keeps its STATIC TYPE — `!= 0` costs four instructions

tags: cpp:branch cpp:eh cpp:local | asm:neg asm:sbb asm:test | topic:codegen-idiom
symptoms: a block of `mov ebx,eax / neg ebx / sbb ebx,ebx / neg ebx` right after a call, then a
  destructor call, then `test bl,bl` — where retail has `mov ebx,eax`, the destructor call, and
  `test ebx,ebx`; repeats once per site in a function that builds CString temporaries in its
  conditions
confidence: 9/10

When a temporary's destructor runs between a call and the branch that tests its result,
cl has to park the condition in a callee-saved register across the dtor call. **What it
parks depends on the static TYPE of the condition expression, not on how it is used.**

```cpp
// NO - `x != 0` is a bool, so cl normalises to 0/1 before spilling, then tests the BYTE
if (m_world->m_imageRegistry->HasKeyEqual("GRUNTZ_" + name) != 0) { … }
//   mov ebx,eax / neg ebx / sbb ebx,ebx / neg ebx   … call <~CString> …   test bl,bl

// YES - a bare int condition is parked raw and tested at full width
if (m_world->m_imageRegistry->HasKeyEqual("GRUNTZ_" + name)) { … }
//   mov ebx,eax                                     … call <~CString> …   test ebx,ebx
```

Four instructions per site. `CState::BuildAssetNamespacePrefixes` @0xdca70 had three of
them (95.57 → 100 EXACT on this change alone). The `== 0` polarity of the same idiom
(`neg/sbb/inc`) behaves the same way.

The lever only exists because of the intervening call. With no temporary to destroy, cl
consumes the flags in place and both spellings are byte-identical — which is why this
reads as a "regalloc/EH wall" until you notice the four instructions are a TYPE
conversion, not a spill.

related: int-to-bool-normalize.md, strcmp-eq-bool-local-setcc.md, seh-bool-return-canonicalize.md
