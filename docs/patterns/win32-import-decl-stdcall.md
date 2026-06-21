# Win32 imports: bring them from the real headers (`<Mfc.h>` / `<Win32.h>` → `<windows.h>`) → `FF15 [IAT]`; cl caches a repeated IAT slot
tags: cpp:import asm:call | topic:codegen-idiom topic:flags
symptoms: spurious `add esp,N` after a USER32/KERNEL32/VERSION call (callee-cleaned in reality), the wrong call shape, or a hand-rolled typedef/extern drifting from the real Win32 signature
confidence: 9/10

Win32/MFC **types and imports come from the real toolchain headers**, not hand-rolled
decls. Include `<Mfc.h>` in MFC TUs (it pulls `<afx.h>` → `<windows.h>` the period-correct,
afx-first way) or `<Win32.h>` in pure-Win32/DirectX TUs (`WIN32_LEAN_AND_MEAN` + `<windows.h>`).
These declare every USER32/KERNEL32/GDI32/VERSION import as `__declspec(dllimport) <ret> __stdcall`
— exactly the `FF15 [__imp__Name]` indirect form, reloc-masked — and give you the real `BOOL`/
`HWND`/`UINT`/`HFONT`/`HKEY`/… types and `MSG`/`MODULEENTRY32`/… structs. **Do NOT** re-`typedef`
`BOOL`/`HWND`/`INT_PTR`/… or re-`extern` `PostMessageA`/`timeGetTime`/`GetCurrentDirectoryA`/… by
hand: use the real type/decl so the signature (and its mangling) can't drift. (`INT_PTR` — absent
in VC5 — and the WINMM `timeGetTime` decl live as one central decl inside the umbrellas.)

Two load-bearing details — properties the real `windows.h` decls already have:

- **`__stdcall` is mandatory** — the windows.h imports are `__stdcall`; a (mistaken) cdecl decl
  emits a spurious `add esp,N` the callee actually cleaned (VERSION.DLL trio, PostMessageA, the
  USER32 set) and arg interleave breaks. `__declspec(dllimport)` (what windows.h uses) gives
  `FF15 [IAT]`; non-dllimport SDKs (DirectX) give the `FF25 [IAT]` thunk + `E8` form.
- **Same dllimport called N× in one body** → cl caches the IAT slot in a reg once
  (`mov edi,ds:[__imp]; call edi`), not N× `FF15 [IAT]` — free if you just call it N times.

**Pulling `<windows.h>` via the umbrellas is matching-NEUTRAL.** The real-MFC-headers migration
(PR #44, `use-real-mfc-headers`) replaced every hand-rolled Win32 typedef/extern with `<Mfc.h>`/
`<Win32.h>` and stayed **247/816 exact, no regressions** — `afx.h` already pulls the full windows.h
symbol set into MFC TUs, so the old "keep the symbol SET small / do NOT include windows.h" fear did
not materialize. Symbol-set entropy is still real for *gratuitous/ad-hoc* churn (see
`matching-patterns.md` mitigations), but the standard umbrellas are the convention, not a churn
source. One thing to verify per-site: a member/return whose real type forces a *different* MFC base
(e.g. `CDWordArray` vs a typed `CTypedPtrArray<CPtrArray,…>`) can shift codegen — check the ctor
match before assuming a type swap is free (GameLevel: a typed array dropped the ctor 89.5%→72%, so
its array is a genuine `CDWordArray` and the pointer↔DWORD casts are the devs' real shape).

STEERABLE. Evidence: WinMain VERSION.DLL trio __stdcall 93.8→95.9%; RunMessageLoop PeekMessage/
Translate/Dispatch/TranslateAccelerator (4 IAT, 100%); CNetMgr PostMessageA; ShowError LoadStringA/
ShowCursor/DialogBoxParamA; GameMode `PostMessageA` pulled from the real `<windows.h>` with an `HWND`
owner member (matching-neutral). Trust the reloc symbol + arg-count over the decoder's guessed import
name (RunMessageLoop: the "GetMessageA" hint was wrong; the 5-arg push = PeekMessageA PM_REMOVE).
