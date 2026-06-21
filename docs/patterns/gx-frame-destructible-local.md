# A destructible stack local (CString/CFileIO/RegistryHelper) forces the `/GX` frame → `flags="eh"`
tags: cpp:eh cpp:local | asm:mov | topic:flags topic:codegen-idiom
symptoms: under `base` flags the whole __except frame + cleanup funclet is MISSING vs target, function ~0% then jumps
confidence: 9/10

If a function holds a stack object with a non-trivial destructor (a `CString` local, a
`CFileIO`, a `Utils::RegistryHelper`) MSVC5 wraps the body in a `/GX` exception-handling frame
to run the dtor on unwind. Compiled under the default `base` (no `/GX`) flags, `cl` ELIDES the
frame entirely → a large structural mismatch. The fix is a unit-level `flags="eh"` profile
(`/O2 /MT /GX`). A function whose target has an EH frame but yours doesn't is the signal.

```toml
[[unit]]  # flags = "eh"  when a CString/CFileIO/RegistryHelper local is in scope
```
STEERABLE (unit flag). Evidence: CheckCdRomRegistry, CGruntzMgr::BuildMoviePath, the CGrunt entrance methods — all need eh.
