# A dead store that SURVIVES in retail was separated by an unrelated store in source

tags: cpp:assign cpp:local | asm:mov | topic:codegen-idiom
symptoms: retail writes the SAME member twice back-to-back (`mov [p+0x30],ebp` then
`mov [p+0x30],edx`) while the recompile emits only the second; the source already has both
assignments adjacent
confidence: 8/10

MSVC 5.0 `/O2` eliminates a store that is immediately overwritten, but the elimination runs on
the **pre-scheduling** statement order and it only looks at the *adjacent* store. Any other store
between the two — even one the later scheduler hoists away again — blocks it.

So when retail keeps a visibly dead store, the two writes were NOT adjacent in the original
source. Find the third store that sat between them: it is usually the one that the schedule
shows moving (a global-load-then-store such as `m_startClock = g_frameTime`). A store of a bare
constant `0` does NOT block it — cl groups the zero stores and DSEs through them; the blocking
store must carry a *loaded* value.

`CTileTriggerContainer::AddGiantRockLogic` 0x116cf0:

```cpp
// the recompile DSEs m_dutyOffSpan = 0
e->m_dutyOffSpan = 0;
e->m_dutyOffSpan = dutyOffSpan;
e->m_startClock  = g_frameTime;

// retail: the dead store stands, and the scheduler still ends up with the
// two m_dutyOffSpan writes adjacent
e->m_dutyOffSpan = 0;
e->m_startClock  = g_frameTime;
e->m_dutyOffSpan = dutyOffSpan;
```

73.80 -> 77.41. The store ORDER in the emitted stream is not evidence for the source order:
cl freely reorders stores to distinct offsets of one base. Only DSE reads the source order.

## Related

- `docs/patterns/struct-rvalue-per-use-dead-half-store.md`
