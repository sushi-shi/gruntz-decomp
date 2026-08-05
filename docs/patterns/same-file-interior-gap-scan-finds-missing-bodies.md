# Scan for CODE GAPS between consecutive same-file claims — that is where a TU's missing bodies are

tags: topic:tooling topic:wall | cpp:method
symptoms: a TU is "complete" by every function-count check yet several of its functions sit
  on an unexplained scratch-register rotation; Ghidra's function list shows nothing missing;
  the exe map shows the contribution contiguous
confidence: 10/10

Ghidra does not carve every retail function — small accessors, forwarders and
compiler-generated thunks are routinely missing from `functions.csv`, so
"unclaimed RVAs in the span" computed against it UNDERCOUNTS badly. The reliable
census is purely address arithmetic over our own claims:

1. sort every `RVA()`/`RVA_COMPGEN()` claim tree-wide by address;
2. for each adjacent PAIR THAT COMES FROM THE SAME FILE, take `[a+size, b)`;
3. strip leading/trailing `0x90`/`0xcc`;
4. anything left is a body retail emitted inside that TU's contribution and we never claimed.

The same-file restriction is what makes it trustworthy: both neighbours are proven to
belong to the unit, so the gap cannot be another TU's contribution.

On `src/Bute/SymTab.cpp` this turned up **nine** bodies where the Ghidra-based scan found
four, all of them reconstructable from the raw bytes in minutes:

| gap | body |
|---|---|
| 0x139a00 | `if (m_owner->m_mappedBuf) return 1; return m_buffer != NULL;` |
| 0x139a20 | `return ReadAt(dst, 0, m_length);` |
| 0x139bc0 | `return (u32)m_cursor >= m_length;` (`cmp/sbb/inc`) |
| 0x139bd0 | `char c; Read(&c, 1, -1); return c;` |
| 0x13a2a0 | `return m_symbols.FindInt(key);` (`add ecx,0x40` = the member's address) |
| 0x13ba50 | a four-argument setter over `[ecx+0x70..0x7c]` |
| 0x13c010 | `return GetRoot()->FindQualified(name);` (push arg, evaluate receiver, call) |
| 0x139c70, 0x139ec0 | `jmp CHashBase::RemoveAll` — cl's out-of-line copies of two inline hash dtors |

Reading them is mechanical: `mov eax,[ecx+N]` is a member off `this`; `[esp+4]` is the
first argument; `ret N` gives the argument count; a lone `e9` is a tail call and its target
names the callee; `add ecx,N` before a call is a member sub-object's method.

The two thunks are worth their own note: an inline destructor whose whole body is one call
(`~CHash() { RemoveAll(); }`) gets an out-of-line COMDAT emitted alongside the inlined
copies, and it lands next to the function whose member-init list needs it for unwinding.
Our obj already emitted `??1CHash@@QAE@XZ` and `??1CHashB@@QAE@XZ` — they were simply never
pinned, so the delinker never carved them. `RVA_COMPGEN` binds them with no source change.

Tree-wide the scan reports ~190 same-file interior gaps. Most are the repeated 143-byte and
698-byte library bands, but the tail is real: small accessors, forwarders, `??_G` scalar
deleting destructors, and static-object destructor thunks (`mov ecx,<static>; jmp ~CString`,
emitted for a function-local `static CString` and referenced by the magic-static block that
registers it with `atexit`).

related: rva-extent-must-include-switch-tables.md, missing-bodies (gruntz.audit),
comdat-inline-ctor-no-standalone.md
