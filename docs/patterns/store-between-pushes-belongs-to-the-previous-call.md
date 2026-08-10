# A member store sandwiched between the NEXT call's pushes holds the PREVIOUS call's result
tags: cpp:member cpp:local cpp:method | asm:mov asm:push asm:call | topic:codegen-idiom topic:identity
symptoms: mov [this+N],eax between two push; getvaluedword; settings; back-to-back calls; result dropped into a local; unused variable
confidence: 10/10

cl 5.0 /O2 hoists a store off `eax` into the argument-setup of the *following* call, so the
store reads as if it belonged to that call. Read it by DATA FLOW, not by position: `eax` at
that point is still the return value of the call ABOVE. Getting it backwards silently moves
a value onto the wrong member and drops the right one — a behaviour bug no percentage can
see, because the byte sequence is the same either way.

```asm
    mov  eax,[ebp+0x118]        ; default arg = the CURRENT value of this member
    push eax
    push <"Easy Mode">
    call GetValueDword
    mov  ecx,[ebp+0x38]
    push esi                    ; <- the NEXT call's args are already going down
    push <"Resolution">
    mov  [ebp+0x118],eax        ; <- STILL the "Easy Mode" result, not "Resolution"
    call GetValueDword
    cmp  eax,0x3                ; "Resolution" is consumed HERE, stored nowhere
```
```cpp
// what to WRITE - direct member assignment, one statement:
m_isEasyMode = m_settings->GetValueDword("Easy Mode", m_isEasyMode);
i32 resolutionRaw = m_settings->GetValueDword("Resolution", IDX(RES_640X480));
```

**The corroborating oracle is the DEFAULT ARGUMENT.** A settings/config probe passes the
member's current value as its own default, so `push [this+N]` before the call and
`mov [this+N],eax` after it are the SAME member; if your source pairs a probe's default
with one member and its result with another, one of the two is wrong. Second oracle: a
local that is computed and never read (`clang -Wunused-variable` over the clangd compile
DB finds them tree-wide, 22 rows across 339 TUs) — retail stored that value somewhere.

Steerable, and it was a real defect: `CGruntzMgr::Run` had `m_isEasyMode = resolutionRaw`,
so `m_isEasyMode` took the resolution index (never 0) and the game ran permanently in Easy
Mode — secret teleporter triggers self-destructed, static hazards never pulsed, KitchenSlime
never grabbed, the water/death bridge tiles were downgraded at load. Run 82.96 -> 83.17 and
the whole registry block became byte-identical including this store's position.
