# Split the TU when one method needs `/GX` but a sibling's CString-return must stay frameless
tags: cpp:eh cpp:dtor cpp:local | asm:push asm:mov | topic:flags topic:codegen-idiom
symptoms: under `flags="eh"` the destructor is 100% but a `CString x = obj->GetKey();`-style sibling gains a `push -1/fs:0` frame the target lacks (~47%); under `flags="base"` the sibling is frameless+higher but the destructor loses its frame (~44%)
confidence: 8/10
variants: gx-frame-destructible-local.md

A single class TU can hold a method that REQUIRES the `/GX` frame (a destructor whose
member CStrings/CPtrList are unwound) AND a method whose CString temp the retail compiler
proved frameless (a by-value `GetKey()` returned into a named local, destroyed by `~CString`
with no intervening throw). MSVC5 under one `flags` setting cannot satisfy both: `eh` frames
the frameless sibling (extra `push -1/fs:0` prologue → big mismatch), `base` un-frames the
destructor. Retail emitted them in ONE TU because its /GX analysis elided the sibling's frame;
MSVC5's /GX is more conservative and always frames the CString local. Fix: split the
destructor (and any other genuinely-EH method) into its own `flags="eh"` TU, keep the
frameless methods under `flags="base"`. The class header is shared; the dtor TU just includes
it and defines `~Class`. Each function is RVA-keyed, so the split is matching-neutral.

```toml
[[unit]]                       # the frameless methods
unit = "chatbox"
source = "src/Gruntz/ChatBox.cpp"
flags = "base"
[[unit]]                       # ONLY the /GX-needing destructor
unit = "chatbox_eh"
source = "src/Gruntz/ChatBoxDtor.cpp"
flags = "eh"
```
STEERABLE (TU split). Evidence: CChatBox — `~CChatBox` 44%→100% in the eh split while `Find`
(frameless `CString key = node->GetKey()` + inline strcmp) stayed frameless under base; both
green/banked in one wave.
