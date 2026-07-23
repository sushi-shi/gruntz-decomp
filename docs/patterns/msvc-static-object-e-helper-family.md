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

Do not test only the name recognizer. The first implementation classified
`_$E28` as an `$E` family but excluded executable sections from the definition
walker, so every helper remained unnormalized while the test stayed green.
The gate now proves that both VC5's local TEXT definition and the delinker's
external TEXT definition enter the content-addressing pipeline. The normalized
sidecar must show `family=e`, `storage=text`, a content-derived canonical name,
and a non-zero meaningful size; `skipped-undefined` means the contract is still
broken.

TEXT identity also excludes trailing NOPs and uses the meaningful body length,
not the containing allocation span. VC5 puts a 26-byte helper in its own
32-byte COMDAT while the delinker may pack the same body into a 28-byte span;
including either padding run or physical span in the digest makes identical
code hash differently. Data identities still retain their physical extent.

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

Evidence: `NetLobby::g_str649618` emits all three helpers automatically, and
replacing the hand-written constructor twin with `RVA_COMPGEN` pins recovered
the family at `0x000bd7f0`, `0x000bd810`, and `0x000bd830`. The 14-byte middle
helper had previously been mislabeled as LOW `__inc`.

The original documentation called all three exact, but that was a false result:
the normalizer recognized `$E` names without ever processing TEXT definitions.
After the full path was fixed, the family exposed unresolved relocation
identity and correctly stopped pairing. Keep that dip as a reverse-audit target;
the real object model is supported, but exactness requires the ordered data and
callee relocations to agree too.

The first full run exposed ten previously credited exact functions. Their
meaningful instruction bytes have the expected helper shapes, but the
content-addressed identities differ because their ordered relocation identities
do not yet agree:

| TU | Retail helpers removed from exact count |
| --- | --- |
| `AreaMgr` | `0x00099b80`, `0x00099be0`, `0x00099c00` |
| `GameText` | `0x00018740`, `0x00082990` |
| `CImage` | `0x00153800`, `0x001538b0` |
| `NetLobbyDialogs` | `0x000bd7f0`, `0x000bd810`, `0x000bd830` |

This is a relocation/model audit queue, not evidence that the normalizer should
fall back to private names. `match.status` reported the first AreaMgr helper as
the vanished semantic name `TokenMgrReset` and the first NetLobby helper as
`InitPlayerNameStr`; those names had occupied the RVAs before the private helper
identity was exposed and must not be read as separate losses. ButeMgr's two
one-byte helpers also rebound from `_$E48`/`_$E50` to their shared
content-addressed identity, but remained exact at the module level and therefore
did not contribute to the ten-function dip.
