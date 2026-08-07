# `if (d < 0) d = 0;` on an `i64` HOMES the quad — a ternary that yields `i32` does not

tags: cpp:local cpp:int cpp:branch | asm:sub asm:sbb asm:mov | topic:codegen-idiom
symptoms: an 8-byte frame retail does not have (`sub esp,0x8` vs no `sub` at all),
  plus one dead `mov DWORD PTR [esp+N],0` that stores the HIGH half of a 64-bit
  value nothing reads; the low half is `xor reg,reg` on the same path
confidence: 8/10

A 64-bit elapsed-time delta that is clamped in place and then truncated:

```cpp
i64 diff = (i64)g_frameTime - t->m_startStamp.m_v;
if (diff < 0) {
    diff = 0;                       // <- assigns BOTH halves
}
hud->m_elapsedTimeMs += (i32)diff;  // <- reads only the low half
```

`diff = 0` writes the whole quad, so cl 5.0 gives `diff` a real 8-byte home and
emits `xor ecx,ecx` for the low half plus a `mov [esp+N],0` for the high half - a
store nothing reads, and 8 bytes of frame retail does not reserve. The 64-bit
comparison itself (`sub`/`sbb` + `jg`/`jl`/`jae`) is identical either way.

Spelling the clamp as a ternary whose TYPE is `i32` never defines the high half,
so the value stays in registers and the frame goes to zero:

```cpp
i64 diff = (i64)g_frameTime - t->m_startStamp.m_v;
hud->m_elapsedTimeMs += (diff < 0) ? 0 : (i32)diff;
```

STEERABLE. `CTriggerMgr::HitTestApply` 0x6ea00 91.28 -> 95.49 on the ternary alone
(`sub esp,0x8` -> retail's frameless prologue), then 98.80 with the second finding
in the same body: retail reads `world->m_frameMarker` TWICE - inline for the delta,
then into a local for the group of zero stores that follows.

Do NOT reach for this on an i64 that is genuinely 64-bit downstream: see
[i64-timer-pairs-are-faithful.md](i64-timer-pairs-are-faithful.md) - the lo/hi dword
pairs retail really does keep are faithful and must not be melded.

related: frame-size-counts-the-locals.md, i64-timer-pairs-are-faithful.md
