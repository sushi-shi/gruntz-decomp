# Compiler ordinals must not alias authored static names

tags: cpp:static | asm:coff asm:relocation | topic:tooling topic:identity
symptoms: restoring a short original static name suddenly makes unrelated literal data appear to miss the same retail relocation
confidence: 10/10 for the canonicalization and consumer controls

Restoring an original file-static datum named `_` exposed a resolver collision.
Its COFF/model spelling `__$S` correctly normalizes to `__`. However, the same
suffix stripper changed anonymous compiler labels such as `_$S56` into `_`.
An underscore-tolerant lookup then assigned every such unrelated literal the
authored datum's address. The full data-relocation audit produced 1,057 false
findings, including a supposed missing reference to the `Global Error: ` string.

The fix preserves whole compiler-generated identities before applying the
named-static suffix rule. It does not special-case the source datum or relax
relocation auditing. Genuine names such as `__$S123` and
`_named$Sdata_data_<digest>_0` still normalize to their authored owners.

Controls cover all three layers:

- `canon()` preserves anonymous `_$S56` and `$S56` identities.
- A real `Resolver` built from synthetic model entries cannot resolve those
  literals to `_`, while genuine named statics retain address/addend behavior.
- A complete synthetic COFF pair passes through `data_relocs.scan`: anonymous
  literals use the paired oracle rather than an invented retail address. A
  named-static negative control still reports the deliberately missing retail
  relocation, proving that the fix does not merely suppress failures.

Reverse use: when many unrelated symbols suddenly resolve to one datum after
a legitimate source rename, trace canonicalization and decoration tolerance
before changing the source. Test the complete consumer path, not just the
regular expression.
