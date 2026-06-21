# A function-local `static T` reproduces MSVC5's one-shot guarded magic-static init
tags: cpp:static cpp:local | asm:test asm:call | topic:codegen-idiom
symptoms: a guard-bit test + conditional ctor + atexit registration around a function-local object the target builds once
confidence: 7/10

A function-local `static AfxString s_empty("");` (or any `static T` with a ctor/dtor) emits
MSVC5's exact one-shot guarded init: `mov al,1; mov cl,[guard]; test al,cl; jne skip; <or guard
bit>; mov [guard]; call T::T(args); push dtorThunk; call atexit; skip:`. Return the static's
address as its `const char*` (the CString object address == its char* @+0). Use this instead of a
hand-rolled guard.

```cpp
static AfxString s_empty("");        // magic-static guard + atexit
return (char*)&s_empty;              // on the error path
```
STEERABLE. Evidence: CButeMgr::GetString function-local `static CString empty` — exact guarded
init; PLATEAU 98% is a tag↔key ebx/edi reg-alloc coin-flip after the static block (entropy-class,
see zero-register-pinning.md), not the static idiom.
