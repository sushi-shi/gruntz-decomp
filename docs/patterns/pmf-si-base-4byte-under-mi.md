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
entry->m_fn = (Handler)&CRollingBall::Update;  // MI-leaf -> SI-base PMF cast
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
