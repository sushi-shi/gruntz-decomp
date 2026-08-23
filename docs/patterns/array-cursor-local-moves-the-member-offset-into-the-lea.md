# A cursor into an array folds the member offset into the `lea`; naming the array element folds it into each store

tags: cpp:local cpp:pointer cpp:array cpp:member | asm:lea asm:mov | topic:codegen-idiom

symptoms: `walls offsetscan` reports a family of mismatches whose deltas are all the
  SAME constant (`0x8->0x2c8`, `0xc->0x2cc`, `0x10->0x2d0`, `0x14->0x2d4`), our `lea`
  carries a base displacement retail's does not (`lea edi,[esi+ecx*8+0x2c0]` against
  `lea edi,[esi+ecx*8]`), and the two sides occupy the SAME byte range - the offset
  just lives in the other operand.

confidence: 10/10 (one function, taken to EXACT by the spelling alone)

## The two spellings

```cpp
CSbiHlRow* g = m_groupSlots;   // a cursor INTO the array
g[col].m_state = HLROW_RAMP_UP_HIGH;
g[col].m_counter = 0x13;
i64* rowClock = &g[col].m_last;
```

```asm
lea  edi,[esi+ecx*8+0x2c0]     ; 7 bytes - the member offset is in the lea
mov  DWORD PTR [edi],0x4       ; 6 bytes
mov  DWORD PTR [edi+0x4],0x13
```

```cpp
m_groupSlots[col].m_state = HLROW_RAMP_UP_HIGH;   // the ARRAY is named at each use
m_groupSlots[col].m_counter = 0x13;
i64* rowClock = &m_groupSlots[col].m_last;
```

```asm
lea  edi,[esi+ecx*8]           ; 3 bytes - only the scaled index
mov  DWORD PTR [edi+0x2c0],0x4 ; 10 bytes - the member offset is in the store
mov  DWORD PTR [edi+0x2c4],0x13
```

Both are the same addresses and the same total byte count; cl decides where the
array's member offset lands purely from whether the source ever names a POINTER to
the element. A cursor makes the element address a value, so it is materialized once;
naming `m_groupSlots[col]` leaves cl free to keep the scaled index alone and re-add
the member offset per access, which is what it does.

This is the mirror of
[interior-subobject-pointer-is-a-source-local.md](interior-subobject-pointer-is-a-source-local.md),
where retail materializes the interior pointer and the reconstruction folds. Both are
one axis: does the source name the pointer or the element? The offsetscan signature
separates them - a uniform delta with a matching `lea` on the other side is this one.

## The detection signature is the UNIFORM delta

A wrong member is one displacement. This is a family, and the constant is the array's
own offset in the class. `walls offsetscan <rva>` prints the pairs; if
`retail_disp - ours_disp` is the same value for every pair AND the byte extent of the
two blocks is equal, no field is wrong - the addressing shape is.

## Measured (2026-08-23, `src/Gruntz/SBI_RectOnly.cpp`)

`CStatusBarMgr::LoadRezMachineConfig` 0x105e40 **99.1540 -> 100.0000 EXACT** by
deleting the `CSbiHlRow* g = m_groupSlots;` cursor and naming `m_groupSlots[col]` at
its four uses. Zero other rows moved (1 of 4427 differed from the banked snapshot,
and that row was this one).

## The parked note that was wrong, and why

The function carried an `@early-stop` asserting this exact spelling had been tried and
cost `98.97 -> 91.22` through the sibling `ConfigureItem` and `GetDwordDef` call
sites. It does not reproduce: the TU's composition moved between that measurement and
this one, and the wall went with it. **Re-run a parked lever before believing its
recorded cost** - a comment records a build that no longer exists, which is why
`docs/comment-markers.md` says to re-derive an `@early-stop`'s residue rather than
trust it.
