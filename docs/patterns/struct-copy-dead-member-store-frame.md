# Missing `sub esp,N` frame + a dead spill of an UNUSED struct member — model a by-value struct copy
tags: cpp:struct cpp:local | asm:mov asm:sub | topic:codegen-idiom topic:regalloc
symptoms: sub esp,0xc, add esp,0xc, mov [esp+N],reg (a store nothing reads), mov eax,[p]; mov ecx,[p+4]; mov edx,[p+8], dead cell[2] spill
confidence: 9/10

Retail reserves a small frame (`sub esp,0xc` for a 3-int struct) and, after loading
all members of a struct, DEAD-STORES the one member the arithmetic never uses. This is a
BY-VALUE struct copy of a small POD where only some members are consumed: MSVC5 /O2 loads
all N members, forwards the used ones into registers (their stack stores DCE'd), but the
UNUSED member's load+store SURVIVES. A field-by-field / N-explicit-locals source DCEs the
unused member entirely — so the recompile omits `sub esp,N` and the whole epilogue +
downstream blocks shift, collapsing the fuzzy score by alignment desync.

```cpp
// WRONG: pointer or explicit locals -> unused `reason` is DCE'd, no frame reserved
i32* cell = m_entranceCell;
i32 base = 3 * cell[0] + cell[1];         // cell[2] never read -> gone

// RIGHT: by-value copy of the 3-int struct -> cell.reason's dead store survives
GruntEntranceCell cell = *(GruntEntranceCell*)m_entranceCell;   // {col,row,reason}
i32 base = 3 * cell.col + cell.row;        // reason unused, but loaded+dead-spilled
```
```asm
sub    esp,0xc
lea    eax,[esi+0x43c]
mov    eax,DWORD PTR [edx]        ; col
mov    ecx,DWORD PTR [edx+0x4]    ; row
mov    edx,DWORD PTR [edx+0x8]    ; reason  <- dead load
mov    DWORD PTR [esp+0x1c],edx   ; dead store nothing reads (the reserved frame)
```
STEERABLE. CGrunt entrance-cell arms: StartBombGruntRun 93.4→98.7, RearmAttackAnim2
73→95.8, RunEntranceMove 73→82, UpdateArrival/RunMoveConfig/StepCombatReaction/
StepEntranceReinit/RearmAttackAnim all +2..7. NOTE: only when the member is truly UNUSED
(dead) — if it is read (passed to a callee), there is no dead spill and the copy diverges
(reverted StepAnimDispatchA/B, which pass `reason` to the name accessor).
