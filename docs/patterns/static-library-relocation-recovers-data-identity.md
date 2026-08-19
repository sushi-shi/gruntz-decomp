# A pinned archive relocation recovers an unlabelled library datum

symptoms: a touched data-coverage gap is referenced only by an admitted
NAFXCW/LIBCMT/LIBCIMT function; the retail instruction has an absolute data
operand, but neither adjacent retail datum owns the bytes

The retail executable has no symbols, but the pinned VC5 static libraries are
the original compilation artifacts for admitted library functions.  For a
retail operand at `site` in a function beginning at `fn`, find the same
function symbol in its archive member and inspect the COFF relocation at

```text
archive function symbol value + (site - fn)
```

An `IMAGE_REL_I386_DIR32` record names the original datum.  The four bytes at
the COFF relocation site are its signed addend, so the retail symbol base is
`retail target - addend`.  The defining archive member then proves storage and
physical extent (next symbol or contribution end; a COMMON symbol states its
own size).  `gruntz verify library-data-refs` performs this join for every
touched non-padding row from `data_coverage_gaps.tsv`.

## Controlled evidence

At retail `0x1eafbc`, `operator new` reads an otherwise anonymous four-byte
slot twice.  NAFXCW `afxmem.obj` has DIR32 relocations at the same offsets,
both naming `__pfnUninitialized$S66856`; its `.rdata` definition is exactly
four bytes.  This establishes name, owner, storage, and extent without using
adjacency.

The same pass falsified seven existing function labels.  Seven MFC archive
operators at `0x1d2f84`, `0x1d3786`, `0x1d3886`, `0x1d4122`, `0x1d418c`,
`0x1d41f6`, and `0x1d5302` had all been labeled as the `CByteArray` overload.
Their COFF relocations instead name the corresponding `CMapStringToOb`,
`CDWordArray`, `CByteArray`, `CObArray`, `CObList`, `CStringList`, and
`CStringArray` `CRuntimeClass` records.  Splitting the adjacent archive
`GetRuntimeClass` and runtime-class initializer contributions gives the same
seven identities independently.

## Reverse-use rule

Use this only when the retail function is already proven to come from the
exact pinned archive.  Require the same function-relative relocation site and
type.  If aliases produce different referents, or the archive function is not
byte/identity-proven, report ambiguity; never choose by neighbouring RVA.
