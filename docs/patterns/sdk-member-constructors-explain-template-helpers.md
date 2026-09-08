# SDK members explain an apparently authored constructor helper

tags: cpp:mfc cpp:templates cpp:constructor | topic:identity topic:source-shape

A hand-transcribed container can conceal both its SDK template identity and the
SDK members of its elements. The reverse signature here is an MFC growth and
serialization family, a 0x28-byte element containing two rectangles, and a
three-byte `mov eax,ecx; ret` function called from one placement-construction
loop but absent from a stack declaration.

The old model called that function `InitRezElem`, typed the rectangles as
`RECT`, and copied `CArray` into `CRezBufferObject`. An out-of-line authored
constructor added a call in `CFaderMesh::ApplyInit`, so the old review concluded
that the element must remain POD. That conclusion tested one declaration
boundary, not the complete SDK family.

Controlled VC5 `/O2 /MT /GX` probes used the actual `AFXTEMPL.H`:

| Element declaration | Serializer result | Constructor result |
| --- | --- | --- |
| Two `RECT` members, POD | 0x1c0-byte padded section; eight relocations | No constructor emitted |
| Two `RECT` members, explicit inline empty constructor | Same serializer result | Empty constructor expanded away |
| Two `CRect` members, implicit constructor | Exact 0x1ce-byte retail body (0x1d0 padded section), ten relocations | Exact three-byte implicit element constructor |

For the final row the payload matches the previously exact serializer byte for
byte. Every ordered relocation agrees after the two justified identity changes:
`InitRezElem` becomes `RezElem40::RezElem40()` and `ZeroRecords` becomes MFC's
`ConstructElements(RezElem40*, int)`. The latter emits the same 0x23-byte byte-fill
body. The element still occupies 0x28 bytes; both rectangle members are real MFC
objects. The complete serializer proves more than an isolated empty constructor
or coincidental layout agreement.

The surrounding rectangle locals matter too. Assigning a Win32 `RECT` to a
`CRect` selects its SDK assignment overload and introduces `CopyRect`; using
`CRect` for the assembled/dispersed local rectangles selects the compiler's
ordinary same-type assignment. Restore declarations and uses together. The
caller and standalone `SetSize` can still vary with real template visibility
and inlining; retaining the copied container to preserve that earlier source
hash would restore the modeling defect.

Use this signature to reopen an old "constructor-shaped helper is not a
constructor" conclusion. Inspect SDK member constructors, implicit special
members, placement construction, and the complete template body. Absence of an
out-of-line call at one site is not proof that its constructor was unavailable
or that the element was POD. Do not force emission with fake references or keep
hand-expanded SDK source to imitate one inlining decision.
