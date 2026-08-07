# Retail recomputes `v >> k` where the recompile CSEs it

tags: cpp:expr cpp:loop | asm:sar | topic:wall
symptoms: retail loads a member ONCE and then emits two (or four) identical `sar reg,5`
against copies of it; the recompile emits one `sar` and reaches the two consumers with
`lea [reg-1]` / `lea [reg+2]`. Every source spelling tried still CSEs
confidence: 7/10

Two independent tile-neighbourhood scans show the same shape. `CBattlezMapConfig::Scan`
0x35f10 preheader:

```
mov ecx,[eax+0x60]      ; m_screenY loaded ONCE (the load IS CSE'd)
mov ebp,[eax+0x5c]      ; m_screenX loaded ONCE
mov edx,ecx / mov esi,ecx / mov edi,ebp / mov eax,ebp
sar edx,5 / sar esi,5 / sar edi,5 / sar eax,5     <- four shifts, no CSE
dec esi / add edx,2 / lea ebx,[edi-1] / add eax,2
```

and again inside the loop body, and again in `CTriggerMgr::FindNearestEnemy` 0x77df0 where
`(x>>5) - r` and `(x>>5) + r + 1` each get their own `sar`. So retail's cl CSEs *memory
loads* but not the *arithmetic* built on them.

Tried and REJECTED (all still emit one `sar`, cl5 /O2 CSEs it every time):
- named locals `tileX = v >> 5` then `tileX - 1` / `tileX + 2` (the obvious spelling);
- the shift written out at all four use sites in the `for` header;
- the shift applied to the member read directly (`p->m_screenY >> 5`), no local at all.

The same asymmetry shows up as retail DUPLICATING an epilogue where our build cross-jumps
(three `return 0` tails kept apart in `AddToList3`, one `xor eax,eax` shared in ours). One
of these is steerable by source structure and one is not - see
`allocate-check-then-body-is-the-then-block.md` for the branch-polarity half, which IS
steerable. The shift half is not, with the spellings above.

Do not spend a budget re-deriving these three attempts. If a flag-level explanation is ever
found (an alias/CSE switch beyond `/O2 /MT /GX`), it would move a large number of functions
at once and belongs in `docs/linker-flags.md`, not here.
