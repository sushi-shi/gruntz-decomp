# `<MfcNoInline.h>` is INERT in any TU whose own header pulls `<MfcWin.h>`

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
