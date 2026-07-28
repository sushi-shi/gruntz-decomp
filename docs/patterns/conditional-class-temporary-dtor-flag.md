# A destructor-FLAG word (`test bl,1` / `test bl,2`) means a CONDITIONAL class TEMPORARY, not two scoped locals

tags: cpp:ternary cpp:eh cpp:ctor mfc:cstring | asm:test asm:and | topic:codegen-idiom topic:eh
symptoms: `mov ebx,1` in one arm and `mov ebx,2` in the other, a matching
`mov [esp+N],ebx` shadow store, then AFTER the shared call
`test bl,0x2 / je / and ebx,0xfffffffd / <dtor>` followed by `test bl,0x1 / je / <dtor>`;
an unexplained `sub esp,8`; two same-typed temporaries at adjacent slots
confidence: 10/10

MSVC5 lowers `f(cond ? A() : B())` — where `A()`/`B()` return a class by value — as
TWO temporaries with a **destructor-flag word**: each arm ORs in its own bit (and
mirrors the word into a frame slot for the /GX funclet), the call runs at the merge,
and both destructors run afterwards, each gated on its bit. That flag word plus its
EH shadow is the "unexplained" 8 extra frame bytes.

Splitting the ternary into two scoped locals is NOT equivalent: each destroys inside
its own arm, no flag exists, the frame is 8 bytes smaller, and every esp-relative
offset below shifts — which reads as a whole-function EH/frame-layout wall.

```cpp
// before - two per-branch scoped locals: no flag, no shared call
if (m_5b0 != 0) { CString b = GetConfigNameB(); token = Build(0,0,m_5b0,0, b); }
else            { CString a = GetConfigNameA(); token = Build(0,0,m_5b0,0, a); }

// after - ONE call, conditional temporary
i32 token = Build(0, 0, m_5b0, 0, m_5b0 != 0 ? GetConfigNameB() : GetConfigNameA());
```

```asm
    ; arm B                         ; arm A
    call GetConfigNameB             call GetConfigNameA
    mov  ebx,0x1                    mov  ebx,0x2
    mov  [esp+0x10],ebx             mov  [esp+0x10],ebx
    ; merge: copy-construct the by-value arg, call, then flagged teardown
    test bl,0x2 | je L1 | and ebx,0xfffffffd | lea ecx,[esp+0x28] | call ??1CString
L1: test bl,0x1 | je L2 | lea ecx,[esp+0x2c]                     | call ??1CString
```

STEERABLE, and it dominates everything downstream. Evidence (2026-07-28):
`CMulti::VerifyCustomLevel` @0x0b8fc0 23.46 → 85.64 on the ternary alone, then
→ **100 EXACT** once the same call's `m_5b0` moved from the `hi` parameter to `lo`
(the surrounding zeros hid a genuinely wrong argument position — see
[compensating-error-signatures](compensating-error-signatures.md)) and the m_530 gate
was re-spelled per
[gate-falls-through-to-shared-latch](gate-falls-through-to-shared-latch.md). It had
been filed a "/GX CString-by-value EH-frame-layout wall … not source-steerable".
