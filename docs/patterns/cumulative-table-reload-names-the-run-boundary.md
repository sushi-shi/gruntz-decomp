# A cumulative table's MISSING reload names where the running total restarts
tags: cpp:expr cpp:array cpp:member | asm:mov asm:add | topic:correctness
symptoms: `mov reg,[this+N]; add eax,reg; mov [this+N+4],eax` repeated down an array, with the reload ABSENT at a few elements | a long run of GetInt/GetDwordDef calls each accumulating the previous slot
confidence: 10/10

A table built as `t[k] = t[k-1] + f(...)` compiles to one reload of the previous element
per row. Where retail SKIPS that reload and stores the call result straight into the slot,
the running total RESTARTS there - the table is several cumulative runs laid end to end,
not one. This is a CORRECTNESS reading: the runs are the boundaries of independent
weighted-random pools, so getting it wrong silently changes drop probabilities.

```cpp
m_battlezPct[2] = m_battlezPct[1] + g_buteMgr.GetInt("Multiplayer", "BrickzPercent");
m_battlezPct[3] = g_buteMgr.GetInt("Multiplayer", "RedBrick");   // NEW RUN - no reload
m_battlezPct[4] = m_battlezPct[3] + g_buteMgr.GetInt("Multiplayer", "BlueBrick");
```
```asm
    mov    DWORD PTR [esi+0x584],eax      ; [2]
    mov    DWORD PTR [esi+0x588],eax      ; [3] - NO `mov reg,[esi+0x584]` before it
    mov    edx,DWORD PTR [esi+0x588]      ; [4] reloads [3], the run continues
```
Steerable, and cross-checkable against the READER: `WapRand(m_battlezPct[2])` picks the
category, `WapRand(m_battlezPct[37])` picks within the last run, and `m_battlezPct[17]`
is that run's first bound - exactly the three boundaries the reloads mark.
`CStatusBarMgr::LoadBattlezItemConfig` 0xfdc00 93.66 -> **99.13**, three runs recovered
(brick colours at [3], toys at [7], tools at [17]).
