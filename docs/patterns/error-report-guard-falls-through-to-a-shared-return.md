# `if (!Ok()) { Report(); } return 1;` — not `if (Ok()) return 1; Report(); return 1;`

- **confidence** c9
- **tags** `cpp:branch` `cpp:switch` | `asm:jne` `asm:jmp` | `topic:codegen-idiom`

## Symptom

Every arm of a big command/state `switch` is TWO instructions longer than retail, and
the extra pair is always the same:

```
 base                          target (retail)
 calll <Try...>                calll <Try...>
 testl %eax, %eax              testl %eax, %eax
 je   <error>                  jne  <ONE shared `movl $1,%eax; jmp <ret>`>
 movl $0x1, %eax               pushl $<line>
 jmp  <ret>                    jmp  <shared Report tail>
 pushl $<line>
 jmp  <shared Report tail>
```

Retail's branch is **inverted** and its target is a single `mov eax,1; jmp ret` block
shared by the whole switch; ours materialises that return inline at every site.

## Cause

The source puts the *failure* path in the guarded block and lets the success path FALL
THROUGH to one trailing `return`:

```cpp
case CMD_MAIN_MENU:
    if (!TransitionState(GAMESTATE_MENU, 1, 0, 0)) {
        ReportError(IDX(IDS_SET_GAME_STATE), 0x426);
    }
    return 1;
```

Because the `return 1` is now the arm's single trailing statement, cl emits it once for
the whole function and every arm branches to it. Writing it as an early `return 1` on
success gives each arm its own copy - the two spellings are semantically identical and
only the count separates them. Two consecutive attempts are one `&&` of two negated
calls, not two sequential `if`s.

Measured on `CGruntzMgr::HandleCommand` 0x862f0 across fifteen arms, 94.69 -> 97.34
(delta +45 -> +11 with two other fixes).

**The mirror image also occurs**: when the guarded block itself ends in `return`, retail
keeps a full inline copy of the report + epilogue and does not share
(`CMD_RETURN_TO_ATTRACT`, `CMD_EXIT_TO_ATTRACT` at 0x3834/0x39df). So the shape of the
arm decides, and the retail branch POLARITY tells you which one it is.

related: [one-shared-return-tail-is-a-positive-gate-nest.md](one-shared-return-tail-is-a-positive-gate-nest.md),
[instruction-count-mismatch-finds-the-real-bug.md](instruction-count-mismatch-finds-the-real-bug.md)
