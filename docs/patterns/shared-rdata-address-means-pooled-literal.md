# Multiple source names at one `.rdata` address mean a pooled literal, not aliased globals
tags: cpp:static cpp:literal | data:rdata data:layout | topic:identity topic:mis-model topic:reloc-fidelity
symptoms: two or more `DATA()` declarations claim the same retail address; each name is referenced only by floating-point comparisons or arithmetic; replacing the names with literals preserves the relocation and can improve code match
confidence: 10/10

MSVC pools identical floating-point constants in `.rdata`. A relocation to that
address proves that the instruction consumes the bytes; it does not prove that
the original source declared a semantic global there.

Treat an address as a pooled literal when all of these agree:

1. Raw retail bytes decode to the expected value.
2. Xrefs use the address only as a floating-point operand.
3. No code takes the address as an object, writes it, or accesses fields around
   it.
4. More than one reconstructed semantic name claims the same retail address,
   or the names are otherwise private aliases for an ordinary literal.

Do not define overlapping globals or preserve one arbitrary alias. Spell the
literal at each use and let MSVC recreate the pool and its relocations.

The Grunt/path-hazard audit proved the pattern:

- RVA `0x001ea400` is the eight-byte encoding of `0.0`; both `g_pathZero` and
  `g_slimeZero` claimed it.
- The adjacent RVAs `0x001ea408` and `0x001ea410` encode `0.03125` and `1.0`.
- RVA `0x001e9738` encodes `0.001` and had one multiplication xref.

Replacing all four names with numeric literals removed four declared-only data
symbols without changing the intended relocations, and
`CPathHazard::BeginLeg` improved from 84.24% to exact.

This is distinct from a real aggregate discovered through repeated field-offset
accesses or static initializer helpers. Nearby addresses and alignment alone do
not decide between literals and a struct; xref shape and writes do.
