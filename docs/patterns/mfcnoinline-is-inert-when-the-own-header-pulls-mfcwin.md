# `<MfcNoInline.h>` is INERT in any TU whose own header pulls `<MfcWin.h>`

> **LINK-PROVEN LIMIT (added at integration).** The mechanism below is real -
> `GRUNTZ_MFC_NO_INLINES` does reach where `<MfcNoInline.h>` cannot, and it does
> make cl emit retail's `call ?Width@CRect@@QBEHXZ`. But **enabling it does not
> link**: NAFXCW.LIB exports neither `?Width@CRect@@QBEHXZ` nor
> `??0CRect@@QAE@HHHH@Z`, so `ninja candidate` fails with `LNK1120: 2 unresolved
> externals`, with 9 further TUs reporting the CRect ctor. It was enabled in
> `src/Font/Font.cpp` by the wave that discovered it and REVERTED at integration.
> An earlier wave had already refuted the device on exactly this ground; that
> refutation was correct. Note what this proves about retail: `0x17b500` is a
> COMDAT from retail's OWN font compiland, not a library import - so reproducing
> the out-of-line call needs that body defined on our side, not a header switch.
> `gruntz build` does NOT link, so no gate below the full tier can catch this -
> run `gruntz link` before believing an inline-linkage experiment.


tags: cpp:include cpp:inline | asm:call | topic:codegen-idiom topic:wall
symptoms: a target obj has an undefined `?Width@CRect@@QBEHXZ` / `?SetRect@CRect@@QAEXHHHH@Z`
(or any other `afxwin1.inl` accessor) that the base obj does not; the TU already includes
`<MfcNoInline.h>` and nothing changes
confidence: 10/10

`_AFX_ENABLE_INLINES` is defined by `<afxver_.h>` (whenever `_DEBUG` is NOT), and
`<afxwin.h>` includes `afxwin1.inl` at its END under that guard. So the accessor bodies
are frozen in at the moment `<afxwin.h>` is first parsed, and an `#undef` after that is
a silent no-op.

`<MfcNoInline.h>` does exactly that `#undef`, but the CANONICAL INCLUDE ORDER puts the
TU's OWN header before the platform preludes - and the own header is usually what drags
`<MfcWin.h>` (hence `<afxwin.h>`) in. Every TU carrying `<MfcNoInline.h>` today is in
that position, so **not one of them actually suppresses an afxwin accessor**. Screen for
it in one line:

```sh
llvm-nm build/objdiff/base/*.obj  | grep -c 'Width@CRect'   # 0
llvm-nm build/objdiff/target/*.c.obj | grep -c 'Width@CRect'   # 1 (font)
```

The switch that DOES reach those units is a config `#define`, which the include-order
contract sorts above everything:

```cpp
#define GRUNTZ_MFC_NO_INLINES 1   // first line of the .cpp, above <rva.h>

#include <rva.h>
#include <Font/Font.h>            // -> <MfcWin.h> -> <afxwin.h>, now inline-free
```

`<MfcWin.h>` honours it next to the existing clang branch:

```cpp
#include <Mfc.h>
#if defined(__clang__) || defined(GRUNTZ_MFC_NO_INLINES)
#undef _AFX_ENABLE_INLINES
#endif
#include <afxwin.h>
```

**Only `afxwin1.inl` is suppressed.** `afx.inl` (CString, CObject, the collection
accessors) is already parsed by `<Mfc.h>`'s `<afx.h>` before the define can act, so the
blast radius is CRect/CPoint/CSize/CWnd only - measured on `src/Font/Font.cpp`, where
exactly ONE symbol moved (`?Width@CRect@@QBEHXZ`, matching retail's font obj) and no
other accessor became a call.

Score is NOT the acceptance test here: `FontRenderer::DrawWrapped` went 74.33 -> 74.17
(regalloc), while the reloc set became retail's. Take the referent fidelity.

Retail's tree-wide reach for this lever is small - `Width@CRect` (font) and
`SetRect@CRect` (gruntzmgr, and there it comes from an inlined `CCreditsState` ctor we
do not inline, so the define does not help) are the only two afxwin accessors that exist
out of line at all.

variants: [mfc-c1189-wall-breakable](mfc-c1189-wall-breakable.md),
[include-order](include-order.md),
[inline-ctor-comdat-via-vector-ctor-iterator](inline-ctor-comdat-via-vector-ctor-iterator.md)
