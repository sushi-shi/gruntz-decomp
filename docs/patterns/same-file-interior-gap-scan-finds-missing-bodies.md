# Scan for CODE GAPS between consecutive same-file claims — that is where a TU's missing bodies are

tags: topic:tooling topic:wall | cpp:method
symptoms: a TU is "complete" by every function-count check yet several of its functions sit
  on an unexplained scratch-register rotation; Ghidra's function list shows nothing missing;
  the exe map shows the contribution contiguous
confidence: 10/10

An analyzer-derived function inventory does not necessarily carve every retail
function — small accessors, forwarders and compiler-generated thunks can be
missing, so "unclaimed RVAs in the span" computed against it UNDERCOUNTS badly.
The reliable census is purely address arithmetic over our own claims:

1. sort every `RVA()`/`RVA_COMPGEN()`/`RVA_DYNINIT()` claim tree-wide by address;
2. for each adjacent PAIR THAT COMES FROM THE SAME FILE, take `[a+size, b)`;
3. strip leading/trailing `0x90`/`0xcc` and split internal padding runs whose
   following byte is 16-byte aligned;
4. anything left is a body retail emitted inside that TU's contribution and we never claimed.

The checked-in read-only view performs that derivation directly:

```sh
gruntz sema gaps
gruntz sema gaps --class substantive
gruntz sema gaps --unit directsoundmgr
```

Its classes are navigation aids, not exclusions. A `thunk`, `trivial`, or repeated
compiler/runtime `band` is still emitted code and remains in the reconstruction queue
until a source or compiler-generated claim covers it.

The same-file restriction makes the **interval** trustworthy, not its ownership. Before a
fragment enters the queue, subtract every existing Model claim, including static-library
and zlib providers. The linker can interleave COMDAT/library contributions between two
ordinary source claims: 0x21280 lies between `ChatBoxOwner.cpp` claims but is the already
claimed HIGH-confidence LIBCIMT body `streambuf::out_waiting`.

The same caution is mandatory for a body reached from `.CRT$XC`; the linker may place an
initializer thunk/body between ordinary claims from another TU. `gruntz sema gaps`
therefore recognizes XC entries from the retail table and attributes them from the relocated
datum the body initializes, never from the neighbours.
The 0x17d60 run is the negative control: it sits between `BattlezDlgColors.cpp` claims but
writes the `customleveldlg` copies at 0x229e08..0x229e88.

On `src/Rez/RezArchive.cpp` this turned up **nine** bodies where the Ghidra-based scan found
four, all of them reconstructable from the raw bytes in minutes:

| gap | body |
|---|---|
| 0x139a00 | `if (m_directory->m_preloadedData) return 1; return m_loadedData != NULL;` |
| 0x139a20 | `return ReadAt(destination, 0, m_size);` |
| 0x139bc0 | `return (u32)m_cursor >= m_size;` (`cmp/sbb/inc`) |
| 0x139bd0 | `char value; Read(&value, 1, -1); return value;` |
| 0x13a2a0 | `return m_types.FindTypeByTag(typeTag);` (`add ecx,0x40` = the member's address) |
| 0x13ba50 | a four-argument setter over `[ecx+0x70..0x7c]` |
| 0x13c010 | `return GetRootDirectory()->FindEntryByPath(path);` (push arg, evaluate receiver, call) |
| 0x139c70, 0x139ec0 | `jmp CHashBase::RemoveAll` — cl's out-of-line copies of two inline hash dtors |

Reading them is mechanical: `mov eax,[ecx+N]` is a member off `this`; `[esp+4]` is the
first argument; `ret N` gives the argument count; a lone `e9` is a tail call and its target
names the callee; `add ecx,N` before a call is a member sub-object's method.

The two thunks are worth their own note: an inline destructor whose whole body is one call
(`~CHash() { RemoveAll(); }`) gets an out-of-line COMDAT emitted alongside the inlined
copies, and it lands next to the function whose member-init list needs it for unwinding.
Our obj already emitted `??1CHash@@QAE@XZ` and `??1CHashB@@QAE@XZ` — they were simply never
pinned, so the delinker never carved them. `RVA_COMPGEN` binds them with no source change.

On the 2026-08-19 tree the first edge-only scan reported 90 gaps and 6,958 bytes, but that
was not a function-level queue: eight apparent compiler/runtime bands and several small
rows each contained multiple padding-separated bodies. Splitting those boundaries gives
237 executable fragments and 3,535 meaningful bytes. After subtracting existing non-source
Model claims and attributing four new helpers/bodies, the remaining 229 split structurally
into 96 XC entry thunks, 72 XC bodies, 22 trivial source candidates, and 39 substantive
source candidates. The generated rows already have source in the owning constructed data;
they need compiler-generated attribution, while the 61 source candidates still need
identity review and, where not another provider, declarations and bodies. That source tail
contains small accessors, forwarders,
`??_G` scalar deleting destructors, and static-object destructor thunks
(`mov ecx,<static>; jmp ~CString`, emitted for a function-local `static CString` and
referenced by the magic-static block that registers it with `atexit`).

The `RVA_DYNINIT` input is load-bearing. Running the same scan without owner-side
dynamic-initializer pins produces 132 rows and 13,590 bytes, manufacturing 42 gaps in
initializer territory. Treat that count as a broken census, not new missing code.

related: rva-extent-must-include-switch-tables.md, missing-bodies (gruntz.audit),
comdat-inline-ctor-no-standalone.md
