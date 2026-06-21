# Subsystem `GetErrorString`/`ReportError` is a `static __cdecl`, not a `__thiscall` method
tags: cpp:method cpp:static cpp:switch | asm:push asm:call | topic:codegen-idiom topic:archetype
symptoms: call site `push×3; call; add esp,0xc; ret`, Ghidra mislabels void __thiscall(void), big HRESULT switch
confidence: 9/10

The DX/Net manager error formatters (`CDirectDrawMgr`/`DirectInputMgr2`/`DirectSoundMgr`/
`CNetMgr`) are **static caller-cleaned cdecl** members that ignore `this`, taking the call
site's `__FILE__`, `__LINE__` and the HRESULT. Ghidra mislabels them `void __thiscall(void)`;
the call site (`push×3; call; add esp,0xc` then a plain `ret`) is the tell. Declaring it
`static` (mangled `?GetErrorString@Class@@SA…`) is THE lever that matches the caller-cleaned
cdecl. Cast every `case` label to `(int)` so clang accepts the >LONG_MAX DDERR/DIERR/DSERR
hex (MSVC5 doesn't care). CNetMgr::ReportError additionally has a 4th `HWND` owner arg.

```cpp
static void Class::GetErrorString(char *file, int line, long hr);  // ?…@@SAXPADHJ@Z
```
STEERABLE → 100% (when the switch is a cmp/je tree; see switch-cmpje-tree-vs-jumptable).
Evidence: DirectInputMgr2/DirectSoundMgr GetErrorString 100%; CDirectDrawMgr 96% (jumptable); CNetMgr::ReportError 100%.
