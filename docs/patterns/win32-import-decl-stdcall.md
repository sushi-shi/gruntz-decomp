# Win32 imports: a minimal `__declspec(dllimport) __stdcall` block (NOT `#include <windows.h>`) → `FF15 [IAT]`; cl caches a repeated IAT slot
tags: cpp:import asm:call | topic:codegen-idiom topic:flags
symptoms: spurious `add esp,N` after a USER32/KERNEL32/VERSION call (callee-cleaned in reality), or the wrong call shape, or entropy churn from header bloat
confidence: 9/10

Declare each used Win32 import as a minimal `__declspec(dllimport) <ret> __stdcall Name(args);`
(plus its struct, e.g. `MSG`). This emits the `FF15 [__imp__Name]` indirect form, reloc-masked.
Two load-bearing details:

- **`__stdcall` is mandatory** — declared plain (cdecl) the call site emits a spurious
  `add esp,N` the callee actually cleaned (VERSION.DLL trio, PostMessageA, the USER32 set), and
  arg interleave breaks. `__declspec(dllimport)` gives `FF15 [IAT]`; non-dllimport SDKs (DirectX)
  give the `FF25 [IAT]` thunk + `E8` form.
- **Do NOT `#include <windows.h>`** — keep the visible symbol SET small; the compiler hashes the
  symbol table into codegen, so entropy follows header churn (see tu-completeness-rva-order /
  orchestration §2a).
- **Same dllimport called N× in one body** → cl caches the IAT slot in a reg once
  (`mov edi,ds:[__imp]; call edi`), not N× `FF15 [IAT]` — free if you just call it N times.

STEERABLE. Evidence: WinMain VERSION.DLL trio __stdcall 93.8→95.9%; RunMessageLoop PeekMessage/
Translate/Dispatch/TranslateAccelerator (4 IAT, 100%); CNetMgr PostMessageA; ShowError LoadStringA/
ShowCursor/DialogBoxParamA. Trust the reloc symbol + arg-count over the decoder's guessed import
name (RunMessageLoop: the "GetMessageA" hint was wrong; the 5-arg push = PeekMessageA PM_REMOVE).
