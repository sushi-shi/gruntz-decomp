# The "MFC C1189 wall" is breakable: a <Win32.h> TU that needs MFC types switches to <Mfc.h>
tags: cpp:mfc cpp:include | topic:tu-layout topic:codegen-idiom
symptoms: a Win32 TU keeps a placeholder view/offset-cast of CMulti/CPtrList/CString because "<Mfc.h> can't be included next to <Win32.h> (C1189)"; `*(i32*)((char*)g_x + 0xNNN)` reads; local `struct CPtrList`/`CMulti` shims
confidence: 9/10

There is NO real barrier to using real MFC classes (CObject/CString/CFile/CPtrList/
CObList/CMulti…) from a TU that was written against `<Win32.h>`. `<Mfc.h>` is a
**superset** of `<Win32.h>`: afx.h pulls the SAME `<windows.h>` (so every Win32 type +
`timeGetTime`/`INT_PTR` stay available) AND gives the MFC classes. The C1189 error
("MFC apps must not #include <windows.h>") only fires when windows.h came FIRST — so
just replace the TU's `#include <Win32.h>` with `#include <Mfc.h>` and keep it FIRST
(before smack.h / DirectX / DDSurface.h, which re-include windows.h under a guard). Do
NOT keep a placeholder view or an `*(i32*)((char*)p+off)` offset-cast "because of the
wall" — there is no wall; use the real class.

```cpp
// BEFORE (Win32-walled): placeholder view + offset-cast
#include <Win32.h>
struct SelOwner_0c50f0 { char pad[0x528]; i32 m_528; ... };   // fake CMulti view
i32 h = *(i32*)((char*)g_64bd5c + 0x5c0);                      // offset-cast

// AFTER (wall broken): real classes
#include <Mfc.h>            // FIRST — superset of Win32.h
#include <Gruntz/Multi.h>   // real CMulti
i32 h = g_64bd5c->m_hostIndex;   // g_64bd5c is a real CMulti*; g_pool a real CPtrList
```
Matching-NEUTRAL (member/method access reloc-masks identically; real CString operator=
== the view's Assign; OnReset 0xc3e30 even went 62%→100%). Two caveats: (1) MFC defines
`STRICT`, so a loose `AnyPtr*` passed to `PostMessageA`/`SendMessageA(HWND,…)` needs an
`(HWND)` reinterpret on the genuine-handle value (byte-neutral); (2) if the TU has a
LOCAL declaration-only `struct CMulti`/`CGrunt` proximity-host, delete it (or qualify
`::CMulti`) so plain `CMulti` resolves to the real class. Broke ApiCallers + ReconBatch2
(killed SelOwner_0c50f0 / ReplayModel_64bd5c / PtrList_bf580 / OptCfg offset-cast), all
touched fns held or improved.
