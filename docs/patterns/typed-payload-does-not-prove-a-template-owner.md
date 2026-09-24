# A typed payload does not prove a template owner

tags: cpp:template cpp:class cpp:array cpp:inline | asm:call asm:coff | topic:source-model topic:mis-model

Recovering the element type and complete layout of a container does not, by
itself, recover a primary template or a capacity parameter. The input-device
group is a negative control against overgeneralizing genuine template wins.

All consumers prove a 136-byte object with two state dwords followed by 32
`CInputDevBase*` slots. Only one element type and capacity are observed. The
original template audit explicitly left generic origin open; its later
`CFixedPtrArray<CInputDevBase,32>` adoption cited those same layout/use facts,
without a surviving declaration, original mangled witness or second family.
The reconstructed template's emitted symbol names were not independent proof.

Retail keeps three ordinary call boundaries: device-group creation calls
`FillFrom` and failure cleanup calls `Clear`; collection teardown also calls
`Clear`; `FillFrom` calls `Add`. The inferred visible template expands all
of them under VC5 `/Ob1`. Moving its bodies after the callers in the real TU,
with whole-class or individual member instantiation, does not change that.
This agrees with the [unmarked-template control](vc5-template-members-inline-without-inline-keyword.md).

The retained reconstruction uses one typed `CInputDeviceGroup` class in
`DinMgr2/InputDeviceGroup.h`, keeping the existing inline two-field constructor
and all three helper APIs. The ordinary helper definitions remain at their
existing positions in `DinMgr2.cpp`. No caller, method arithmetic, storage
width, guard, loop or ownership operation changes. No erased base, synthetic
provider TU, explicit specialization or inlining pragma is introduced.

Controlled real-TU results:

| Function | Inferred template | Typed ordinary owner | Retail |
| --- | --- | --- | --- |
| `CreateDeviceGroup` 0x1331e0 | 187 B, 83 instructions, 3 calls, 10 branches; 19.960785% | 124 B, 51 instructions, 5 calls, 5 branches; exact | 124 B, 51 instructions, 5 calls, 5 branches |
| `FreeDeviceGroups` 0x1331a0 | 65% | 55 B, 25 instructions, 3 calls, 3 branches; exact | 55 B, 25 instructions, 3 calls, 3 branches |
| `FillFrom` 0x134be0 | 122 B, 60 instructions, 0 calls, 7 branches; 76.111115% | 126 B, 63 instructions, 1 call, 6 branches; exact | 126 B, 63 instructions, 1 call, 6 branches |

The complete normalized instruction and ordered-relocation pairs agree.
`Clear` stays exact at 20 bytes/9 instructions; `Add` stays exact at
36 bytes/12 instructions. Real-VC5 header controls verify the object extent,
slot offset, rejection of an unrelated element pointer, and retained calls
to each helper from a compiled consumer.

This falsifies the earlier assertion that typed uses alone established the
generic primary. It does not prove that every conceivable original template
implementation is impossible, nor recover the original class spelling.
Keep generic origin open to new positive source/type evidence. Meanwhile,
use the smallest supported typed owner and preserve its proven call boundaries,
rather than forcing an inferred template to imitate an ordinary class.

Reverse-audit signature: a newly inferred single-specialization template
keeps its small leaf bodies exact but expands every helper the retail family
calls; visibility controls are flat and the adoption has no evidence beyond
payload/layout. Revisit the identity claim before adding inline-budget levers.
This does not apply to source-proven `zDArray`, `zSymTab`, or typed-list families.
