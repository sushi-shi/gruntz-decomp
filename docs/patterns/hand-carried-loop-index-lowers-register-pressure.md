# A hand-carried loop counter/offset LOWERS register pressure — write the plain `for` with the full index

**Symptom.** A counted loop whose body is otherwise right, and every row of the diff
is "retail spends more instructions than we do":

```
- or   byte ptr [eax+0xa],0x1          <- base, one memory RMW
+ mov  ebp,dword ptr [eax+0x8]         <- retail, three instructions and a scratch
+ or   ebp,0x10000
+ mov  dword ptr [eax+0x8],ebp
```

plus retail carrying a `push ecx` / `sub esp,N` the base does not have, a `this`
(or another loop-invariant) spilled to that new slot and reloaded inside the loop,
and the base coming out **SHORTER than retail** — `RemovePlayerUnitsImmediately` was 154 B against
retail's 179, `CreateRange` 244 against 254.

**Cause.** The reconstruction hand-rotated the loop, or hand-carried the address
arithmetic:

```cpp
i32 n = last - row + 1;                 // hand-rotated downcount
CGrunt** cell = &m_units[row * COLS];    // hand-carried cursor
i32 g2 = row * COLS;                    // hand-carried offset accumulator
do { ... cell++; g2 += COLS; n--; } while (n != 0);
```

Every one of those is a strength reduction the compiler was going to perform
itself — but performing it in the SOURCE changes what cl sees. A hand-carried
cursor is a *named local*; the same cursor created by cl's own induction-variable
pass is a *derived value* it is free to home in a dead parameter slot. So the hand
form wins a callee-saved register that retail's form spends on something else, the
loop ends up one register RICHER than retail, and cl then picks a cheaper
instruction selection — the narrowed `or byte [m+2],1` instead of retail's
register read-modify-write, no spill of `this`, no extra frame slot.

The three-instruction RMW is therefore a **consequence of the loop shape, not a
spelling**. Measured on `RemovePlayerUnitsImmediately`: all five RMW spellings — `p->f |= K`, a
named temp, temp-plus-or, self-or, and a cached object pointer — compile to the
identical single `or byte ptr [eax+0xa],0x1`. So
[rmw-split-means-a-named-temp](rmw-split-means-a-named-temp.md) does not reach this
case; fix the loop and the RMW falls out.

**Fix.** Write the loop the way the dev did — a plain `for` with the bound as a
comparison, and the *full index expression* at each use:

```cpp
for (i32 r = row; r <= last; r++) {
    CGrunt** cell = &m_units[r * TM_UNITS_PER_PLAYER];
    for (i32 col = 0; col < TM_UNITS_PER_PLAYER; col++) {
        CGrunt* c = cell[col];
        if (c != NULL) {
            c->m_wwdObject->m_flags |= 0x10000;
            cell[col] = NULL;
            m_unitExited[r * TM_UNITS_PER_PLAYER + col] = 0;
        }
    }
    ...
}
```

Same for an output cursor: `out[n] = item;` not `*p++ = item;`. cl rewrites
`out[n]` into exactly retail's `mov ecx,[esp+home]; mov [ecx],eax; add ecx,4;
mov [esp+home],ecx` — the cursor lives in the dead `start` parameter home and the
callee-saved register goes to the loop-invariant (`suffix`) instead.

**Screen.** Base SHORTER than retail on a counted loop, with retail carrying an
extra frame slot / `push ecx` and a reloaded loop-invariant. The clean signal is
that retail's extra bytes are all *spill traffic and wider instruction forms*, not
extra statements.

**Results.** `CTriggerMgr::RemovePlayerUnitsImmediately` 0x6bd40 66.97 -> **100.00 EXACT** (the
downcount `do/while` plus the `g2` accumulator; 64-cell forest, 12 cells exact —
every exact cell had the plain `for` AND the `r * COLS + col` index, none had the
accumulator). `CDDrawPtrCollections::CreateRange` 0x142630 84.02 -> **100.00 EXACT**
(the `p` cursor; 181-cell forest, 12 exact, every one of them `out[n]`). Both had
been filed as regalloc walls — the first as "our narrowed `or` vs retail's dword
RMW is downstream of allocation", the second as "retail keeps `p` in a stack slot
and `suffix` in ebp; we have it exactly inverted".

**Not this lever** when the base is LONGER than retail, or when the loop's exit
epilogues are what differ — that is
[hand-rotated-loop-merges-the-exit-epilogues](hand-rotated-loop-merges-the-exit-epilogues.md),
which is about `ret` counts and must be screened on the asm.
