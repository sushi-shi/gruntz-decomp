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
listed in `config/retail/library_labels.csv` is carved out as library and emitted
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
