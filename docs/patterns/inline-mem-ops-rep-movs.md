# `strcpy`/`memset`/struct-copy inline to `rep movs`/`rep stos` at `/O2 /Oi` — write the plain call, no `call _strcpy`
tags: cpp:builtin asm:rep | topic:codegen-idiom
symptoms: target has `rep movsd; rep movsb` / `repne scasb` / `rep stosd` where you expected a `call _strcpy`/`memset`; an N-dword struct zero-init
confidence: 8/10

At `/O2 /Oi` MSVC5 inlines the CRT string/mem intrinsics — there is no `call _strcpy`/`_memset`:

- **`strcpy(dst,src)`** → `repne scasb` (inline strlen) then `rep movsd; rep movsb`. A
  `strcpy` followed by `strlen(dst)` re-scans — write `strcpy(dst,src); n = strlen(dst);` plainly.
- **`rep stosd` of N dwords** = zero-init of an N-dword struct (WNDCLASSA = 10 dwords) — model as
  an N-iteration `int*` zero loop or a `memset`.
- **struct copy** (`*pDst = *pSrc` for a fixed-size struct) → `rep movs` of the dword count.

Write the plain, natural call/assignment; do NOT hand-roll the rep sequence or call out to a CRT
symbol. (Requires `/Oi`, which `/O2` implies in this toolchain.)

STEERABLE. Evidence: WwdFile::CheckHeader (`repne scasb; rep movsd; rep movsb` strcpy);
CGameApp::VirtualUnknownMethod03 (`rep stos` 0x75-dword GameInfo memset, 3 inline strcpys);
CGameLevel::LoadWwd (`rep movs` 0x17d-dword header copy + strcpy of levelName).
