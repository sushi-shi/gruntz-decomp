# A tiny inline ctor (bare vptr store) has no source-recoverable standalone COMDAT
tags: cpp:ctor cpp:inline | asm:mov | topic:comdat topic:wall
symptoms: 7-byte target `mov [this],&vftable; ret` with NO `mov eax,ecx` and NO null guard, can't graduate from stub
confidence: 7/10

A retail standalone COMDAT that is a bare `mov dword [ecx], offset ??_7Class@@6B@; ret` (7 bytes,
void, **no** `mov eax,ecx` return, **no** placement-new null guard) is the out-of-line
materialization of a ctor that is INLINED at every real call site. The canonical class cannot
emit it: a real `??0` always appends `mov eax,ecx` (verified /O2 and /O1); placement-new appends
a `test ecx,ecx; je` guard. So there is no source spelling for the standalone form — leave it in
the `src/Stub/` backlog (or use a stand-in TU as for CSBI_RectOnly; orchestration §5).

WALL (no source form). Evidence: CGruntzCommand_0242f0 / _024430 (7-byte stubs, left in backlog).
