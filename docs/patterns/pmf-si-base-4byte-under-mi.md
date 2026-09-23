# A 4-byte member-fn-pointer under an MI class means the PMF TYPE lives on an SI base
tags: cpp:pmf cpp:inheritance | asm:mov asm:call | topic:codegen-idiom
symptoms: converting a class to multiple inheritance breaks every PMF-table fn at once (RunAct/RegisterActs pairs dropped -6..-20%); table stores/loads gain a second dword; dispatch gains a this-adjust
confidence: 9/10

MSVC 5.0 sizes a pointer-to-member-function by the INHERITANCE MODEL of the class it
is declared on (default `/vmb`-style best-case): single-inheritance -> 4 bytes (bare
code ptr), multiple-inheritance -> 8 bytes (code ptr + this-delta). The
REPRESENTATION is fixed at the PMF *type*, not at the stored method.

So when a leaf class gains a real second base (e.g. the tile-logic leaves'
RTTI-proven `: public CUserLogic, public CWapX`), every
`typedef i32 (CLeaf::*Handler)()` silently doubles to 8 bytes: the registration
stores write two dwords, the table stride changes, and the dispatch
`(this->*fn)()` emits the this-adjust dance - a whole-family byte break even
though no function's logic changed.

Retail's own tables show the dev shape: the RTTI template name
`.?AV?$zDArray@P8CUserLogic@@AEHXZ@@` is literally
`zDArray<int (CUserLogic::*)(void)>` - the handler type was declared on the
SINGLE-INHERITANCE BASE (4-byte form), and derived-class methods were cast in at
the registration sites:

```cpp
typedef i32 (CUserLogic::*Handler)();          // SI base -> 4-byte PMF, as retail
entry->m_fn = static_cast<Handler>(&CRollingBall::Update); // MI -> SI primary base
                                               // (explicit: no implicit derived->base
                                               // PMF conversion exists)
(this->*e->m_fn)();                            // this implicitly converts to the
                                               // PRIMARY base -> no adjust, same bytes
```

STEERABLE: declare every handler PMF type on the SI primary base, never on the MI
leaf. The cast is safe for methods reachable through the primary base (zero
this-delta). Evidence: the CWapX second-base conversion (2026-07-17) broke ~17
RunAct/RegisterActs pairs at once (-8.00/-6.27 uniform); retyping the typedefs to
`CUserLogic::*` + casting ~30 registration sites restored them.

## VC5 accepts the typed conversion; a union is not required

The later `edc4e96a0` reconstruction claimed that no cast between the 8-byte
`CGrunt::*` and 4-byte `CUserLogic::*` representations was legal, and introduced
`GruntActPmf` to read the first word through a union. Controlled builds against
the complete current `Grunt.h` and pinned VC5 disprove that blanket claim.

| Real-header consumer | VC5 `/O2 /MT` body, before alignment |
| --- | --- |
| Return `static_cast<CActHandler>(&CGrunt::FinishEntranceMove)` | `mov eax, <method>; ret`; one DIR32 relocation directly to `FinishEntranceMove`, no thunk |
| Return `static_cast<CActHandler>(handler)` from a `GruntActHandler` parameter | `mov eax, [esp+4]; ret`; no relocation or thunk |
| Cast a `CString::*` parameter to `CActHandler` | Rejected: unrelated owners |

The tests assert both representation widths, raw function bytes and exact
relocation type/addend/target, not just successful compilation. They live in
`scripts/test_container_headers.py`. This result licenses the existing
registrar's **zero-adjustment primary-base** conversions only: VC5's emitted
parameter conversion discards the adjustment word, so it is not a general
adapter for secondary-base method pointers.

The retained `ToActHandler` is the ordinary one-return inline cast, and the
table-store macro remains a separate operation. No union, raw pointer view,
class-layout lie or compiler forcing flag is needed. In the complete
registrar, all nineteen ordered handler targets remain unchanged.

This type correction does not by itself close the registrar's inline-budget
gap. At recovery base `7f4c0adff`, the macro union body has 786 instructions;
the typed inline conversion has 749, against retail's 734. A direct cast
expression without the inline boundary has 686, and a named scalar temporary
has 702. A union-local cast has 718 but is not a reason to retain an unnecessary
union. The inline binder composition has 846. Value and const-reference
conversion parameters are byte-flat; removing the unused union declaration
does not change this registrar's 749-instruction body. These are controlled
call-boundary differences, not proof that the highest one-step score selects
the original source. Preserve the typed helper while investigating the
remaining name/handler indexer cuts.
