# An inline ctor with NO rel32 caller: `??_H` took its address for a member array

tags: cpp:ctor cpp:inline cpp:class cpp:array | asm:push asm:call | topic:identity topic:codegen-idiom
symptoms: `gruntz sema xref <ctor-rva>` prints "no direct call/jmp rel32 caller
in .text" and then "referenced as data / address-taken"; a `push <ctor>; push
<count>; push <sizeof>; push <base>; call` quadruple; a consumer TU carrying MORE
relocs than retail, the surplus being calls to a small ctor retail expands in place
confidence: 10/10

A constructor whose out-of-line copy has **no rel32 caller at all** is an INLINE
member. Its COMDAT exists only because cl 5.0 hands the ctor's **address** to
`??_H` (`vector constructor iterator`) for a member array of class type — and an
address-take forces the body to be emitted even though every ordinary site
expands it in place. That is retail's both-shapes result with no artifact.

```cpp
// header - the real shape
struct CSbiHlRow {
    RVA(0x000c86d0, 0x11)
    CSbiHlRow() { m_lastLo = 0; m_intervalLo = 0; m_lastHi = 0; m_intervalHi = 0; }
    ...
};
class CStatusBarMgr {
    CSbiHlRow m_groupSlots[3];      // <- these two arrays are what take the address
    CSbiHlRow m_hlGrid[12];
};
```
```asm
c8041: push 0x403a3a      ; &??0CSbiHlRow  (through its ILT thunk)
c8052: push 0x3           ; element count -> m_groupSlots[3]
c8066: push 0x18          ; sizeof(CSbiHlRow)
c8074: push eax           ; array base (esi+0x2c0)
c8081: call 0x1aa5        ; `vector constructor iterator'
```

Steerable. cl picks per site: scalar members and short arrays get the body
unrolled in place (retail's `CMulti::LoadGameAssetNamespaces` expands it three
times), a longer array goes to `??_H`. Define the ctor OUT of line and you lose
both halves — every site becomes a `call`, which shows up as surplus relocs.

The pin travels with the COMDAT, so put `RVA()` on the header definition and let
`labels.py` attribute it to whichever obj emits it. Retail agrees on the owner:
0xc86d0 sits directly after `CPlay::LoadGameAssetNamespaces` (0xc7ec0 + 0x5f5 =
0xc84b5), inside play's contribution, not in sbi_rectonly's span. Expect the
labels gate to report the matching per-unit LOST/GAINED (`GRUNTZ_LABELS_ACK=<unit>`,
then commit `config/labels_manifest.tsv`); the TOTAL must be conserved.

## Evidence

`??0CSbiHlRow@@QAE@XZ` (0xc86d0, 17 B) was defined out-of-line in
`SBI_RectOnly.cpp`, so both play.obj and multi.obj carried `U ??0CSbiHlRow` —
the 74-vs-71 reloc surplus in `CMulti::LoadGameAssetNamespaces`. Moving the body
into `include/Gruntz/StatusBarMgr.h` as an inline member: the ctor became
**100.0000 EXACT** and re-homed to `play`, `?LoadGameAssetNamespaces@CMulti`
84.4591 → 85.8470, `?LoadGameAssetNamespaces@CPlay` 78.3187 → 79.2254, unit
`multi` 97.2093 → 97.3382. Labels conserved: play 92 → 93, sbi_rectonly 98 → 97.

variants: [eh-funclet-band-owns-the-inline-dtor-comdat.md](eh-funclet-band-owns-the-inline-dtor-comdat.md),
[inline-budget-emits-ool-comdat.md](inline-budget-emits-ool-comdat.md),
[interleaved-comdat-methods.md](interleaved-comdat-methods.md)
