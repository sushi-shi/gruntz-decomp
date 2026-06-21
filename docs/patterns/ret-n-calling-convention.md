# Pick the calling convention by the `ret N` in the target — `ret` vs `ret N` decides cdecl/stdcall/thiscall
tags: cpp:method cpp:static asm:ret | topic:codegen-idiom topic:archetype
symptoms: wrong/extra stack cleanup at call sites; a static member mangled `@@SA` (cdecl, `ret`) where the target callee-cleans (`ret N`, `@@SG`)
confidence: 8/10

The epilogue `ret` form tells you the convention, which fixes both callee bytes and every call
site's cleanup:

- `ret` (c3) = `__cdecl` / void-arg, or a `__thiscall`/`__stdcall` with no stack args.
- `ret N` = callee-cleanup (`__stdcall`/`__thiscall`) popping N bytes of stack args.
- A **`static` member** with `ret N` is `static … __stdcall` → mangles `@@SG…`; default `static`
  is `__cdecl` → `@@SA…`, `ret`. Match the `ret` to choose the `S?` letter.

```cpp
static long __stdcall WndProc(void*,UINT,UINT,long);   // ?WndProc@C@@SGJ... , ret 0x10
```
STEERABLE. Evidence: CGruntzApp::ErrorDialogProc `SG` (ret 0x10) not `SA`; CGameApp::
GameWindowProc `SGJ…`; the L1705 ret-N one-liner. The dual of this for the IGNORES-`this` case is
thiscall-ignoring-this.md (callee bytes alone don't disambiguate — read the caller's `mov ecx`).
