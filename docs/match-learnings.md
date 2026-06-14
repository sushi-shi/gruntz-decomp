# Match learnings — running log (campaign)

Per-function / per-subsystem insights gathered while byte-matching. Durable,
generalizable findings graduate into `docs/matching-patterns.md`; this file is the
fast-moving scratchpad. Newest at top within each section.

## How the loop runs (orchestrator + matcher contract)
- Matchers run **sequentially in the main worktree** (the `build/` tree —
  delinkable EXE, ghidra DB, toolchain — is gitignored, so git worktrees can't
  build; and `symbol_names.csv`/`units.toml`/the wine prefix/`build/` are shared).
- Each matcher = one TU = **its own atomic commit** (adds only its own files) +
  `git push`. After each, the orchestrator refreshes the comprehension DB
  (`build/ghidra-named`) when the matcher pinned new names/types, so the next
  matcher stands on prior work ("shoulders of giants"), then re-runs
  `scripts/analysis/gen_match_queue.py` to refill `docs/match-queue.md`.
- Per-function asm diff: `objdiff-cli diff -p build/objdiff -u <unit> <mangled-sym>
  -o - --format json-pretty`. Roll-up: `gruntz build`.

## Subsystem notes
### RezMgr archive container (unit `rezmgr` — 3 byte-exact + 1 plateau; OpenSub deferred)
- New TU `src/Rez/RezMgr.cpp` (+ `.h`). The Monolith "RezMgr Version 1" in-memory
  directory-tree node classes. CONFIRMED the long-@unconfirmed container offsets
  (updated `structure/formats/rez.h`). Built `/O2 /MT /GX` (engine code, not MFC;
  /GX reserved for the deferred OpenSub EH frame — adding /GX did not affect the
  4 fns here, which carry no EH).
- **THREE "CRezDir"-labeled functions are actually THREE DIFFERENT classes** — the
  load-bearing discovery. tomalla's notes call Load/OpenSub/FindEntry/ctor all
  `CRezDir`, but the field OFFSETS conflict, proving distinct layouts:
  - the **0x38 CRezDir ctor** (@0x13c940) uses +0x10/+0x1c as an embedded child
    collection's two vtables (0x5ef7c8), +0x14/+0x18 head/tail;
  - **Load's node** (@0x13a0f0) uses +0x10 as a payload SIZE, +0x18 as the archive-
    source ptr, +0x48 as the loaded buffer / already-loaded gate, +0x38 as ITS
    child collection;
  - **OpenSub's node** (@0x13b0c0) uses +0x10 as a list-append target AND +0x1c as
    a child COUNT (`[ebx+0x1c]++`) — directly conflicting with the ctor's vtable
    store at +0x1c. ⇒ model each fn on its own faithful struct; names are
    placeholders, only offsets+bytes are load-bearing. Did NOT force them onto one
    class (would mis-place fields and break the green ctors).
- **CONFIRMED layouts** (the headline deliverable):
  - `CRezItmBase` (16 B, base ctor @0x13c4e0): vtbl@+0 (base 0x5ef768), parent@+0xc.
  - `CRezItm : CRezItmBase` (**0x24 = 36 B**, ctor @0x13c540): vtbl@+0 (derived
    0x5ef788), +0x10=0, +0x14=0, +0x20=-1 (id/handle unset). **BYTE-EXACT 99.23%.**
  - `CRezDir : CRezItmBase` (**0x38 = 56 B**, ctor @0x13c940): vtbl@+0 (0x5ef7a8),
    embedded child-collection two-vtbl @+0x10/+0x1c (0x5ef7c8) + head@+0x14/tail@
    +0x18 = 0, +0x20/+0x24/+0x28/+0x34=0, +0x2c=RezMgr back-ptr (ctor arg2), +0x30=1.
- **CRezDir ctor PLATEAU (78.45%)**: all 14 member stores hit the correct offsets
  with the correct values, but MSVC5 schedules the +0x10/+0x1c vtbl stores and the
  +0x14/+0x18 zero stores in a different (still-correct) order than the target and
  materializes the vtbl constant before the zero (target: `xor eax,eax` then `mov
  ecx,vtbl`; mine: the reverse → vtbl lands in eax, zero/arg2 reg-alloc cascades).
  NO source lever flips it: tried 6 store orderings, a shared `void* v` local, and
  an **embedded RezColl sub-object** (the sub-object emitted an OUT-OF-LINE ctor
  `call` + its own EH frame → 15%, far worse — so the engine inlines the collection
  init, do NOT model it as a member with a ctor). Entropy-class; left per doctrine.
  objdiff's edit-distance penalizes the mid-function reordering heavily, hence 78%
  not high-90s — but every store is correct + the vtbl operands are reloc-masked.
- **`FindEntry` (@0x13c080) is a filesystem STAT, NOT a binary search** (tomalla's
  v0.77 label is wrong; bytes are ground truth). It builds a 0x24-byte WIN32-find
  record on the stack via `0x18c780` (FindFirstFileA/GetDriveTypeA/file-times),
  returns 0 on failure, else `(*(int*)(rec+6) & 0x4000)==0x4000` — whether the
  entry's attribute dword has the **directory bit 0x4000**. `this` is never read.
  **BYTE-EXACT 99.74%** (only the stat-helper call is reloc-masked).
- **`Load` (@0x13a0f0) BYTE-EXACT 99.57%**: `if (m_buf/*+0x48*/) return 1;`
  validate `m_src->m_8 != 0 && (unsigned)m_src->m_1c <= 1` else push the assert
  string + `RezAssertFail` + return 0; if `m_size/*+0x10*/ > 0` alloc the buffer
  and **virtually read via a 3rd-slot vtable call** `m_src->m_stream->[vtbl+8]
  (off, 0, size, buf)`; if childFlag, iterate the +0x38 child collection
  (`First()`/`Next()`, both **__thiscall** engine helpers 0x184ae0/0x1848b0) and
  recurse `Load(1)` into each `node->m_14` (a sub-dir node ptr). Modeling the two
  iterators as **__thiscall member fns** (collection/node in ecx) reproduced the
  `lea ecx,[esi+0x38]; call` / `mov ecx,esi; call` shapes; declaring them
  `extern "C"` (cdecl) instead emitted `push;call;add esp,4` (wrong). The 3rd-slot
  read = a polymorphic stream class with the read at vtable index 2 (`virtual v0;
  virtual v1; virtual ReadAt(...)` → `mov edx,[ecx]; call [edx+8]`).
- **OpenSub (@0x13b0c0, 568 B) DEFERRED**: a 3rd distinct node layout (count@+0x1c,
  list-append@+0x10, gate@+0x40, name-buf@+0x64, max-dims@+0x54..+0x60), plus a C++
  EH frame, inline CString strlen+strcpy of the lookup name, the embedded list-
  append helper (0x1851e0, __thiscall on dir+0x10), two-slot virtual dispatch on
  the allocated child (`new CRezDir(0x38)` dir-branch / `new CRezItm(0x24)` file-
  branch — both call the ctors above), a 0xA8-byte item-header parse feeding the
  running max-dims, and two large external tail calls (0x13b300 recursive FS walk,
  0x13a580 item-record). >512 B of high entropy; per the prompt's "don't sacrifice
  a green fn", left for a dedicated worker — the container layouts it would confirm
  are already pinned by the two ctors. (Its dir/file branches DO confirm the new
  sizes 0x38/0x24 and that it calls ctor #2 / ctor #1 — consistent with the above.)

### CFileIO — engine KERNEL32 file-stream class (unit `filestream`; 9/10 byte-exact)
- New TU `src/Io/FileStream.cpp` (+ `FileStream.h`). This is the MFC **CFile**
  work-alike that gates ALL engine file I/O (RezMgr, WwdFile, save/load). The
  `WwdInputStream` placeholder matcher #8 declared as an external class IS this
  class — now reconstructed in full. Class name = **CFileIO** (per engine_labels).
- **LAYOUT (16 bytes, `: public CObject`)**: vtable@+0x00 (implicit, two-phase),
  **HANDLE m_handle@+0x04** (-1=closed), **int m_open@+0x08** (1 = we own the
  handle), **CString m_name@+0x0c**. `m_name` is an MFC **CString** (a single
  `char*` @+0; ctor/dtor/Empty/`operator=` are NAFXCW calls 0x1b9b93/0x1b9cde/
  0x1b9c69/0x1b9e74) — modeled as a minimal external `AfxString` class (no
  bodies + an inline `operator const char*` returning m_pchData) so its calls
  are reloc-masked. Do NOT declare an explicit vtable member: `virtual ~CFileIO`
  already puts the vptr @+0; an explicit `void* m_vtbl` shifts every field by 4.
- **BUILT WITH `/O1` (optimize for SIZE), NOT the locked `/O2`** — THE load-bearing
  discovery. This is MFC-derived (CFile) code and MFC shipped /O1. /O2 (favor
  speed) plateaus these fns at 60s–80s%; /O1 takes them byte-exact. The two tells
  of /O1 vs /O2, observable in the TARGET: (1) **push memory operands directly**
  (`push [ecx+4]`) instead of pre-loading to a reg (`mov eax,[ecx+4]; push eax`);
  (2) **keep ebp frames for functions that take the address of a local/param**
  (Read/Write/Open get `push ebp;mov ebp,esp`+`[ebp+N]`) — /O2 omits the frame and
  uses esp-relative even for escaped locals. Seek/GetPosition have NO address-taken
  locals → frameless under BOTH (so a global `/Oy-` is WRONG: it would force a
  frame on Seek too). Per-function frame ⇒ it's an opt-LEVEL choice, not /Oy.
- **Address-of-LOCAL forces the frame at /O1; `&param` may not.** Write went
  frameless when its out-param reused the `nCount` *parameter* slot, but got the
  frame (matching target) once written with a **separate `DWORD nWritten;` local**
  passed as `&nWritten` — MSVC still folds it into the param slot, but the local
  *declaration* tips the frame heuristic. Read's frame falls out of `&nCount`
  reused as the out-param under /O1.
- **The CString member ⇒ the ctor/dtor carry an MFC5 EH frame** (`mov eax,OFFSET
  FuncInfo; call __EH_prolog @0x121000; push ecx;push esi; mov [ebp-0x10],this;
  …; mov [ebp-4],ehstate; … mov fs:0,ecx; leave`). To reproduce it you MUST give
  CFileIO a **polymorphic base `CObject`** (inline-empty ctor/dtor) so you get the
  **two-phase vtable** stores (base `0x5e8cb4` then derived `0x5ed15c`, both masked)
  AND `m_open = 0` in the ctor body (target zeroes +8). A standalone polymorphic
  class gives ONE vtable + no EH frame (43% plateau) — the base + the dtor-bearing
  member together produce the exact ctor/dtor bytes (98.6%/98.8% = masked relocs).
- **Two clean bonus matches in the cluster**: the **adopt-handle ctor**
  `CFileIO(HANDLE)` @0x1bf033 (`??0CFileIO@@QAE@PAX@Z`, `m_open=0; m_handle=h;`)
  and the **scalar-deleting dtor** `??_GCFileIO` @0x1bf017 (`call ~CFileIO; if
  (flag&1) operator delete(this); return this;` — the standard `delete` idiom).
- **`CFile::Open` (0x1bf200, `ret 0xc`) = the MFC nOpenFlags→Win32 translator**:
  `nOpenFlags &= 0xFFFF7FFF`; `switch(f&3)`→GENERIC_READ/WRITE/both; `switch(f&0x70)`
  →share 0/1/2/3 (default = lpszFileName, an MFC quirk); a stack `SECURITY_ATTRIBUTES
  {nLength=0xc, NULL, bInheritHandle=!(f&0x80)}`; disposition = `f&0x1000 ?
  (f&0x2000?OPEN_ALWAYS:CREATE_ALWAYS) : OPEN_EXISTING`; CreateFileA(name,acc,share,
  &sa,disp,0x80,0); on -1 fill the `CFileException* pError` (m_lOsError@+0xc,
  m_cause@+0x8 via the os→cause mapper 0x1c1a71, m_strFileName@+0x10) if non-null,
  return 0; else store handle+`m_open=1`, return 1. **szPath buffer = `char[0x104]`**
  (MAX_PATH) so the frame is exactly `sub esp,0x110` (0x104 buf + 0xc SA). The path
  is canonicalized by `AfxFullPath` @0x1bf8f8 (GetFullPathNameA + long-name fixup),
  reloc-masked. PLATEAU 95.5%: a single MSVC layout coin-flip on the share switch —
  which of {`case 0x40`, `default`} bodies is the cmp-ladder fall-through (`cmp
  0x40;jne default` vs `cmp 0x40;je body`). All bodies + control flow + the merge
  are byte-identical; source reordering (default first/last/mid, pre-init) does NOT
  flip the polarity — entropy-class, left per doctrine.
- **objdiff "exact" undercounts**: shows `6/10` because the masked vtable/IAT/
  __EH_prolog/CString-call operands are DIR32-vs-REL32 on differently-named symbols
  (the documented ~99.5%-fuzzy-but-byte-exact idiom); 9 fns verified instruction-
  identical vs `dump_target.py` + `llvm-objdump -dr` on the delinked obj.
- UNLOCKED: every engine file-I/O caller is now verifiable — `WwdFile::IsValidWwd/
  CheckHeader` (their `WwdInputStream` stub is literally this CFileIO), `RezMgr`/
  `CRezDir::Load`/`RezSync::Init` (the 0x1bfxxx CreateFileA cluster), `Image::
  LoadFromRez` (.PID open), and the save/load path. The CFile clone @0x1bf16f
  (DuplicateHandle into a `new CFileIO(-1)`) and Write/Seek/Open's 3 throw helpers
  (0x1c18b7 AfxThrowOsError, 0x1c199c AfxThrowFileError, 0x1c1a71 os→cause) are now
  anchored callees.

### Utils::WinAPI (Win32 helper TU — unit `winapi`; 4/4 driven to byte-exact)
- New TU `src/Utils/WinAPI.cpp`. `FileExists` @0x1189c0, `ActiveWait` @0x13dfe0,
  `IsGruntzCDInAnyDrive` @0x1fd50 are 100% (reloc/IAT/thunk-masked only);
  `GetGruntzDriveLetter` @0x1ffe0 plateaus 97.9% on a register-allocation residue
  (logic/struct/EH/control-flow byte-exact — see below).
- **`FileExists(char*)` returns `int`, not `bool`** — the disasm zeroes/copies via
  full-width `eax` (`xor eax,eax`, `mov eax,edx`), so the symbol is `…@@YAHPAD@Z`
  (H), not `_N`. Picked by the result WIDTH in the bytes, not the prompt's "bool".
  Same for `IsGruntzCDInAnyDrive` → `…@@YAHXZ`. Impl = `OpenFile(path,&of,0x4000
  /*OF_EXIST*/) != -1` over a 0x88-byte `OFSTRUCT` local; null/empty path returns
  the (already-zero) pointer in eax with NO explicit `xor` (MSVC reuses the loaded
  arg reg — write `if(!p) return 0;` and it reuses eax). FileExists is **emitted
  twice** (0x1189c0 and 0x1fd70, byte-identical); objdiff pairs by NAME so map the
  one symbol to either RVA (used 0x1189c0).
- `ActiveWait(ms)` is a `timeGetTime()` busy-spin: `DWORD t=timeGetTime()+ms;
  while(timeGetTime()<t);` — uses **WINMM!timeGetTime**, not Sleep/GetTickCount.
- **CD-detection (`GetGruntzDriveLetter`)**: memoised in a file-scope `static char`
  (binary @0x62b25c; 0 = unprobed). (1) Reads `HKLM\Software\Monolith Productions\
  Gruntz\1.0` value **"CdRom Drive"** via `Utils::RegistryHelper` (Open args decode
  exactly like AdvancedOptions, szSubKey defaulting to "Software"); if the stored
  byte `> 0x14` (a real letter) it `sprintf("%c:\\")` + `GetDriveTypeA==5
  (DRIVE_CDROM)` + `FileExists("%c:\\GAME\\GRUNTZ.EXE")`. (2) Else scans `'A'..'Z'`
  with the same drive-type + marker-file check. **Marker file = `<L>:\GAME\
  GRUNTZ.EXE`**; format strings `"%c:\\"` and `"%c:\\GAME\\GRUNTZ.EXE"` via the
  engine's `sprintf` @0x11f890. Both found-paths share one tail (`s_x=letter;
  return letter`) → reproduce with a `goto found;` single exit.
- **`RegistryHelper` needs a ctor + dtor here** (added inline to the header):
  `RegistryHelper(){m_0=0;}` and `~RegistryHelper(){Close();}`. The stack-local
  `reg` makes MSVC emit a **C++ EH frame** (`__CxxFrameHandler` + FuncInfo, the
  `fs:0` registration + `push -1; push handler`), so this TU must build with
  **`/GX`** (per-unit `cflags` override — the global locked set has no /GX). Adding
  /GX took GetGruntzDriveLetter 73%→94%; the ctor fixed the early `reg.m_0=0`
  scheduling (it inlines at the object's scope-entry, exactly where the binary
  zeroes it) and the dtor (Close) inlines at each `return` with the
  `mov [eh_state],-1; call Close` shape. **Adding the ctor/dtor to the shared
  header did NOT regress the already-matched `registryhelper`/`advancedoptions`
  units** (verified) — a clean way to share a class across TUs.
- **`/GX` ⇒ C++ EH frame** is the load-bearing flag tell: a function with `fs:0`
  + `push -1; push <handler>` where the handler is `mov eax,OFFSET FuncInfo; jmp
  __CxxFrameHandler` means a stack object with a destructor under `/GX`. The
  handler/FuncInfo/`__except_list`/`$L` symbols are all reloc-masked in objdiff.
- **Stack-buffer SIZES are load-bearing for the frame + slot offsets.** At /O2
  slot *names* are free but sizes/decl drive placement: matching the binary's
  `sub esp,0x460` + the reg-object landing exactly at `[esp+0x150]` required
  `value[32]`, `drivePath[32]`, `exePath[256]` (so `0x10+32+32+256 = 0x150`), reg
  (0x21c), then the A-Z scan's own `drivePathScan[256]` placed AFTER reg. The two
  paths SHARE `exePath` (one slot) but get DISTINCT drive-path buffers (MSVC's
  live-range allocation) — model that as two separate buffers + one shared.
- **Plateau (97.9%)**: the sole residue is MSVC reading the value byte into `bl`
  directly (`mov bl,value[0]; movsx esi,bl`) vs the target's `mov al,…; movsx
  esi,al; mov bl,al` — a register-allocation coin-flip (`letter` must end in `bl`
  for the A-Z loop counter; the target spills via al-then-copy, we read straight
  into bl). Same opcodes, different reg field + one redundant `mov bl,al`. No
  source lever flips it (tried char/int temps, signed-char cmp, letter-vs-driveChar
  as the %c arg) — entropy-class; left per the doctrine.
- OPTIONAL `LegacyAreProcessesRunning` @0x118ce0 LEFT OUT: 501 B TOOLHELP32 process
  enum (GetModuleHandleA/GetProcAddress for CreateToolhelp32Snapshot/Process32First
  /Process32Next, OpenProcess, CloseHandle, PROCESSENTRY32 @0x128) that also calls
  three un-identified helpers (0x118f60, 0x120090 [133 B], 0x11fdf0 [208 B, likely
  strstr]) — too many new deps to match cleanly without destabilising the 4 greens.
- UNLOCKED (callers now anchored): `WinMain` @0x11c860, `CGruntzMgr::Get*` CD-gate
  callers of `IsGruntzCDInAnyDrive`/`GetGruntzDriveLetter`, and the `sprintf`
  @0x11f890 call surface.

### WWD header validators (unit `wwdfile` — IsValidWwd + CheckHeader byte-exact)
- `WwdFile::IsValidWwd` @0x160530 (293 B, __stdcall `ret 8`) and
  `WwdFile::CheckHeader` @0x160660 (299 B, __stdcall `ret 8`) BOTH byte-exact
  (96.635% fuzzy / 0% "exact" — every non-matching byte is a reloc-masked call
  operand; verified instruction-identical vs `dump_target.py`). Args: `(const
  char* name, void* headerBuf)`; full-width-eax `int` return (1/0), NOT bool.
- **Both open a file by NAME and validate the 0x5F4 (1524-byte) header**: null-guard
  `name` then `headerBuf` (early `xor eax,eax; ret 8`, BEFORE the stream object is
  constructed → no dtor on those paths), construct a stack-local engine **binary
  file stream**, `Open(name, 0, 0)` (returns NONZERO on success — reversed sense),
  `Read(buf, 0x5F4)` and require `== 0x5F4`, then require the header **signature
  (first u32 == sizeof(WwdHeader)) `<= 0x5F4`** (`mov eax,[buf]; cmp eax,0x5f4;
  jbe valid`). No magic-string/CRC/memcmp — the ONLY checks are read-length==0x5F4
  and firstU32<=0x5F4. WWD has no ASCII magic; the "signature" is the self-size u32.
- **IsValidWwd reads straight into the caller buffer**; **CheckHeader reads into a
  PRIVATE `char header[0x5F4]` stack buffer then `strcpy`s it out to the caller**
  (`repnz scasb; rep movsd; rep movsb` = inline strlen+copy at /O2/Oi). The stack
  frame is `sub esp,0x604` = `0x5F4` (header buf) + `0x10` (the 16-byte stream obj).
  IsValidWwd's frame is just `sub esp,0x10` = the stream object alone.
- **The stack-local stream object forces a C++ EH frame → `/GX` on the unit**
  (same tell as winapi/gruntzapp/gameapp: `push -1; push FuncInfo; push fs:0`, a
  `.text$x` unwind funclet `lea ecx,[ebp-N]; jmp dtor` + `mov eax,FuncInfo; jmp
  ___CxxFrameHandler`, and a `mov [esp+N],-1` EH-state write before each dtor). The
  dtor (Close) inlines at EVERY post-construction exit; model it as a real
  stack-local C++ object with a declared (external, unmatched) ctor/dtor.
- **Engine binary file stream class** (16 bytes; ctor @0x1befd7, dtor/Close
  @0x1bf121, `Open` @0x1bf200 `ret 0xc`, `Read` @0x1bf328 `ret 8`): layout
  vtable@+0, **HANDLE@+0x4** (CreateFileA result; -1 = closed), open-flag@+0x8,
  **CString filename@+0xc** (MFC-ish; sub-ctor 0x1b9b93 shares the global empty-
  string @0x65162c). Open() builds the path via a CString, CreateFileA, stores the
  HANDLE@+4; dtor CloseHandles if HANDLE!=-1. Declared the class with **unmatched
  external** ctor/dtor/Open/Read — their `call rel32` displacements are reloc-masked
  in objdiff, so the validators are byte-exact even though the stream class name I
  picked (`WwdInputStream`) differs from the engine's. (This is why a call-heavy fn
  plateaus at ~96.6% not ~99%+: 8 reloc-bearing calls in ~120 bytes.)
- **Declaring an unmatched callee as an external class method** (no body) is the
  clean way to emit a reloc-masked engine call from a C++ stack object — you get
  the ctor/dtor scheduling + EH frame for free, and objdiff masks the call name.
- UNLOCKED: anchors the WWD load entry points — `CGameLevel::LoadWwd` @0x15d280
  (the `cmp firstDword,0x5f4` + 0x17d-dword header copy caller) and the
  `CRezDir::Load` path now have these two header-validator leaves in place; the
  engine binary file-stream class (ctor 0x1befd7 / Open 0x1bf200 / Read 0x1bf328 /
  Close 0x1bf121) is now mapped for any future TU that does file I/O.

### zlib (DONE — 51 fns byte-exact)
- 1.0.4, statically linked, locked flags `/O2 /MT` (cdecl). The REZ entry payloads
  are raw deflate → `_inflate*`/`_uncompress` are the live decompressors.
- `WwdFile::InflateMainBlock @0x160790`: algorithm-exact but plateaus ~88.7% on an
  un-steerable MSVC5 register allocation — left per the entropy doctrine. First
  real-world confirmation that a high plateau with no source diff IS done.

### REZ / WWD asset load
- Pinned on-disk structs (clean-room from OpenClaw + binary): `WwdHeader 0x5F4`,
  `WwdPlaneHeader 0xA0`, `WwdObjectRecord 0x11C`, `PidHeader 0x20`
  (`structure/formats/`). `imageSet` is a length-prefixed string, then a `sound`
  string. RezMgr container offsets still @unconfirmed (blocks `CRezDir::Load`).

### Wap32 / CGameWnd::CreateAndShow + CGameApp::InitializeGameWindow (window glue)
- `CGameWnd::CreateAndShow` @0x13cf20 (143 B, __thiscall `ret 8`) byte-exact
  (99.83% fuzzy; only the singleton-global + 2 IAT relocs masked). Signature is
  `int CreateAndShow(CGameWndCreateParams *pParams, void *pOwner)`: bails (→0) if
  pParams/pOwner null **or** a window is already active, then `m_8=pOwner;
  s_activeWnd=this; m_c=0;` → `CreateWindowExA(12 args)` → store HWND in **m_4
  (+0x04)** → if null return 0 → `ShowWindow(hwnd, 1)` → return 1.
- **CGameWnd HWND member = +0x04 (m_4)**; m_8 (+0x08) = owner ptr; m_c (+0x0c) =
  guard (re-zeroed here). The ctor's `m_4=0` is the same HWND slot.
- **Active-window singleton = file-scope `static CGameWnd*` @ binary 0x653c68**
  (DAT_00653c68) — DISTINCT from the CGameApp instance counter @0x653c6c. Read by
  CreateAndShow (reject if already set) AND by GameWindowProc (dispatch target).
- **12-arg `CreateWindowExA` from a params struct = struct-field-push idiom.** The
  binary loads `[eax+0], [eax+4], … [eax+0x2c]` in ASCENDING order, each pushed
  immediately. MSVC pushes the call's args **right-to-left**, so the FIRST-loaded
  field (`[eax+0]`) is the LAST/rightmost CreateWindowExA arg (`lpParam`) and
  `[eax+0x2c]` is the first arg (`dwExStyle`). ⇒ declare the params struct in
  **REVERSE CreateWindowExA-arg order** (lpParam@+0 … dwExStyle@+0x2c) and call
  `CreateWindowExA(p->dwExStyle, …, p->lpParam)` normally; the ascending-offset
  push sequence falls out. (Natural-order struct loads +0x2c first → 97.6%; the
  reverse-order struct → 99.8%.) **GOTCHA: a header-only struct reorder does NOT
  retrigger the .cpp's recompile in this ninja setup — `rm` the unit's
  base/current .obj (or touch the .cpp) before rebuild or you'll diff stale code.**
- `CGameApp::InitializeGameWindow` @0x13db60 (87 B, `ret`) = **`return new
  CGameWnd;`** — the CGameWnd-allocation analog of CGruntzApp::InitializeGameManager
  (`new CGameMgr`). `operator new(0x10)` (sizeof CGameWnd) + CGameWnd ctor under a
  C++ EH frame (`push -1; push FuncInfo; push fs:0`); `push ecx` reserves one local
  dword for the new ptr / EH temp; **`this` (the CGameApp) is never read**, uses no
  ≥0x254 fields. **Class decision: it's a CGameApp member** (sits in the 0x13dxxx
  CGameApp cluster; builds a CGameWnd, no game-app-specific fields) → extend
  `gameapp`, NOT gruntzapp. Throwing ctor forces **`/GX`** on the gameapp unit;
  adding `/GX` did NOT regress the 3 already-100% gameapp fns (CloseResources/
  InitializeAccelerators/ReportError stayed exact) — same zero-cost-EH result as
  winapi/gruntzapp. 99.79% fuzzy (FuncInfo/`fs:0`/operator-new relocs masked; the
  `??0CGameWnd` ctor call resolves directly).
- **GameWindowProc @0x13cff0 SKIPPED — 860 B** (far over the ~400 B budget). It's a
  static __stdcall `ret 0x10` wndproc: reads the s_activeWnd singleton, on null →
  DefWindowProcA; else `[ecx]`-vtable dispatch via a triple sub-normalize switch
  ladder (0x1..0xf with a `jmp [eax*4+table]` jump-table @0x53d34c, 0x10/0x1c/0x111
  ranges, and 0x200..0x206 via tables @0x53d360/0x53d374) → each case calls a
  CGameWnd vfunc (`[vptr+0x0c..0x54]`); default → DefWindowProcA. Needs ~20 CGameWnd
  virtual slots + 3 jump tables; deferred to a dedicated worker.

### Wap32 / CGameApp (DONE — ctor + 4 methods; unit `gameapp`)
- Matched `??0CGameApp@@QAE@XZ` (ctor) + `?CloseResources@…XXZ` @0x13d8c0,
  `?InitializeAccelerators@…HPBD@Z` @0x13dc20, `?ReportError@…XIJ@Z` @0x13dcb0,
  `?InitializeDefaultWindowClass@…XXZ` @0x13d9b0 — all byte-exact (commit 7323fe5).
- Layout pinned (`src/Wap32/Wap32.h`): m_4/m_8 = `CGameResource*` (polymorphic,
  vtable slot0 = scalar-deleting dtor; m_4 also exposes HWND@+0x4 + guard@+0xc),
  m_c=HINSTANCE, m_10=HACCEL, m_18 flag byte (bit1=system arrow cursor),
  m_a0 cursor/icon name buf, m_160 class-name buf, `WNDCLASSA m_wc@+0x1e8`,
  m_244/m_248(one-shot guard)/m_24c/m_250 error fields.
- UNLOCKED: `GameWindowProc` @0x13cff0 (`?GameWindowProc@CGameApp@@SGJPAXIIJ@Z`,
  static __stdcall) now anchored in the TU; its body is in-scope.

### Wap32 / CGameApp Init orchestration (extended gameapp — +3 methods)
- Added `?VirtualUnknownMethod03@…UAEHPAXPAD11HHH@Z` @0x13d7b0 (BYTE-EXACT 100%),
  `?InitializeDefaultCreateStruct@…UAEXXZ` @0x13da50 (99.15%, 1 scheduling residue),
  `?VirtualUnknownMethod02@…UAEHPAUGameInfo@@PAUtagWNDCLASSA@@PAUtagCREATESTRUCTA@@@Z`
  @0x13d5d0 (97.28%, register-alloc residue). The 6 prior methods stayed exact.
- **The whole CGameApp had to become a full VIRTUAL class (tomalla vtable order).**
  0x13d5d0 dispatches the other methods via the vtable (`call [vptr+0x2c/0x34/0x38/
  0x3c/0x40]` = InitializeAccelerators/InitializeGameWindow/InitializeGameManager/
  InitializeDefaultWindowClass/InitializeDefaultCreateStruct), so they MUST sit at
  the binary's slot indices. Declared all 16 vtable slots in tomalla order (empty
  inline stubs for the unmatched ones) ⇒ matched-method manglings flip **`Q`→`U`**
  (`?CloseResources@CGameApp@@QAEXXZ` → `…UAEXXZ` etc.); update symbol_names.csv to
  the `U` form. Method BODIES are byte-identical virtual-vs-nonvirtual, so the 6
  greens stayed exact. **This also flips CGruntzApp::InitializeGameManager Q→U**
  (it now overrides a virtual base method) — its gruntzapp row must change to
  `?InitializeGameManager@CGruntzApp@@UAEPAVCGameMgr@WAP32@@XZ` (a CORRECTION, the
  old `Q` was technically wrong; body byte-identical, stays 99.79%). Base return
  type must be covariant: `WAP32::CGameMgr*` (forward-declared in Wap32.h, full def
  in GameApp.cpp — keeps GruntzApp.cpp's own `WAP32::CGameMgr` def, separate TUs).
- **CONFIRMED tomalla's `GameInfo` (0x1d4 B) + embedded layout** (now in Wap32.h):
  CGameApp embeds `GameInfo m_gameInfo@+0x14`, `WNDCLASSA m_wc@+0x1e8`,
  `CREATESTRUCTA m_createStruct@+0x210`. GameInfo: size@+0(=0x1d4), windowClassFlags
  @+4 (an **int**, NOT char — read full-dword `mov eax,[..]` with no `movsx`; the
  `D`→`H` mangling), hInstance@+8, szCmdLine[0x80]@+0xc, szGameIdentifier[0x40]@+0x8c,
  szWindowName[0x40]@+0xcc, szWindowClassName[0x80]@+0x14c, windowWidth@+0x1cc,
  windowHeight@+0x1d0. ⇒ the old flat `m_a0`(+0xa0)=szGameIdentifier,
  `m_18`(+0x18)=windowClassFlags, `m_160`(+0x160)=szWindowClassName were SUB-FIELDS
  of m_gameInfo; +0xe0=szWindowName, +0x1e0/+0x1e4=width/height are new exposures.
- **03 (the param→struct builder):** `if(!hInstance) return 0;` then build a stack
  `GameInfo gi` (memset→`rep stos` 0x75 dwords), fill fields, conditionally
  `strcpy` the 3 names (inline `rep movs` at /O2/Oi), `return Method02(&gi,0,0)`.
  The vcall target = vtable slot +0x4 (Method02) ⇒ a `this->Method02(...)` virtual
  call. windowClassFlags must be `int` (see above) for byte-exact.
- **InitializeDefaultCreateStruct idiom wins (DialogFrame style):** the windowed
  branch picks `style = (DialogFrame) ? 0xCA0000 : 0xCF0000` with exStyle 0x40000;
  fullscreen = 0x80080000 / 0x40008. **Write it as `style=0xCF0000; if(flags&2)
  style=0xCA0000;` (default-then-conditional-override), NOT a 2-way `if/else` select**
  — the two constants differ by one bit (0x50000) so an `if/else` folds to a
  BRANCHLESS `neg/sbb/and 0x50000; add 0xca0000` (14%); the default-override emits
  the target's `mov 0xcf0000; je; mov 0xca0000` branch (78%→99%). Assign `style`
  BEFORE `exStyle` inside the branch to match the `mov ecx;mov edx` store order.
- **x and y (CW_USEDEFAULT/0) must be TWO separate locals**, both set in one
  `if/else` — a single shared local folds the 0x80000000-vs-0 select to the same
  branchless `neg/sbb/and 0x80000000` (target uses a branch: value lands in a
  callee-saved reg for x + a stack slot for y; 14%→78%).
- **Method02 (Run/Init orchestration, 97.28% PLATEAU):** validate count(<=1)+
  GameInfo.size==0x1d4 + (if pWndClass) its lpszClassName non-empty; `m_244=1;
  m_248=m_24c=m_250=0;` copy `*pGameInfo→m_gameInfo` (`rep movs`); resolve
  m_hInstance from gameInfo.hInstance ?: pWndClass->hInstance ?: pCreateStruct->
  hInstance (else fail) — write as ONE chained `&&` so the fails share the exit;
  `if(!szWindowClassName[0]) sprintf(…,"%sClass",szGameIdentifier);` +
  `if(!szWindowName[0]) sprintf(…,"%s",…)` (engine sprintf @0x11f890); copy/
  default the WNDCLASSA (vtbl +0x3c) and CREATESTRUCTA (vtbl +0x40); RegisterClassA;
  InitializeAccelerators (vtbl +0x2c); `m_4=InitializeGameWindow()` (vtbl +0x34),
  `CreateAndShow(&m_createStruct,this)` else `delete m_4`; `m_8=InitializeGameManager()`
  (vtbl +0x38), `m_8->Run(m_4,&szCmdLine)` (mgr vtbl +0x4) else `delete m_8`. All
  validation `return 0`s → one `goto Fail;` shared epilogue (separate-`return 0`
  nests create an extra epilogue). PLATEAU = the conditional `pCreateStruct->
  hInstance` fallback: MSVC keeps pCreateStruct in callee-saved **esi** across the
  sprintf/Init calls (reloading it per branch) where the target loads it fresh into
  **eax** (dead immediately) + once into esi for the copy — a pure reg-alloc
  coin-flip; 165=165 instructions, logic/layout/CFG byte-identical, only the
  pCreateStruct home register + one reload differ. Tried 4 source forms (chained-&&,
  nested-if, split-if, temp-ptr) — all 94.7–97.3%, none flips it. Entropy-class.
- All vtable/IAT/string-literal/engine-sprintf operands are reloc-masked → the
  99% scores are byte-exact-modulo-relocs per the standard idiom.
- UNLOCKED: the CGameApp vtable is now fully laid out (16 slots), so any caller
  that dispatches through it (`WinMain`/`CGruntzApp::InitInstance`) is anchored;
  `GameInfo`/`CREATESTRUCTA`/`WNDCLASSA` member offsets are pinned for the rest of
  the app-init path; the game-manager `Run` (mgr vtbl +0x4) entry is mapped.

### Utils::RegistryHelper (config subsystem — unit `registryhelper`; 4 matched, more open)
- Matched byte-exact (commit 76c2483): `GetValueString` @0x1394a0, `GetValueDword`
  @0x1395d0, `Close` @0x139330, `GetRegistryKey` @0x139650 (static __stdcall).
- Layout pinned: `+0x00` open/result gate (tested before queries; Close zeroes it);
  `+0x08/+0x0c/+0x10/+0x14` a chain of nested HKEYs opened along the key path
  (Close RegCloseKey's all, skipping +0x14 when == +0x18); `+0x18` the deepest/open
  HKEY that the getters operate on.
- **FULLY MATCHED 8/8** (commit 4a3600d adds `Open` @0x139210, `InitializeLastKey`
  @0x139370, `SetValueString` @0x1393b0, `SetValueDword` @0x139460). Layout adds:
  `+0x04` base HKEY (saved by Open), `+0x1c` char[0x100] (szKeyName2),
  `+0x11c` char[0x100] (szLastKey); min size ≥0x21c.
- `GetRegistryKey` @0x139650 is **__thiscall** (`?…@@QAEH…`), NOT static __stdcall —
  even though its body never reads `this`/ecx (all args off stack, `ret 0xc`). Tell
  is caller-side: callers emit a redundant `mov ecx,this` before each call;
  declaring it static drops that and regresses every caller. (A thiscall that
  ignores `this` == static-stdcall in the *callee* bytes, but callers differ.)
- NOTE: `0x1396f0/0x139710/0x1397a0` are a DIFFERENT class (vtable@+0x1c), not
  RegistryHelper. No `GetValueBool` exists in the cluster.
- UNLOCKED & verifiable now: `LoadOptions` @0xb1b0 (5× GetValueDword), `SaveOption`
  @0xb110 (SetValueDword), `SaveOptions` @0xb270, `SetDefaults` @0xb160,
  `AdvancedOptionsDialogProc` @0xafb0 (the dialog TU).

### CGruntzApp app object (unit `gruntzapp` — 2/2 byte-exact, reloc-masked plateau)
- New TU `src/Gruntz/GruntzApp.cpp`; `class CGruntzApp : public CGameApp`
  (`#include "../Wap32/Wap32.h"`). Both methods byte-exact modulo reloc-masked
  operands; objdiff scores them ~99.2–99.4% fuzzy / 0% "exact" purely from the
  reloc *names* (operator-new/ctor/EH-FuncInfo/IAT/global addrs) — verified
  byte-identical against `dump_target.py` + `llvm-objdump -dr` on the base obj.
- **`InitializeGameManager` @0x080a20 (90 B) = `return new WAP32::CGameMgr;`** —
  NOT a member-store-then-return as the tomalla guess suggested; the disasm has NO
  `mov [this+N],eax`. It is `operator new(0xa30)` (`??2@YAPAXI@Z`) + a *throwing*
  ctor (`??0CGameMgr@WAP32@@QAE@XZ`, reached via an incremental-link thunk) under
  a **C++ EH frame** (`push -1; push FuncInfo; push fs:0`), with the EH state slot
  at `[esp+0xc]` set to 0 before the ctor and to -1… (actually the dual-exit just
  restores fs:0). The `push ecx` at entry is MSVC reserving **one dword of locals**
  for the new pointer / EH-tracked temp — `this` is never read. The throwing ctor
  forces **`/GX`** on the TU (same tell as GetGruntzDriveLetter: a stack/temp
  object with a dtor under EH ⇒ `__CxxFrameHandler` + FuncInfo). Manager size =
  **0xa30 bytes** (pinned by the `push 0xa30` to operator new); model `CGameMgr`
  as a forward class with a `char[0xa30]` body + a declared ctor — full layout not
  needed for the new+ctor+return shape.
- **`ErrorDialogProc` @0x080c70 (85 B) is a `static __stdcall` member**
  (`?ErrorDialogProc@CGruntzApp@@SGHPAXIIJ@Z`, `ret 0x10`) — the `SG` (static
  __stdcall) mangling, NOT `QAG`/`AAG`; body reads no `this`. Same WM-message
  ladder as AdvancedOptions: `sub eax,0x110; je INITDIALOG; dec eax; jne default`
  with the **WM_INITDIALOG case body at the function tail** (reached by forward
  `je`). It **stores hWnd into a file-scope `static HWND` (binary @0x64557c)
  UNCONDITIONALLY** right after the `sub` (before the switch dispatch) — write the
  `g_errorHwnd = hWnd;` assignment as the first statement so it schedules before
  the switch. WM_COMMAND: `if (wParam==1 || wParam==2) EndDialog(hWnd, 0); return
  1;` (IDOK and IDCANCEL share one EndDialog(,0) path — `cmp 1;je; cmp 2;je`).
  WM_INITDIALOG: `SetDlgItemTextA(hWnd, 0x40d, g_errorText); return 1;` where the
  error text is a **`push imm` of the buffer ADDRESS** (binary @0x644ea0, `68 ..`
  not `ff 35 ..`) ⇒ a file-scope `static char g_errorText[]` (address-of), not a
  `char*` global.
- CGruntzApp LAYOUT facts: confirmed `: public CGameApp` (base ends @0x254);
  **neither matched fn touches a CGruntzApp instance field** — InitializeGameManager
  returns the manager rather than storing it (so the "manager-pointer offset" is
  NOT pinned by these two fns; a *caller* like Run/InitInstance does the store),
  and ErrorDialogProc is static. No CGruntzApp-specific members needed yet.
- UNLOCKED: the callers that store the manager + the error-dialog launcher —
  `CGruntzApp::Run`/`InitInstance` and `WinMain @0x11c860` now have these two
  callees anchored; the manager-pointer member offset will fall out of whichever
  caller does `m_xxx = InitializeGameManager()`.

### AdvancedOptions dialog (unit `advancedoptions` — DONE, 5/5 byte-exact)
- All five byte-exact modulo reloc-masked operands (commit pending). They are the
  first *consumers* of RegistryHelper from a higher-level TU — confirms the whole
  Open/Close/Get/SetValueDword call surface links + matches end-to-end.
- The dialog persists 5 flags under **HKLM\Software\Monolith Productions\Gruntz\1.0**.
  The `Open` call args (right-to-left pushes, `this` in ecx) decode to
  `Open("Monolith Productions","Gruntz","1.0", NULL/*szLastKey*/,
  HKEY_LOCAL_MACHINE, NULL/*szSubKey→"Software"*/)`.
- **Control-ID enum** (LOWORD of WM_COMMAND wParam; full wParam compared, not
  LOWORD): `IDC_DISABLE_VIDEO=0x46c`, `IDC_DISABLE_AUDIO=0x46d`,
  `IDC_DISABLE_SOUND=0x46e`, `IDC_DISABLE_MUSIC=0x46f`, `IDC_DISABLE_MOVIE=0x470`,
  `IDC_DEFAULTS=0x426`. Std dialog cmds: `IDOK=1` (save+EndDialog(,1)),
  `IDCANCEL=2` (EndDialog(,0)). Reg value names = the literal labels
  ("Disable Direct Video Access", "Disable Audio", "Disable Sound",
  "Disable Music", "Disable High Quality Movie"), mapped in that fixed order to
  IDC_DISABLE_VIDEO..MOVIE.
- **`switch(message)` on 0x110/0x111 → `sub eax,0x110; je; dec eax; jne`** (NOT
  `cmp` per case). The first case body (`WM_INITDIALOG`) is laid out **at the END**
  of the function (after the default `xor eax,eax;ret`), reached by a forward `je`.
  Writing two separate `if(message==…)` blocks emits per-case `cmp` and inlines
  INITDIALOG first → wrong layout (0% structural). The `switch` reproduces the
  subtract-normalize ladder AND the tail-placement of the first case. [VERIFIED here]
- **Two globals referenced by address are file-scope defs in the TU**:
  `g_registryHelper` (the `Utils::RegistryHelper` instance @0x6295d8) and
  `g_hInstance` (HINSTANCE @0x651618). A plain `static T g_x;` provides the symbol;
  the address-load bytes (`mov ecx,OFFSET g`, `mov edx,ds:[g]`) match — the reloc
  naming them is masked. (The global's ctor/dtor wrappers @0xaf00–0xaf90 are a
  SEPARATE concern: af50 zeroes the instance, af90 calls Close, af70 registers af90
  via atexit-style 0x11f490 — CRT static-init/teardown of `g_registryHelper`, NOT
  called by any of the 5 dialog fns. Left for whatever TU owns the global's
  init/term; the prompt's "SaveOptions calls a tiny __cdecl wrapper" turned out to
  be SaveOption itself, reached through an incremental-link thunk.)
- **Incremental-link thunks**: in the delinked target, intra-EXE calls go through a
  `jmp rel32` stub in the low .text thunk region (e.g. `call 0x12c1; 12c1: jmp
  0xb110`). objdiff shows `call thunk_FUN_0040b110` on the target side vs your direct
  `call ?SaveOption@…`; both are `call rel32` with a masked displacement — byte-exact,
  not a real diff. (Same for the dialog proc's calls to SaveOptions/SetDefaults/
  LoadOptions.)
- `IsDlgButtonChecked` returns the BST_* state directly into `SetValueDword`'s value
  arg (no normalization) — `pReg->SetValueDword(name, IsDlgButtonChecked(hWnd,id))`.

## Matching idioms confirmed here (candidates for matching-patterns.md)
- Member inits emit in the **optimizer's schedule order**, not declaration order
  (e.g. CGameApp stores +0x10 before +0x0c) — mirror that order in the source.
- Names/namespaces are **placeholders**: only offsets and code bytes are
  load-bearing. Pick any mangling; make `symbol_names.csv`'s name equal exactly
  what `cl` emits for your source symbol (objdiff pairs base↔target by name).
  Read the real mangled name from the base obj: `llvm-objdump -t
  build/objdiff/base/<unit>.obj | grep <hint>`.
- vtable/global stores read ~99.5% *fuzzy* though byte-exact (REL32 vs DIR32 on a
  differently-named symbol) — confirm by reloc-masked byte-compare; not a real diff.
- **`delete pObj`** reproduces the scalar-deleting-dtor `mov eax,[ecx]; push 1;
  call [eax]` exactly — a class with one `virtual ~T()` puts `??_G…` at vtable
  slot 0; `delete` emits the inline null-check + `push 1; call [vptr]`. No manual
  vtable forging.
- **Win32 imports**: declare a minimal `__declspec(dllimport) … __stdcall` block
  (reproduces `FF15 [IAT]`); do NOT `#include <windows.h>` — keep the visible
  symbol SET small (the compiler hashes it; entropy follows header churn).
- **`rep stosd` of N dwords** = a zero-init of an N-dword struct (e.g. WNDCLASSA =
  10 dwords); reproduce with an N-iteration `int*` zero loop.
- **Calling-convention pick by `ret N`**: `ret` (c3) = __cdecl/void-arg or thiscall
  no-args; `ret N` = callee-cleanup (__stdcall/__thiscall) with N bytes of stack
  args. A `static` member with `ret N` is `static … __stdcall` (mangles `@@SG…`);
  default `static` is __cdecl (`@@SA…`, `ret`) — match the `ret` to choose.
- **Same dllimport called N× in one body** → MSVC5 caches the IAT slot in a reg
  once (`mov edi,ds:[__imp]; call edi`), not N× `ff15 [IAT]`. Free if you just
  call the function N times.
- **`RegFn(...) == 0` bool return** → `neg eax; sbb eax,eax; inc eax`; write
  `return Fn(...) == 0;`.
- **`strcpy`+re-strlen** at `/O2 /Oi` inlines to `rep movsd/movsb` then a strlen
  scan — no `call _strcpy`; write the plain `strcpy(dst,src); n = strlen(dst);`.
- **Statement order is scheduled faithfully**: a store emitted *between* an arg
  push and its `call` must be written *before* the call statement in source
  (reordering it after the call shifts a byte).
- **Local-array (string literal) declaration POSITION drives scheduling**: declare
  `char s[]="Software";` *just before its use*, not at function top — top
  declaration scheduled the `.data`/`.rdata` 3-load copy idiom early and broke ~15%.
- **`static __stdcall` vs `__thiscall`-ignoring-this**: identical callee bytes; the
  ONLY difference is the caller's `mov ecx`. Decide by the caller, not the callee —
  if callers set ecx, it's a (possibly this-ignoring) thiscall member.
- **Invert a null-check to get `jne`**: writing the null/early path FIRST
  (`if(!p){...} else ...`) emits `test;jne`; the `!=0` on a call result emits the
  `neg/sbb/neg` 0/1 normalize.

## Blocked / deferred
- (none yet — populate as matchers hit walls)
