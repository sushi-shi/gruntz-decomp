# `switch` case BODIES are emitted in source-`case` order — reorder to retail layout
tags: cpp:switch | asm:jmp | topic:codegen-idiom
symptoms: structure matches but case-body blocks in wrong .text order, fuzzy jumps after reordering cases
confidence: 8/10

MSVC5 emits each `switch` case's body block in the order the `case` labels are WRITTEN in
source; the jump table just indexes into them. So if your reconstruction has the cases in a
different order than retail, the body blocks land in the wrong relative positions and the fuzzy%
plateaus below the reloc ceiling. Reorder the `case` labels in source to match the retail
body-layout order (read it from the target disasm) — this is steerable and can move a function
several points (e.g. 90→95%).

```cpp
switch (msg) { case WM_CREATE: …; case WM_CLOSE: …; /* in retail .text body order */ }
```
STEERABLE. Evidence: WAP32::CGameApp::GameWindowProc 90.1→94.83% after reordering window-lifecycle→kbd→cmd→mouse.
