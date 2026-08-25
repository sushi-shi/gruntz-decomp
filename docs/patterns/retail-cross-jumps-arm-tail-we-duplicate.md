# Retail cross-jumps an arm TAIL and we duplicate it — sharing the locals is not the inverse lever

tags: cpp:branch cpp:local cpp:switch | asm:jmp asm:call | topic:wall
symptoms: a CRT-symbol census (a CRT-symbol reference census (retired)) shows exactly ONE extra
call of a routine (`_atoi` 10 vs 11, `__ftol` 189 vs 190) in a function with two
source-identical `if/else if` or `switch` arms; retail's FIRST arm ends in a bare
`jmp` into the MIDDLE of the second arm's tail, ours emits the tail twice; block
counts differ by the length of that tail
confidence: 8/10

## Symptom

Retail merges only the LAST FEW instructions of two arms, stopping at the first
byte that differs, and the earlier arm reaches the merge point with a `jmp`:

```
retail, CBattlezMapConfig::LoadConfig 0x25020         ours
  Hard arm ... call __ftol                              Hard arm ... call __ftol
  mov [esi+0x48],eax                                    mov [esi+0x48],eax
  mov ecx,[esi+0x54]      <- differs (ecx vs eax)       mov ecx,[esi+0x54]
  mov [esp+0x10],ecx                                    ...
  mov [esp+0x14],edi                                    fmulp
  fildll [esp+0x10]                                     fmul  st,st(1)
  jmp  <Normal arm + 0x56>   <=== merge                 call  __ftol        <- 4th ftol
  Normal arm ...                                        fstp  st(0)
  fmuls <scale>              <=== merge target          mov [esi+0x54],eax
  fmul  st,st(1)                                        jmp <join>
  call  __ftol               (3rd and last ftol)
```

`CPlay::LoadByMode` 0xca200 is the same shape: the `GAME_BATTLEZ` arm's digit-skip
loop falls into a bare `jmp` at the `GAME_MULTI` arm's `push ecx; call _atoi; add
esp,4; mov ecx,edi; mov ebx,eax; call ?ReleaseData` tail, so retail calls `atoi` once
where we call it twice.

## What is NOT the lever

`identical-arms-need-distinct-locals.md` says cl5 cross-jumps arms whose remaining
STATEMENTS are the same statements, and that giving each arm its own block-scope
local is what makes cl emit both arms in full. The obvious inverse — join the
locals so the tails become statement-identical and cl merges like retail — was
measured on both functions and **does not fire**:

* `CPlay::LoadByMode`: hoisting `ins`/`desc`/`p`/`c` out of the two arms and
  dropping the `i32 num` temp (`level = atoi(p); ins->ReleaseData();`) removed the
  extra `mov ebx,edi` and made the two tails byte-identical. cl still emitted both.
  (It was still worth doing: 82.83 -> 83.15 and the tail is now retail's.)
* `CBattlezMapConfig::LoadConfig`: hoisting the per-`case` `i32 r` to one
  function-scope `r` produced a byte-identical object — same 5 `__ftol`, same
  score. Reverted.

So the merge/duplicate decision is not purely "are the statements the same". Read
it the other way round: the doc's rule predicts when cl merges TOO MUCH and how to
stop it; there is no known source spelling that makes cl merge when it has decided
not to. Both residues are one instruction-run each; park them.

`CMinimap::Refresh` at `0x0a3460` is the small no-call control.  Its
`SpriteTeamColorVariant` switch has separate `PRIMARY` and `default` arms that
both store `node->m_teamColor1`.  Retail cross-jumps them into one selected
address/store block; the current `/O2` state emits both.  This accounts for the
entire census delta: base/retail have 238/239 instructions, 48/47 branches,
6/6 calls, 4/4 returns, and 8/8 ordered relocations.  Combining the labels is
not the source: it collapses two more blocks (233 instructions, 46 branches)
and lowers the function to 67.00%.  Reordering the four complete arms is
byte-identical.  Keep the semantically complete switch and classify the one
extra `je`/`jmp` as this cross-jump wall.

Also measured on `LoadConfig`: retail folds the `g_diffScale` load into `fmul m32`
where cl preloads with `fld`/`fmulp`. Reassociating the expression
(`(m_x * g_diffScale) * r` instead of `r * (m_x * g_diffScale)`) does NOT produce
the fold — it only moves the `fmul st,st(1)` earlier and costs 1% (95.29 -> 94.29).
Keep the `r * (m_x * scale)` spelling; its instruction ORDER is retail's.

## See also

- `docs/patterns/identical-arms-need-distinct-locals.md` — the direction that IS steerable.
- `docs/patterns/identical-return-epilogue-tailmerge.md` — the same coin-flip for epilogues.
