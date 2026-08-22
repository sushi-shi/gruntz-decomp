# A member store retail makes BEFORE a call it logically follows is the wrapper's, not a hand expansion

tags: cpp:inline cpp:call cpp:class | asm:mov asm:call | topic:codegen-idiom
symptoms: retail stores a member from a field the guard just proved NULL, RE-READ
through the callee's own receiver, and does it BEFORE the call; ours calls first and
stores after, from the guard's copy-propagated zero register. `walls diagnose` reports
REGALLOC or CFG with matching call and branch counts
confidence: 9/10 (CFrontCandyAni::CFrontCandyAni 0xacf40 91.19 -> 100.00 EXACT,
CBehindCandyAni::CBehindCandyAni 0xad540 94.54 -> 97.64)

`CUserLogic::SwitchGeometry` is

```cpp
i32 SwitchGeometry(const char* key, i32 flag) {
    m_value = m_wwdObject->m_animCursor.m_animation;
    return m_wwdObject->ApplyLookupGeometry(key, flag);
}
```

so the save is the FIRST STATEMENT OF THE INLINE BODY. cl 5.0 binds an inline's
arguments before substituting its body, so the save lands after the argument setup
and before the call, and the read goes through the receiver the call is about to use
rather than through a value the caller already has in a register.

```cpp
// ours - reads the guard's zero, stores after the call
if (m_wwdObject->m_animCursor.m_animation == NULL) {
    m_wwdObject->ApplyLookupGeometry("GAME_CYCLE100", 0);
    m_value = m_wwdObject->m_animCursor.m_animation;
}

// retail
if (m_wwdObject->m_animCursor.m_animation == NULL) {
    SwitchGeometry("GAME_CYCLE100", 0);
}
```

This is why hand-reordering the two statements does not reach it: as CALLER
statements either order costs an extra use of the zero constant, which claims a
fourth callee-saved register and shifts every frame offset. The wrapper is a
different C1 construct, not a different statement order — the same distinction as
[out-param-reset-between-arg-setup-and-call-is-in-the-helper](out-param-reset-between-arg-setup-and-call-is-in-the-helper.md).

Detection: a `Apply*`/`Setup` call whose logically-following member store retail
emits BEFORE it. Sieve the tree for hand expansions with
`rg -A1 'ApplyLookupGeometry\(' src | rg 'm_value ='`.
