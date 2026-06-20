# Assembly patterns (MSVC 5.0, x86, /O2 /MT)

How recovered-assembly shapes map back to the Gruntz C++ source. The knowledge
base lives in this folder — **ONE FILE PER PATTERN**, indexed in
[`INDEX.md`](INDEX.md). Modeled on the vostok project's
`docs/binary_matching/patterns/` (which targets MSVC 8.0 /Od+LTCG — a *different*
toolchain; do not import its pattern contents, only the schema). Our target is
**MSVC 5.0 /O2** — the codegen idioms here are ours, recovered against
`GRUNTZ.EXE`.

## Searching (matchers: do this when staring at a stuck diff)

1. Grep [`INDEX.md`](INDEX.md) — one line per pattern
   (`- [title](file.md) — cN — tags — symptoms`, cN = confidence/10):
   - by construct: `grep 'cpp:switch' INDEX.md` (also `cpp:ctor`, `cpp:eh`, …)
   - by mnemonic: `grep 'asm:idiv' INDEX.md` (also `asm:call`, `asm:jmp`, …)
   - by topic: `grep 'topic:scoring-artifact' INDEX.md` (also `topic:wall`, …)
   - by symptom token: `grep 'rand' INDEX.md`, `grep 'jump table' INDEX.md`
2. Read ONLY the hit files; follow `variants:` links.
3. Full-text fallback: `grep -rli '<token>' docs/patterns/`.

## The two big classes of pattern

- **STEERABLE** — a source spelling reproduces the target shape (e.g. declare a
  method `static` not `__thiscall`; cast switch cases to `(int)`; reorder cases
  to source order). These CLOSE the diff → match. Topic tag `topic:codegen-idiom`.
- **WALL** — a documented MSVC5 codegen choice no natural source spelling
  reproduces (rand%peel, out-param zero-init scheduling, register pinning), or a
  delinker/objdiff **scoring artifact** where the code bytes already match. These
  do NOT close; recognize them so you stop chasing (orchestration §2a) and either
  accept the plateau or wait for the TU to complete. Topic tags `topic:wall`,
  `topic:scoring-artifact`.

## Per-file schema (authoring: new pattern = new file + one INDEX.md line, SAME commit)

```markdown
# <title — symptom-first>
tags: cpp:<construct> | asm:<mnemonic> | topic:<category>
symptoms: <raw grep tokens: asm fragments, score behaviors, mangling letters>
confidence: N/10
variants: <sibling-file.md, …>   (only when true variants exist)

<DESCRIPTION first: 1–3 lean sentences — what this is and when you see it.>

```cpp
<minimal C++ spelling — what to WRITE>      (omit if no C++ side)
```
```asm
<minimal target-side asm signature — what you SEE>   (omit if no asm side)
```
<1–2 trailing lines: steerable-or-wall + evidence (functions, %s).>
```
