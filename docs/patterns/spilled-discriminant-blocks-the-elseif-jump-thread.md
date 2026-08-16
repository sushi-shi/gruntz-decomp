# A spilled discriminant BLOCKS the `else if` jump-thread: the reload lands in the shared predecessor

tags: cpp:if cpp:branch cpp:local | asm:jcc asm:mov asm:test | topic:wall topic:regalloc topic:codegen-idiom
symptoms: `gruntz walls diagnose` reports equal branch counts and ONE `TOPOLOGY` row - one `jcc`
lands one block later in retail than in the recompile - while `--diff` and `gruntz walls diagnose --asm`
read identical instruction for instruction
confidence: 8/10 (CDDrawShadeBlit::BlitCopyMirrored 0x149d00, 55/55 branches, one topology row)
variants: masked-diff-hides-branch-target.md, over-merge-is-decided-before-layout.md

A three-arm dispatch whose first arm is an `&&`:

```cpp
if (clip->left == 0 && clip->right == m_width - 1) {   // arm 1
} else if (clip->left != 0) {                          // arm 2
} else if (m_width - 1 != clip->right) {               // arm 3
}
```

lowers to two exits from the `&&`. On the `clip->left != 0` exit the second `if` is already
known true, so cl can jump PAST its re-test straight into arm 2; on the
`clip->right != m_width - 1` exit it cannot, and must re-test. Retail threads exactly that
one edge:

```asm
; retail 0x149dc7 - clip is LIVE in edx
    jne  0x149ecb              ; left != 0  -> arm 2 BODY, past the re-test
    ...
    jne  0x149ec3              ; right != w-1
0x149ec3: test eax,eax         ; the re-test, reached only from the right-check
          je   arm3
0x149ecb: mov  eax,[edx+0xc]   ; arm 2 body - edx is still clip
```

The recompile threads NEITHER edge, because its `clip` is memory-homed: arm 3 needs it in a
register, cl hoists that reload into the block both edges pass through, and a target block
that now begins with an instruction cannot be entered one instruction later.

```asm
; base - clip lives at [esp+0x28]
    jne  0x1d3
0x1d3:    mov  ecx,[esp+0x28]  ; arm 3's reload, hoisted into the shared predecessor
0x1d7:    test eax,eax         ; <- the right-check edge lands here
          je   arm3
0x1df:    mov  eax,[esp+0x28]  ; arm 2 body reloads it AGAIN
```

WALL, and the lever is not the branch: the topology row is a CONSEQUENCE of the register
homing, so no `if`/`else if`/nesting/`goto` respelling of the dispatch reaches it - fix which
value owns the callee-saved register, or leave it. `BlitCopyMirrored` is flat at 79.48 across
106 measured TU states and every arm-chain spelling; read the `TOPOLOGY` row as a homing
report, not as a control-flow bug.
