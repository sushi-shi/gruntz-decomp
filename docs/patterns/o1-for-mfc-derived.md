# MFC-derived (CFile-clone) code is built `/O1` (optimize SIZE), not the locked `/O2` — flag tell
tags: cpp:mfc | asm:push asm:mov | topic:flags topic:codegen-idiom
symptoms: an MFC-shaped function plateaus 60s-80s% under /O2 no matter the source; pushes pre-load to a reg, omits ebp frames on address-taken locals
confidence: 8/10

MFC shipped compiled `/O1` (optimize for SIZE), so engine classes derived from MFC (`CFile`
clones, `CObject`-derived) byte-match only under `/O1`, not the project-locked `/O2`. /O2 (favor
speed) plateaus these at 60s-80s%; /O1 takes them byte-exact. Two TARGET-observable tells of /O1
vs /O2:

1. **pushes memory operands directly** (`push [ecx+4]`) where /O2 pre-loads to a reg
   (`mov eax,[ecx+4]; push eax`).
2. **keeps ebp frames for functions that take the address of a local/param** (`push ebp; mov
   ebp,esp` + `[ebp+N]`); /O2 omits the frame, uses esp-relative even for escaped locals.

Per-FUNCTION: a fn with no address-taken local is frameless under BOTH — so this is an opt-LEVEL
choice (`/O1` unit), NOT a global `/Oy-` (which would wrongly force a frame on Seek too). The
address-of-a-LOCAL is what tips the /O1 frame heuristic; `&param` reused as an out-slot may not.

STEERABLE (unit `flags = "o1"`-style profile). Evidence: CFileIO (CFile clone) — Open/Read/Write
got the frame at /O1, byte-exact; /O2 plateaued. Pure-CString getters elsewhere are /O2 (args-in-
regs, no ebp frame — the /O2 tell), so opt-level is per-TU by code provenance.
