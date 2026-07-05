# Large sparse switch: case DENSITY flips a single byte-index packed table into a binary-search tree
tags: cpp:switch | asm:jmp asm:cmp asm:lea | topic:codegen-idiom

MSVC5 lowers a big `switch` over a wide, moderately-dense value range one of two ways,
and the choice is a *density threshold* on the case set — NOT the source structure:

- **Packed byte-index table** (retail, dense enough):
  ```asm
  lea eax,[edi-0x8005]      ; bias to 0
  cmp eax,0x1d0             ; range check (464 slots)
  ja  default
  xor ecx,ecx
  mov cl,byte ptr [eax+T1] ; T1 = 465-byte index table (.rdata)
  jmp dword ptr [ecx*4+T2] ; T2 = distinct-target dword table (.rdata)
  ```
- **Binary-search tree of small jump tables** (just-below threshold):
  ```asm
  cmp edi,0x8021           ; bisect
  jg  hi
  je  ...
  lea eax,[edi-0x8005]
  cmp eax,0xa              ; dense low cluster only
  ja  default
  jmp dword ptr [4*eax+T]  ; small dword table for one cluster
  ```

The trigger is subtle: **a couple of missing/extra case labels can cross the threshold.**
`CGruntzMgr::HandleCommand` (0x862f0, 117 outer cases over 0x8005..0x81d5 = 0x1d0):
reconstructed with **115** of the 117 cases → MSVC emitted the TREE (5.4%-style whole-body
misalignment, capped ~30%). Adding the two missing labels (0x806b, 0x80d7) → MSVC emitted
retail's packed byte-index table, and the whole dispatch skeleton + case-body block re-aligned
(+6% instantly, unblocking the rest).

**How to diagnose/fix (fast, decisive):** extract the retail tables from the EXE
(`mov cl,[eax+T1]` gives T1; `jmp [ecx*4+T2]` gives T2) — the T1 byte table is the exact
per-value → dword-index map, i.e. the complete case set and which cases share a body. Then
compile a **minimal probe** — `int f(int x){switch(x){ case ...: return k; ...}}` with the
exact retail case-value set under the same flags (`/O2 /MT`) — and `llvm-objdump -d`:

- probe with retail's full set → **packs** (`cmp $0x1d0; movb (%eax),%cl; jmpl *(,%ecx,4)`).
- probe with your (short-by-2) set → **trees** (`cmp $0x8021; jg; je; ... jmpl *(,%eax,4)`).

That isolates the flip to the case SET (not the nested-`default` switch, not body content,
not case order — all verified matching-neutral via probes). The T1 table is also the
ground-truth for de-rotating mislabeled bodies (each dword-index → body RVA → physical order).

Residual after the flip is the usual megafunction frame/regalloc wall
(`sub esp,N` + a 3rd callee-saved `push ebx` follow the WHOLE body's pressure;
see megafunction-cached-locals-vs-reload-regalloc.md).
