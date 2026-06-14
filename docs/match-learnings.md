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
  `scripts/gen_match_queue.py` to refill `docs/match-queue.md`.
- Per-function asm diff: `objdiff-cli diff -p build/objdiff -u <unit> <mangled-sym>
  -o - --format json-pretty`. Roll-up: `scripts/rebuild.py`.

## Subsystem notes
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
