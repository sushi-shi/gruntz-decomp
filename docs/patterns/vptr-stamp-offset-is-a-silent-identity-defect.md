# A vptr stamped into the wrong SUBOBJECT moves two bytes and no channel reads them

tags: cpp:ctor cpp:dtor cpp:virtual cpp:struct | asm:mov | topic:correctness topic:identity

symptoms: a ctor's `mov DWORD PTR [this+N],OFFSET ??_7Class@@6B@` where retail uses a
  different `N` or a different `??_7`; the function reads as REGALLOC/SCHEDULING with
  "bytes first differ at +0xNNN"; at run time every virtual call through that
  subobject dispatches on the wrong table

confidence: 10/10 (hermetic + real-COFF injected positives, both directions)

A constructor stamps each subobject's vtable. Two independent facts live in that one
instruction: the **displacement** says which subobject the vptr belongs to, and the
**relocation** says which class the object claims to be. Get the displacement wrong
and a polymorphic member's vptr lands in the neighbouring field; get the relocation
wrong and the object's dynamic type is another class. Either is a correctness defect
of the worst kind, and either moves **two bytes**.

```asm
mov DWORD PTR [esi+0x70],OFFSET ??_7CObject@@6B@   ; the embedded CObject member
mov DWORD PTR [esi+0x08],OFFSET ??_7CButeNode@@6BzPtrColl@@@   ; a SECONDARY base
```

## Why nothing else sees it

`offsetscan` suppressed the whole line until 2026-08-23: a relocated line's
displacement is usually a masked link-time address (`[eax+<g_table>]`), so dropping
it is right - **except here, where the relocation is the IMMEDIATE and the
displacement is a real member offset**. That suppression is now split by which operand
the relocation owns. `assert-relocs` proves the referent resolves, not where it is
stored. `vtable_owner --audit` proves a `VTBL()` binding against RTTI, which is the
vtable's own identity, not the store. And two bytes read as regalloc residue.

## How to read it - the COFF bytes, not a disassembly

A stamp is opcode `C7` with a ModRM whose `reg` field is 0 and a DIR32 relocation in
the imm32, so the relocation site's own address decodes the instruction **backwards**
with no ambiguity: try each candidate start, re-encode forward, accept the one that
ends exactly at the imm32. The whole image reads in about a second.

Two alignment rules the sieve had to learn, and a hand reading will repeat them:
**the alignment key can never be the field under test.** Keying on the referent and
asking whether the referent differs is a tautology; measured on a real object,
re-pointing ONE relocation made difflib delete-and-insert, re-pair the neighbours and
report TWO wrong offsets that were not there. So the displacement is read from a
referent-keyed alignment and the referent from a displacement-keyed one, and each
reading is WITHHELD when its own key's multiset differs.

## Measured 2026-08-23: the channel is CLEAN

`gruntz walls vptrscan` over the whole image - 989 stamps through a non-frame base in
functions both objects define, 974 aligned pairs, **zero differing in displacement and
zero differing in vtable symbol**. The split is non-trivial: 18 stamps at a NON-ZERO
subobject offset, 228 distinct vtables, and the multiple-inheritance population the
question is really about (a secondary base's own `??_7C@@6BBase@@@`) is 23 direct
stamps plus 3 through a register, matching retail exactly on offset, symbol, function
and count. Four rows are withheld, every one an inlining divergence in the stamp COUNT
with each paired stamp at the same offset naming the same vtable: an extra
`CGruntCoordList` in `CGrunt`'s ctor, an extra `CRgn` in
`CGruntzMgr::TransitionState`, a duplicated `CObject` at +0x70 in
`CDDrawWorkerHost::RebuildPlanes`, and a `CResolveNode` retail stamps in
`~CWwdGameObject` that we do not.

Re-run the sieve after any inheritance or member-layout change; it costs a second.
A frame-relative stamp (`[esp+N]`, a stack-constructed object) is a slot number rather
than a subobject offset and belongs to `framescan`.

## The companion reading: `vptrscan --slots`

The stamp says the object claims the right class; the SLOT says the call reaches the
right method of it. A virtual dispatch is `call DWORD PTR [reg+N]` where N is the slot
index times four, so a wrong N calls a different virtual function - and `offsetscan`
drops every `call`/`jmp` line by construction (it is excluding the switch table), so
nothing read that either. Same reading, same shape of answer: 4433 dispatches ours
against 4434 retail over 44 distinct slot displacements (deepest +0xe8 = slot 58) and
**five** rows whose multiset differs - none at the same call count, none using a slot
the other side does not, every one a pure count divergence of the same slots. No site
in the tree dispatches through a different slot than retail.

Two bounds the byte scan needs, both measured: a slot displacement is a pointer index,
so it is a multiple of 4 and far below a vtable's length. Unfiltered, `ff` occurring
inside another instruction's operand read `call [reg+83]` and `call [reg+0x90909090]`
as slots and produced three same-count "defects" that were every one a misalignment.

And a bound the byte scan CANNOT be given, which is why every differing row is re-read
off a decoded stream: **the two sides are not byte-symmetric**. Our base object has
every `call rel32` displacement zeroed by its relocation, while the delinked target
resolves a SELF-call internally and leaves a real negative displacement - four `ff`
bytes that decode as `call DWORD PTR [edi+0xe8]`. `CRezArchiveDir`'s destructor is
byte-identical to retail at 100.00 and still produced a retail-only slot 58 that way.
The re-read discards 23 of the byte scan's 28 rows, including every one of the 22 that
looked like a slot one side never uses.
