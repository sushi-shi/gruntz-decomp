# MFC `AFXAPI` helpers (CString `operator+`, `Format`) are `__stdcall`, not `__cdecl` — kills the spurious `add esp,N`
tags: cpp:method cpp:mfc | asm:call asm:add | topic:codegen-idiom
symptoms: a spurious `add esp,0xc` (caller-cleanup) after each CString `operator+`/concat call, stack-temp offsets cascade, ~3-5% low across a whole family
confidence: 8/10

The MFC `AFXAPI` free functions — `CString operator+(LPCTSTR, const CString&)` @0x1b9ff5,
`operator+(const CString&, LPCTSTR)` @0x1b9f81, the `CString::Format` wrapper — are `__stdcall`
(`ret 0xc`, callee pops the hidden struct-return slot + both args), NOT `__cdecl`. Declared
plain, the call site emits a spurious `add esp,0xc` after each call (caller-cleanup) and the
by-value-return stack-temp offsets cascade. The by-value class return + `__stdcall` together
give the exact `lea &temp; push arg; push lit; push &temp; call; (no add esp)` shape.

```cpp
AfxString __stdcall operator+(const char *l, const AfxString &r);  // ??H@YA...@Z, ret 0xc
```
STEERABLE. Evidence: CGrunt::Resolve*Animation +~3-5% across all five (Death 95→97, generic
90.9→94.7); CNetMgr config writer (same AFXAPI `operator+` @0x1b9f81, ret 0xc). The `__cdecl`
mangling is `??H@YA…`; pick the convention by the `ret N` (see ret-n-calling-convention.md).
