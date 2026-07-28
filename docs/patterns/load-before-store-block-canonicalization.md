# cl canonicalizes a block to LOADS-before-STORES; retail sometimes has the store first — wall

**Tags:** cpp:local | asm:mov | topic:wall topic:scheduling

## Symptom

An otherwise byte-exact function differs by exactly one adjacent store/load
**pair** at the head of a join block: retail stores first, the recompile loads
first. Same registers, same encodings, just transposed.

```
retail:  89 6f 20   mov [edi+0x20],ebp   ; m_activeNode = 0
         8b 77 14   mov esi,[edi+0x14]   ; p = m_list.m_head
base:    8b 77 14   mov esi,[edi+0x14]
         89 6f 20   mov [edi+0x20],ebp
```

## Why it is a wall

cl5 **canonicalizes** the block: within one basic block it schedules an
independent load ahead of a preceding store (shortening the load→use distance to
the `cmp`/branch that consumes it). The transposition is not expressible from
source — writing the source in the load-first order emits the *same* load-first
code, so both source orders collapse to one output.

## Measured non-fixes

On `CSymParser::Clear` 0x13b850 (99.55%, the sole residue), nine spellings all
produced byte-identical output:

- `for (p = head; p; p = head)` / `while ((p = head) != 0)` / `for(;;){ p = head; if(!p) break; }`
- `while (m_list.m_head != 0) { CRezItmBase* p = m_list.m_head; … }` (fetch inside the body)
- `p` declared before the preceding `delete` / fetched *before* the null-store
- `this->m_activeNode = 0;`
- a `CParserObjList*` local for the whole list / for the head reads only

## Distinguish from

If the two instructions touch registers that differ as well, it is not this — go
look for a live-range difference first
([one-use-local-is-a-regalloc-knob](one-use-local-is-a-regalloc-knob.md)).
This pattern is *only* the pure transposition of an otherwise identical pair.
