# A dead stack store in retail is NOT an era anomaly — cl 5.0 SP3 emits them too

tags: cpp:local cpp:struct cpp:member | asm:mov | topic:wall topic:tooling
symptoms: retail writes an aggregate's fields from members and overwrites them a few
instructions later with no read in between, the base emits nothing there, and the row's
whole residue is exactly those extra loads + stores; the reflex is to call it the
RTM-provenance family
confidence: 8/10

Retail keeping a provably dead store looks like the era-compiler family, so lanes park it.
Run the both-sides census first: cl 5.0 SP3 emits dead stack stores routinely, and a
100%-matching function proves it from ordinary source. `SaveScreenshot` writes
`srcRect.right = 0;` and overwrites it two statements later; cl keeps the zero store and
the row is EXACT. So the question is never "can the compiler do this" — it is "which
source shape does it".

```cpp
// SaveScreenshot 100.0000 - cl KEEPS the dead .right/.bottom zero stores.
// .left/.top keep their zeros and stay live, so the store group is only PARTLY dead.
srcRect.left = 0;  srcRect.top = 0;  srcRect.right = 0;  srcRect.bottom = 0;
srcRect.right  = mgr->GetModeSize().cx;
srcRect.bottom = mgr->GetModeSize().cy;
```
```asm
; CGrunt::RectContains 0x51850 - the four instructions no spelling reproduces
mov  edx,[ecx+0x180]      ; m_lastTilePx.m_y
mov  [esp+0x14],edx       ; -> r1.top       DEAD, overwritten at +0x52
mov  edx,[ecx+0x17c]      ; m_lastTilePx.m_x
mov  [esp+0x10],edx       ; -> r1.left      DEAD, overwritten at +0x6d
```

WALL, bounded and characterized. The census (scan base AND target objs for
`mov <reg>,K(%esp)` twice to one slot with no intervening read) reports 56 base hits in
18 objs against a similar target set, mostly the same functions — so the mechanism is
shared. What is NOT reproduced is a store group that is **fully** dead: seven spellings all
compile byte-identically to the plain member read (plain `Coord at = m_lastTilePx;`; a
by-value `Coord LastTilePx()` inline modelled on the real `EntrancePx` COMDAT; direct
stores into the address-taken `RECT r1` killed by an aggregate assign; the same killed
field-wise; immediate-zero stores killed by an aggregate assign; immediate-zero stores
killed field-wise; `Coord at; Copy(&at, m_lastTilePx);` through a real `inline` free
function — cl inlines it and still eliminates the copy, so an address-take does NOT block
the elimination). The open discriminator is the SaveScreenshot one: part of the store group
must stay live. Affects `CGrunt::RectContains` 0x51850 (83.02, residue is exactly these 4
instructions: same call, branch and `ret` counts) and `CGrunt::RectContainsGated` 0x51a20
(82.53, same 4); `CTriggerMgr::ApplyTriggerB` 0x6e120 has the same pair spilled at
`[esp+0x20]/[esp+0x24]`.
