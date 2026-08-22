# Free-list init loops where retail stores through the one-past cursor - suspected inlined link-helper

tags: cpp:idiom msvc5:c1-inline | asm:lea asm:neg-disp | topic:wall topic:method
symptoms: an array-init linking loop scores mid-80s; retail addresses the
current element's stores as NEGATIVE displacements off the NEXT cursor
(`mov [eax-0xc],...` where eax = cur+stride) while ours addresses them
cur-relative (`mov [eax+0x18],...`); compare/value arithmetic still uses cur
confidence: mechanism proven by A/B; the source construct is UNMAPPED

## The three-spelling A/B (CMapArrayA::Allocate 0x9e740, 2026-08-22)

| spelling | score |
|---|---|
| hoisted two-cursor walk (`next` outside, `++e; ++next;`) - banked | 87.52 |
| stores through `next[-1].field` (inhuman probe) | **95.80** |
| per-iteration `next = e + 1; ... e = next;` (natural) | 84.72 |

The probe proves the codegen shape (stores one-past-relative, cur kept for
the compare and the `e - 1` value, single `add` tail) but fails the
no-sane-dev test and was NOT kept. The natural spelling that produces a
single-cursor tail measured WORSE, so no plain loop respelling reaches the
shape.

## Leading hypothesis

The era source linked nodes through a TINY inline helper (a BrickzNode /
free-list `Link`/`PushBack` member) that C1 expanded per iteration; the
helper's own body supplies the one-past addressing. The mapmgr TU repeats
the `x->m_openPrev = ...; x->m_openNext = ...` micro-pattern at ~8 sites
(Search seed handling, Expand, CellPush, Drain, ResetCells) - a helper
archaeology pass across the WHOLE free-list family is the next move, not
further per-loop spelling A/Bs. Sibling with identical state:
CMapArrayB::Allocate 0x9e860 (87.65 banked, same three-way result).
