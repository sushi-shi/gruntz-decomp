# Statement order decides whether cl can cross-jump a switch-arm tail

tags: cpp:switch cpp:branch cpp:store | asm:jmp asm:mov asm:or | topic:codegen-idiom topic:wall topic:regalloc
symptoms: a large per-case switch has periodic short arms; repeated member-store or immediate counts differ; an arm jumps into another arm's last two or three instructions
confidence: 10/10
variants: retail-duplicates-small-return-epilogues.md, goto-fail-shares-one-exit-block.md, switch-arm-break-not-return-replicates-the-epilogue.md

Do not classify a periodic 10-14 byte arm deficit as an unsteerable C2
tail-merge until the semantic operand multisets agree. A shared tail can be a
direct reading of statement order: cl can cross-jump only the suffix that the
source makes common.

`CGrunt::LoadPickupSprites` 0x65e80 is the decisive controlled A/B. Its lookup
macro originally ended each arm as:

```cpp
id = idv;
m_pickupGeoSrc = geo;
```

That form scored 77.2767. The candidate emitted only 43 stores to
`m_pickupGeoSrc` (`this+0x3d8`) against retail's 58, only three stores to
`m_powerupDuration` (`this+0x378`) against retail's seven, and materialized
voice id `0x3bf` six times against retail's one. The periodic short arms were
therefore not harmless residue: cl was sharing the wrong suffixes.

Retail's six W/A/R/P/Coin/Stopwatch arms each store their lookup result and
then converge on one `mov edi,0x3bf`. Reversing the two real statements exposes
exactly that suffix:

```cpp
m_pickupGeoSrc = geo;
id = idv;
```

The source change raises the normal report to 80.7749 and makes every semantic
multiset exact: 90/90 stores, 184/184 immediates, 142/142 ordered referents,
68/68 calls, 164/164 branches, 2/2 returns, and 249/249 relocations. It also
preserves all seven duration stores and emits one `0x3bf`. This is a source
recovery, not a score-only reorder: the target's shared suffix states the
statement order.

After that correction, candidate and retail still differ by one instruction
and eight bytes (0x1498/1250 versus 0x14a0/1251). Their duplicated-run reports
are symmetric and semantic operands are exact. Ninety-seven target-adjacent
TU-state trials, 49 top-of-TU trials, 19 relational variants, and 29
commutative variants each found only the baseline compiler island. That
post-adjudication residue is the genuine regalloc/scheduling wall.

A second face of the same pass appears when an arm ends `flags |= 0x10`.
Whether cl emits mergeable `or DWORD PTR [member],0x10` or
`mov eax,[member] / or al,0x10 / mov [member],eax` depends on whether EAX is
free. One register choice can change instruction selection and tail
mergeability across many arms. Treat that as codegen closure only after calls,
branches, referents, stores, and immediates have been reconciled.

`CStatusBarMgr::SetTabState` 0x100d70 is a different source pattern. Its arms
ended in statement-identical `return 1;`, feeding the early return cross-jump.
Changing each arm to `break;` plus one trailing `return 1;` took 88.53 to exact.
Always inspect the arm terminator and shared suffix before assigning a switch
to the residual tail-merge family.
