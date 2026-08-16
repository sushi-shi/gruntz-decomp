# TRAP: an MFC accessor at `sec 0` in the delinked obj does NOT mean that compiland had the inlines off

- **Confidence**: 10/10
- **Tags**: `cpp:inline` `msvc5:mfc` | `topic:tooling` `topic:scoring-artifact`

## The tempting (wrong) inference

`FontRenderer::DrawWrapped` shows retail calling `?Width@CRect@@QBEHXZ` five
times where we inline it, and the delinked object looks like a smoking gun:

    llvm-objdump -t build/objdiff/target/font.c.obj | grep CRect
    [36](sec  0) ... ??0CRect@@QAE@HHHH@Z      <- EXTERNAL
    [60](sec  0) ... ?Width@CRect@@QBEHXZ      <- EXTERNAL

while our base obj DEFINES both as COMDATs. It reads as "retail compiled this TU
with `_AFX_ENABLE_INLINES` off" — and the corresponding device (a config
`#define` honoured by `<MfcWin.h>` *before* `<afxwin.h>`, since `<MfcNoInline.h>`
undefines it too late for a TU whose own header already pulled `<afxwin.h>`)
does close the five-call deficit.

**It is refuted, and it breaks the link.**

## Why it is wrong

`sec 0` in a delinked object is a DELINKER decision, not a compiler one. Any body
listed in `config/retail/functions_static_libs.tsv` is carved out as library and emitted
as an undefined external no matter which compiland actually emitted it. Both
rows are there:

    0x029ac0,??0CRect@@QAE@HHHH@Z,NAFXCW,HIGH,mfc-4.2-header-inline
    0x17b500,?Width@CRect@@QBEHXZ,NAFXCW,HIGH,mfc-4.2-header-inline

Read the ADDRESS instead. `font.cpp`'s last function is `LayoutWrapped`
(0x17b120 + 0x3c6 = 0x17b4e6) and `?Width@CRect@@QBEHXZ` sits at **0x17b500** —
immediately after it, INSIDE the font compiland's contribution. With the inlines
off cl emits no body at all, so a COMDAT copy inside that TU's range proves the
opposite: retail's `font.cpp` had `_AFX_ENABLE_INLINES` **on**, and cl simply
declined to inline `Width()` into a 0x8xx-byte function. Ordinary inline-budget
divergence, not a flag difference.

## And the link says so too

Release MFC has no out-of-line copies of these — only `NAFXCWD.LIB` /
`MFC42D.LIB` (the DEBUG variants) contain `?Width@CRect@@QBEHXZ`. Every other TU
that uses `CRect(int,int,int,int)` references it without defining it, so
`font.obj` is the image's SOLE provider of that COMDAT; suppressing it takes
`ninja candidate` from 0 unresolved to `LNK1120: 2 unresolved externals`
(11 objs on `??0CRect`, font on `?Width@CRect`).

## Rule

To decide whether a compiland had MFC's inlines on, ask **which TU's address
range the COMDAT falls in**, never what section the delinked object puts the
symbol in. And check `ninja candidate` before believing any change to a shared
MFC wrapper: an inline suppression can silently orphan a COMDAT the whole image
depends on.

## But the budget is NOT the explanation either (measured 2026-08-16)

The closing line above - "cl simply declined to inline `Width()` into a 0x8xx-byte
function, ordinary inline-budget divergence" - is refuted. **cl 5.0 with the inlines on
never declines `CRect::Width`, at any budget.** Five caller shapes, one probe TU, unit
flags `/nologo /c /O2 /MT /GX`, counting `call ?Width@CRect@@QBEHXZ` relocations:

| caller shape (5 `rc.Width()` sites each) | Width calls emitted |
|---|--:|
| global `CRect` receiver, 25 sites, nothing else | 0 |
| by-value `CRect` parameter receiver | 0 |
| by-value parameter also passed BY VALUE onward | 0 |
| **25 expansions of a 12-statement inline ahead of the sites** (20 of the 25 were REJECTED, so the budget was demonstrably spent) | 0 |
| MFC `CString` traffic + a destructible local (`/GX` frame) | 0 |

The fourth row is the decisive one: the budget was exhausted enough to turn 20 of 25
`heavy()` sites into real calls, and every `Width()` beside them still expanded.
`?Width@CRect@@QBEHXZ` titrates as a budget-EXEMPT callee (25/25 expanded at PAD=0,
i.e. `cb <= 0x28`), and per the model an exempt callee is inlined regardless of budget
or site count - see [inline-budget-emits-ool-comdat.md](inline-budget-emits-ool-comdat.md).

So `FontRenderer::DrawWrapped`'s five `call`s remain **unexplained by any source or
budget lever we have**. What the address argument above does still prove is where the
body came from: `?GetAt@CString@@QBEDH@Z` at 0x17b4f0 and `?Width@CRect@@QBEHXZ` at
0x17b500 are one COMDAT band between font's last function (`LayoutWrapped`, ends
0x17b4e6) and feccrypt's first (0x17b510), our own font.obj emits the `GetAt` half of
that band from the same source, and neither symbol appears anywhere in release
`NAFXCW.LIB` (`grep -ao` finds 0 in NAFXCW, 22 in NAFXCWD). Do not spend another lane
on a source spelling for the `Width` half.
