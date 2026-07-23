# File-scope object emits a compiler-private `$E<n>` constructor/atexit/destructor family
tags: cpp:ctor cpp:dtor cpp:static | asm:push asm:call asm:jmp | topic:identity topic:tooling
symptoms: an isolated 14-byte body does `push <adjacent helper>; call atexit; add esp,4; ret`; nearby helpers tail-jump to a constructor/destructor on one global object; FID calls the 14-byte body `__inc`; source hand-writes only the constructor helper while also defining the real file-scope object
confidence: 10/10

A real file-scope object with a non-trivial constructor or destructor makes MSVC
5 emit a family of compiler-private `_$E<n>` functions:

- a constructor helper that loads the global as `this` and tail-jumps its ctor;
- a 14-byte helper that passes the destructor helper to `atexit`;
- a destructor helper that loads the same global and tail-jumps its dtor;
- a combined initializer referenced by `.CRT$XCU`.

The numeric suffix is a per-object counter and changes when the TU's emitted
symbols change. Do not turn these into stable developer-written names, and do
not keep a hand-written constructor twin beside the compiler-emitted one. Define
the real global object and pin the retail helpers with distinct placeholder
names:

```cpp
DATA(0x00......)
CString g_name;
RVA_COMPGEN(0x000bd7f0, 0xa, _$E776176)
RVA_COMPGEN(0x000bd810, 0xe, _$E776208)
RVA_COMPGEN(0x000bd830, 0xa, _$E776240)
```

`canonicalize_data_symbols.py` recognizes both `$E28` and the x86 COFF spelling
`_$E28`, then pairs base and target by normalized bytes plus ordered relocation
targets. The placeholder number therefore has no identity meaning; the content
and relocations select the emitted helper.

This is an acknowledged annotation-contract weakness. Names such as `_$E776208`
are globally unique RVA-derived placeholders, not semantic identities, and the
compiler's real private symbols can be renumbered or reordered by the
compiler. A future dedicated annotation should express the helper role
(constructor, atexit registrar, or destructor) and its owning global, then resolve
the emitted private symbol mechanically. Until that exists, never treat a
placeholder suffix as recovered source identity.

Audit the whole family, not only the visible 14-byte function. The address pushed
into `atexit` proves the destructor-helper boundary and global owner. The
constructor and destructor helpers prove the global's actual type. If the base
object does not emit a matching `$E` family, the source still has a storage or
class-model defect—commonly an `extern` plus `DATA_SYMBOL` standing in for the
real global—and a pin alone would hide that defect.

Evidence: `NetLobby::g_str649618` emits all three helpers automatically.
Replacing the hand-written constructor twin with `RVA_COMPGEN` pins recovered
`0x000bd7f0`, `0x000bd810`, and `0x000bd830` as exact compiler-generated
functions. The 14-byte middle helper had previously been mislabeled as LOW
`__inc`.
