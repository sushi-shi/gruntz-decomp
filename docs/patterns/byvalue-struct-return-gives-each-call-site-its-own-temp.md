# Two calls to one out-param helper using TWO different temp slots means it returns BY VALUE

tags: cpp:return cpp:param cpp:local | asm:push asm:lea asm:call | topic:codegen-idiom

symptoms: a `T* f(T* out, ...)` helper is byte-exact on its own, but a caller that calls
  it twice is one `lea` off - retail passes `lea eax,[esp+0xc]` at the first call and
  `lea eax,[esp+0x18]` at the second where the source has ONE named `T tmp;` and passes
  `&tmp` both times; the caller's frame has one MORE aggregate slot than the source
  declares

confidence: 9/10

variants: out-param-helper-returns-the-pointer.md

[`out-param-helper-returns-the-pointer.md`](out-param-helper-returns-the-pointer.md)
recovers the first half of MSVC's struct-return lowering: a `void f(T* out)` whose
retail form keeps the out-pointer in eax across every `ret` is really
`T* f(T* out) { ...; return out; }`. That is still the LOWERED form. When the same
helper is called more than once in one function, the CALLER proves the source-level
signature is a plain by-value return:

```cpp
T f(...);            // cl passes a hidden pointer to a per-call-site temporary,
                     // pushed FIRST (so it appears last on the stack), and returns
                     // it in eax; the callee ends `*out = local; return out;`
```

Each *call site* gets its **own** hidden temporary. A hand-modelled out-param
signature forces every site to share one named `tmp`, so the caller's frame is one
aggregate short and the second `lea` points at the wrong slot.

## The tell, in the caller

```asm
; retail CShadeTableCache::CompareHue, frame `sub esp,0x24` = 3 x sizeof(ColorHSV)
lea    eax,[esp+0xc]        ; hidden temp #1
push   ecx                  ; the real argument
push   eax
call   RgbToHsv
mov    ecx,[eax]            ; memberwise copy of the RESULT into `ha` at [esp+0x0]
...
lea    eax,[esp+0x18]       ; hidden temp #2  <-- a DIFFERENT slot
push   ecx
push   eax
call   RgbToHsv
```

Slot 0 is `ha`, slot 1 is `hb`, slot 2 is the second call's temp - and the FIRST
call's temp is slot 1, reused for `hb` after it dies. Three slots, two named
variables: the extra one is the second hidden return temp, and it only exists if
each call site has its own.

## Fix

Declare the by-value return and drop the out-parameter and the shared temp:

```cpp
// header
ColorHSV RgbToHsv(u32 color);

// definition - unchanged body; `return hsv;` is what cl lowers to `*out = hsv; return out;`
ColorHSV RgbToHsv(u32 color) { ColorHSV hsv; ...; return hsv; }

// caller
ColorHSV ha, hb;
ha = RgbToHsv(...);
hb = RgbToHsv(...);
```

The definition's own bytes do not move (cl re-emits the identical hidden-pointer
prologue and `*out = hsv; return out;` epilogue), and `RVA()` binds through our own
mangled name on both sides, so `?RgbToHsv@@YAPAUColorHSV@@PAU1@I@Z` ->
`?RgbToHsv@@YA?AUColorHSV@@I@Z` re-binds transparently.

Note the assignment form (`ha = f(...)`, not `ColorHSV ha = f(...)`): the initialiser
form lets cl construct directly into `ha` and the temp disappears again. Retail's
memberwise copy out of the temp into the named variable is the proof that the source
assigned rather than initialised.

Evidence: `CShadeTableCache::CompareHue` @0x14fa60 - 86.71 -> **100.00 EXACT** on the
signature change alone (the shared `ColorHSV tmp;` was the whole residue);
`RgbToHsv` @0x14fcc0 stayed 100.00 under the new signature.
