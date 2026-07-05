# Using `<afxwin.h>` (CWnd/CRgn/CGdiObject) — undef `_AFX_ENABLE_INLINES` for clang
tags: cpp:mfc cpp:include | topic:tu-layout topic:toolchain | topic:codegen-idiom
symptoms: adding `#include <afxwin.h>` builds under wine `cl` but the ninja `gen_labels <unit>` step FAILS with `FileNotFoundError: ...tmp*.ll` (no IR emitted); the real clang error is `afxwin1.inl(1027): error: a type specifier is required for all declarations` on `_AFXWIN_INLINE CMenu::operator==(...)`
confidence: 9/10

`<Mfc.h>` deliberately pulls only `<afx.h>` + `<afxcoll.h>` (CObject/CString/CFile/
CObList/…), NOT `<afxwin.h>` — so CWnd, CRgn, CGdiObject, CDC, CMenu, CView are absent,
and TUs modeled them with hand-rolled views (a local `class CWnd`, `struct CursSinkVtbl`,
`struct CreditzRgn`, …). Pulling the real `<afxwin.h>` to fold those views USED to look
like a hard wall: the retail **base compile under wine `cl` (MSVC5) succeeds**, but the
**clang label step** (`labels.py` → `clang --driver-mode=cl -Xclang -emit-llvm`, which
extracts each TU's function/global mangling) **hard-errors** inside MFC's own
`afxwin1.inl`:

```cpp
_AFXWIN_INLINE CMenu::operator==(const CMenu& menu) const   // <-- no return type
```

`_AFXWIN_INLINE` expands to `inline`; the `operator==`/`operator!=` bodies rely on the
**implicit-`int` return** MSVC5 accepts but clang rejects (`-fms-compatibility` does not
rescue a member-operator definition). `-emit-llvm` stops at the first fatal, no `.ll` is
written, and `gen_labels` dies reading the missing file.

**Fix — skip MFC's `*.inl` for clang only.** `<afxwin.h>` includes `afxwin1.inl` /
`afxwin2.inl` **only `#ifdef _AFX_ENABLE_INLINES`** (defined in `afxver_.h`, pulled early
by `afx.h`). The label step needs only the class **declarations** (for mangling), never
the inline **bodies** — so undef the switch for `__clang__` before `<afxwin.h>`:

```cpp
#include <Bute/ButeMgr.h>   // (or <Mfc.h>) — pulls afx.h, which defines _AFX_ENABLE_INLINES
#ifdef __clang__
#undef _AFX_ENABLE_INLINES   // label-step clang: skip afxwin*.inl (implicit-int CMenu::op==)
#endif
#include <afxwin.h>          // real CWnd / CRgn / CGdiObject / CDC / CMenu / CView
```

The wine `cl` base compile does **not** define `__clang__`, keeps `_AFX_ENABLE_INLINES`,
and gets the real inlines → correct codegen. `afx.inl` (CString etc.) was already pulled
by `afx.h` **before** the undef, so it is unaffected — only the `afxwin*.inl` bodies are
dropped, and those methods just become the out-of-line (reloc-masked) library entries the
match already uses. Matching-neutral (CreditzScreen::BuildText held 99.90% folding
`struct CreditzRgn` → real `CRgn`).

STEERABLE. This retires the "CWnd/CGdiObject can't be pulled" exception: a TU that needs a
real afxwin.h class does the 3-line undef and folds the view. Evidence:
ApiWrappers.cpp CreditzRgn → CRgn. Related: mfc-wall-is-breakable-switch-to-mfc.md
(the afx.h-subset case), emit-vtable-in-tu.md.
