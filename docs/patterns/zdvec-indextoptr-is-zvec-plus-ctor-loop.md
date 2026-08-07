# `call _zvec::IndexToPtr` + an inline ctor loop IS `_zdvec::IndexToPtr`

tags: cpp:inline cpp:container | asm:call | topic:codegen-idiom
symptoms: `insn_seq --seq` shows the target calling
`?IndexToPtr@_zvec@@QAEPADH@Z` where we call `?IndexToPtr@_zdvec@@QAEPADH@Z`;
elsewhere in the same function the target has no call at all but a run of
`g_typeColl` loads, an `imul` by `m_stride`, a `GrowTo` call and a
`??0CString@@QAE@XZ` loop; a hand-written copy of the grow logic next to the
real fetch compiles to nothing because its result is unused
confidence: 9/10

`_zdvec::IndexToPtr` is `_zvec::IndexToPtr` **plus** a trailing loop that
placement-news a `CString` into each newly grown slot:

```cpp
char* _zdvec::IndexToPtr(i32 i) {
    char* r = _zvec::IndexToPtr(i);           // bounds check / GrowTo / Report
    char* slot = m_alloc;
    i32 n = m_grown;
    while (n-- != 0) { if (slot) new (slot) CString(); slot += 4; }
    return r;
}
```

Both halves are inline-visible, so cl expands the outer one at every site but
decides **per site** whether to expand the inner one. In `CGrunt::ArrivalRecycle`
(0x59230) the FIRST of three identical probes keeps `call _zvec::IndexToPtr`
and inlines only the ctor loop, while the second and third expand the whole
thing (`mov m_grown,0`, the `m_lo`/`m_hi` compares, `imul m_stride`, the
`GrowTo` / error arms). All three are ONE source statement -
`*g_typeColl.GetNameRecord(key)`.

## The trap

Reading the first site as "a raw `_zvec` resolve" and then writing the
expansion out by hand as a SECOND statement produces neither shape: the
hand-written block's `rec` is unused, so /O2 deletes all of its address
arithmetic (no `imul`, no `lea`) and keeps only the side effects. The function
then looks 48 instructions short with a reloc sequence that has the right calls
in the wrong places.

`CTypeCollRuntime::GetNameRecordRaw()` names the first form (`ScratchResolve` +
an explicit `ConstructGrownSlots()`); `GetNameRecord()` names the fully inlined
one. Whichever you use, the ctor loop counts with `while (n-- != 0)`
(`mov ecx,eax; dec eax; test ecx,ecx; je; lea edi,[eax+1]`), never
`while (n != 0) { ...; n--; }` - that spelling costs one `lea` and one `dec`
per site.

CGrunt::ArrivalRecycle 0x59230: 68.46 -> 93.19, insn delta -48 -> -1.

related:
[reloc-sequence-diff-names-the-missing-statement.md](reloc-sequence-diff-names-the-missing-statement.md),
[inline-depth-splits-one-body-into-two-shapes.md](inline-depth-splits-one-body-into-two-shapes.md)
