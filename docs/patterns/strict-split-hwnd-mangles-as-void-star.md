# A `<Win32.h>` TU's `HWND` parameter mangles `PAX`, so an MFC-side declaration of it must say `void*`
tags: cpp:mfc cpp:include cpp:param | topic:tu-layout topic:scoring-artifact
symptoms: `NEW declared-only alias: ?Fn@@YAX...PAUHWND__@@...@Z (defined nowhere)`; `llvm-nm` shows the definer emitting `PAX` where the caller emits `PAUHWND__@@`; a header declaring a Win32 helper with `void*` that its `.cpp` defines with `HWND`
confidence: 9/10

`windows.h` picks the handle representation from `STRICT`: with it, `DECLARE_HANDLE`
makes `HWND` a distinct `struct HWND__*`; without it, `HWND` **is** `void*`. `<Mfc.h>`
defines `STRICT` (afx.h does), `<Win32.h>` does not. So the SAME `HWND` parameter
mangles differently depending on which umbrella the TU used, and a function defined in
a `<Win32.h>` TU exports `PAX` where an MFC TU calling it emits `PAUHWND__@@` — an
unresolved reference, caught by `verify undefined-closure`.

```cpp
// src/Utils/TimeSplit.cpp  -  a <Win32.h> TU: this emits ?BlockScreenSaver@@YAHPAXIIJ@Z
i32 BlockScreenSaver(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam) { ... }

// include/Net/LobbyDialogs.h  -  an <Mfc.h> (STRICT) header: HWND here would emit
// PAUHWND__@@ and resolve to nothing, so the parameter is spelled void*.
i32 BlockScreenSaver(void*, UINT, WPARAM, LPARAM);
```
So a `void*` on the MFC side of such a declaration is FORCED, not a lost type: it is the
same type the definer used. Retyping it is only correct together with moving the
definition's TU to `<Mfc.h>`. Found on `BlockScreenSaver` (TimeSplit.cpp) and
`SetActiveAndFocus` (HeapDiag.cpp), both declared in MFC headers.
