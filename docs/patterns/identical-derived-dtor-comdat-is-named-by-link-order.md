# Two byte-identical dtor COMDATs are named by LINK ORDER, never by their bytes
tags: cpp:dtor cpp:inline cpp:eh msvc5:mfc | asm:mov asm:call | topic:identity topic:correctness
symptoms: `??1CGdiObject@@UAE@XZ` claimed at >1 RVA, name-injectivity, library-overlap, two `mov [esi],<vftable>` stores, `$gap_` EH funcinfo referent, `__ehunwindmap$` unnamed
confidence: 10/10

An empty derived destructor of a base with a non-trivial dtor compiles to bytes
IDENTICAL to the base dtor: cl 5.0 deletes the most-derived vptr store as dead
(the inlined base dtor overwrites `[this]` before any call can observe it), so
only the base's own vptr store — the one guarding a real call — survives. Both
COMDATs then carry the same instructions AND the same relocation triple. So a
retail image holding TWO copies of that byte sequence is not a duplicate symbol;
the copies are DIFFERENT symbols, and only COMDAT selection order says which.

```cpp
// MFC: CPen : CGdiObject : CObject, `CPen::~CPen() {}`, `~CGdiObject(){DeleteObject();}`
CPen pen(PS_SOLID, 2, RGB(0, 0, 0));    // the local that forces the COMDAT
```
```asm
; both ??1CPen@@UAE@XZ and ??1CGdiObject@@UAE@XZ, byte- and reloc-identical
  mov  DWORD PTR [esi],OFFSET ??_7CGdiObject@@6B@   ; NOT CPen's vftable
  call ?DeleteObject@CGdiObject@@QAEHXZ
  mov  DWORD PTR [esi],OFFSET ??_7CObject@@6B@
```

Read the identity off the EDGES, not the bytes. The base copy is pinned by the
base's own vtable: `??_7CGdiObject@@6B@+4` -> `??_GCGdiObject@@UAEPAXI@Z` ->
`??1CGdiObject@@UAE@XZ`, which lands in the FIRST TU that defines it (dialogs,
0x016460). Any second copy belongs to the first TU that used a DERIVED class,
and its only referrer is that function's unwind funclet — `lea ecx,[ebp-N]; jmp
<copy>` for the fully-constructed local. fontconfig 0x0220f0 is `??1CPen`, not
`??1CGdiObject`: MeasureLabel's `CPen pen` is its sole referrer, and dialogs (an
earlier obj) already owns the CGdiObject copy. Steerable and load-bearing —
claiming it `RVA_COMPGEN(0x000220f0, 0x46, ??1CPen@@UAE@XZ)` scored 100.0000,
named the last `$gap_` referent image-wide (`__ehunwindmap$??1CPen@@UAE@XZ` at
0x1f97c8), and cleared both the name-injectivity and library-overlap gates that
the ~CGdiObject spelling failed.
