# A lone `mov [stream+4],1` next to an `fld dword`/`fstp qword` pair is `ostream::operator<<(float)`

tags: cpp:call cpp:cast cpp:float | asm:mov asm:fld asm:fstp | topic:codegen-idiom
symptoms: `mov DWORD PTR [eax+0x4],0x1` on the value an `operator<<` just returned;
`fld DWORD PTR [esp+N]` then `fstp QWORD PTR [esp]`; an unexplained store into an
`ostream` before a call to `??6ostream@@QAEAAV0@N@Z`; `x_floatused`
confidence: 10/10 (the store disappears/appears exactly with the cast; three sites)

## The shape

MSVC 5.0's `<ostream.h>` declares `operator<<(double)` out of line but
`operator<<(float)` **inline**, and the inline has a side effect:

```cpp
// MSVC 5.0 OSTREAM.H:120
class _CRTIMP ostream : virtual public ios {
    ...
private:
    ostream& writepad(const char *, const char *);
    int x_floatused;                 // ostream's ONLY data member -> offset +4
};

inline ostream& ostream::operator<<(float _f)
    { x_floatused = 1; return operator<<((double) _f); }
```

So streaming a `float` emits **two** things: a store of `1` into the stream at
`+4` (`+0` is the virtual-base pointer), and then the ordinary
`operator<<(double)` call with the widened value:

```asm
; CButeMgr's ButeGroup_Apply, BUTE_FLOAT arm @0x17131c
17131c: mov  edx,[edi+0x4]            ; value->pValue
17131f: push 0x624198                 ; s_strFloat
171326: mov  eax,[edx]                ; the float BITS, staged as an int
171328: mov  [esp+0x10],eax           ;   ... into a dead param slot
17132c: call 0x16be60                 ; operator<<(const char*)
171331: fld  DWORD PTR [esp+0xc]      ; reload the float
171335: sub  esp,0x8
171338: mov  ecx,eax                  ; chain onto the returned stream
17133a: mov  DWORD PTR [eax+0x4],0x1  ; <-- x_floatused = 1   (the tell)
171341: fstp QWORD PTR [esp]          ; widen to double, in place
171344: call 0x191df0                 ; operator<<(double)
```

## Why you will write the wrong source

`operator<<(double)` is the symbol the disassembly names, so the natural
transcription is a hand-widened argument:

```cpp
output << s_strFloat << static_cast<double>(scalar);   // WRONG - drops the store
```

That binds `operator<<(double)` directly, the inline never runs, and the
`mov [eax+4],1` vanishes. The fix is to stop widening and let overload
resolution pick the float inline:

```cpp
float scalar = *static_cast<float*>(value->pValue);
output << s_strFloat << scalar;                        // RIGHT
```

Same for a `float`-returning call: `stream << GetFloat(tag, key);`, never
`stream << (double)GetFloat(tag, key)`.

## Reading it the other way

The store is also a **type oracle**. If retail streams a value and you see the
`+4` store, the expression's static type is `float`, not `double` — even when the
only call is to the `double` overload. Its absence beside an `fstp QWORD` says
the value really was a `double` (or a hand-widened float, which is what this
pattern is about). In `ParseAttributeFile` the two `BUTETOK_KEYWORD_FLOAT` /
`BUTETOK_FLOAT_SUFFIX` arms carry the store and the three `BUTETOK_VECTOR` /
`BUTETOK_RANGE` / `BUTETOK_DOUBLE` arms do not — which is exactly the
`GetFloat` vs `GetDouble` split.

Measured: `ButeGroup_Apply` 86.95 -> 87.70 and `ParseAttributeFile` 76.49 ->
77.14 with no other change (the arm-order fix below landed in the same build).

## The neighbouring tell: arm bodies are laid out in SOURCE order

`ButeGroup_Apply`'s jump table at `0x1714b4` routes index 2 to `0x17134c` (the
`double` body) and index 3 to `0x17131c` (the `float` body) — the **table** is in
enum order but the **bodies** are not. cl emits `case` bodies in the order they
appear in the source, so retail's source writes `case BUTE_FLOAT:` *before*
`case BUTE_DOUBLE:`. Whenever a jump table's targets are not monotonically
increasing, read off the real source order and reorder your `case` labels to
match; nothing else reproduces it.

## Related

[`x87-copypaste-vs-inline-fp-block`](x87-copypaste-vs-inline-fp-block.md),
[`inlined-mfc-accessors-transcribed-as-raw-offsets`](inlined-mfc-accessors-transcribed-as-raw-offsets.md)
(the same "a library header inline was transcribed as its out-of-line callee" mistake).
