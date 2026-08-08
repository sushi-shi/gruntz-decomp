# `push A / jmp J / push B / J: <shared call>` is TWO calls cross-jumped, not a selected argument
tags: cpp:branch cpp:call cpp:ternary | asm:push asm:jmp asm:cmp | topic:codegen-idiom
symptoms: retail branches around a single `push <imm>` and falls into a shared argument tail + `call`; the recompile computes the value into a register first — either assign-then-override (`mov eax,B / cmp / jne / mov eax,A`) or, when the two constants are close, branchless `neg/sbb/and imm/add imm`
confidence: 9/10

When two calls differ in exactly one argument, cl 5.0 **cross-jumps** them: the
common suffix (the remaining pushes and the `call`) is emitted once and each arm
keeps only its own `push`. The result looks like a selected argument but is not —
no register ever holds the choice.

```asm
mov    eax,[ebp+0x24]
push   0x0                 ; the arguments the two calls SHARE
cmp    eax,0x5
jne    L
push   0x8023              ; arm A's only instruction
jmp    J
L:
push   0x8027              ; arm B's only instruction
J:
mov    eax,[ebp+0x4]       ; ... rest of the shared tail
push   0x111
mov    ecx,[eax+0x4]
push   ecx
call   [PostMessageA]
```

```cpp
// WRONG - a value select. As a local it is assign-then-override
// (`mov eax,0x8027 / cmp / jne / mov eax,0x8023 / push eax`); as a ternary the
// two constants differ by 4 so cl if-converts it to
// `neg/sbb eax,eax/and eax,4/add eax,0x8023` - further away still.
u32 wp = IDX(CMD_ATTRACT);
if (m_previousStateId == GAMESTATE_MENU) { wp = IDX(CMD_MAIN_MENU); }
PostMessageA(hwnd, WM_COMMAND, wp, 0);

// RIGHT - two calls; cl merges the tail itself
if (m_previousStateId == GAMESTATE_MENU) {
    PostMessageA(hwnd, WM_COMMAND, IDX(CMD_MAIN_MENU), 0);
} else {
    PostMessageA(hwnd, WM_COMMAND, IDX(CMD_ATTRACT), 0);
}
```

Reading the arms: the branch that is TAKEN names the second-written arm, so
`cmp k,5 / jne L / push A` is `if (k == 5) call(A); else call(B);`.

Distinguish from [default-then-override-flag](default-then-override-flag.md) and
[default-hoists-into-destination-no-jmp](default-hoists-into-destination-no-jmp.md):
those apply when the selected value is a **variable** that outlives the branch.
Here the value dies inside the call, and the tell is that the differing
instruction is the `push` itself.

`CCreditsState::Render` 0x0391d0 96.65 -> 100.00 EXACT (together with a
devirtualisation fix in the same function).
