# i64 zero-stores batch lo-halves-then-hi-halves per SUB-OBJECT — the batch boundaries name the real structs

**Tags:** cpp:ctor cpp:member cpp:int | asm:mov | topic:codegen-idiom topic:identity

## Symptom

A ctor zeroing four `__int64` members emits eight stores, and the retail order
is *not* the order any flat spelling produces:

```
retail:  [+0x108] [+0x110] [+0x10c] [+0x114]   [+0x120] [+0x128] [+0x124] [+0x12c]
base:    [+0x108] [+0x110] [+0x120] [+0x128]   [+0x10c] [+0x114] [+0x124] [+0x12c]
```

Both sides batch all the low halves before all the high halves — but retail
batches them **twice, in groups of two i64s**, where the flat spelling batches
all four at once.

## Read it as structure, not as scheduling

The batch is per *initialisation region*. Four loose members assigned in a ctor
body are one region; two sub-objects with their own default constructors are
two. So the boundary in the bytes tells you the retail class had a nested
struct:

```cpp
// four loose members -> ONE batch of four (does not match)
i64 m_legDeadline, m_legWindow;  /* +0x118 gate, pad */  i64 m_strikeDeadline, m_strikeWindow;
CPathHazard::CPathHazard() { m_legDeadline = 0; m_legWindow = 0; m_strikeDeadline = 0; m_strikeWindow = 0; }

// two sub-objects -> TWO batches of two (matches retail exactly)
struct CHazardTimer {
    i64 m_deadline, m_window;
    CHazardTimer() : m_deadline(0), m_window(0) {}
};
CHazardTimer m_leg;      // +0x108
i32 m_strikeArmed;       // +0x118
char m_pad11c[4];
CHazardTimer m_strike;   // +0x120
CPathHazard::CPathHazard() {}      // body now empty
```

Things that do **not** split the batch (all measured): chained assignment
(`a = b = 0`), interleaved order (`a; c; b; d`), a member-initializer list,
block scopes, calling two inline member helpers (`ResetLeg(); ResetStrike();`),
an `i64 z = 0;` shared temp. Only a real sub-object constructor does.

## Corroboration before you commit to the shape

Look for the same pair being moved as a unit elsewhere. Here the serializer
already streamed them pairwise through one helper
(`SerQuadPair(s, tag, &m_legDeadline)` / `(&m_strikeDeadline)`, reading `p[0]`
and `p[1]`) — an independent witness that `{deadline, window}` is one object.
The helper then takes `CHazardTimer*` and the `p + 1` pointer arithmetic goes away.

## Evidence

`CPathHazard` (2026-07-28). Both constructors carried the eight stores:

- `??0CPathHazard@@QAE@XZ` 0x13170 — 99.87% -> **100% EXACT**.
- `??0CPathHazard@@QAE@PAUCGameObject@@@Z` 0xb35a0 — the same batch order lands,
  and the derived-vptr stamp moves after the stores as retail has it.
- `?SerializeDispatch@CPathHazard@@` 0xb4d30 — 100%, unchanged by the retype.

`CStatusBarMgr` supplies the score-led negative control. Three adjacent
`{last, interval}` pairs were left as six loose `i64` fields because the first
aggregate experiment lowered `CPlay::LoadGameAssetNamespaces`. Retail independently
places each pair's stores at one member-initialization position, writes each as
`lastLo, intervalLo, lastHi, intervalHi`, and passes each pair to `SyncClockPair`.
Retyping them as `SbiClockPair` objects is therefore required even though the caller
score moves. The natural `SbiClockPair() : m_last(0), m_interval(0) {}` form emits the
retail store order and stays inline; the reconstruction-specific four-half body
crosses `/Ob1` at one site in both `CPlay` and `CMulti`, adding a C1 state. This is a
useful adjudication rule: the aggregate identity comes from batch boundaries and
complete-object consumers, while a temporary inline-cut regression is not contrary
evidence.

`CWarlord::CWarlord` (0x42d40) supplies a second positive control. Four loose
`i64` members at +0x88..+0xa7 emitted one batch of four lows followed by four
highs. Retail instead emits `+0x88,+0x90,+0x8c,+0x94`, then
`+0x98,+0xa0,+0x9c,+0xa4`, immediately after the preceding `CString` member
constructor and before the derived-vptr stamp. Modeling those ranges as
`m_cooldownTimer` and `m_notifyTimer`, each a two-`i64` `WarlordTimer`, reproduces
both batches exactly and moves the first divergence from +0xfc to +0x12d
(78.1128% -> 78.8385%). `SerializeDispatch` independently walks each range through
one `i64*` cursor. Initializer-list versus constructor-body assignments and a
typed helper versus native MFC `void*&` lookup were byte-flat; a classified
32-island/33-state campaign found one compiler island. The remaining first
divergence is the separately bounded in-place tile-snap register pair, not a
reason to flatten the timers again.

## Related

- [ctor-scalar-seeds-interleaved-are-a-mem-init-list](ctor-scalar-seeds-interleaved-are-a-mem-init-list.md)
  — the same read applied to scalar seeds woven between member ctors.
- [ehvec-member-array-not-adjacent-fields](ehvec-member-array-not-adjacent-fields.md)
  — another "the codegen shape names the sub-object" identity lever.
