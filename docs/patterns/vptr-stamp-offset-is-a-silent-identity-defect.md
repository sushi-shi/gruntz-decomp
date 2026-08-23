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
