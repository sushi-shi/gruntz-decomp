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
### Application-object lifecycle — CGruntzApp ctor/Init + CGameApp idle/free virtuals (extended `gruntzapp`+`gameapp` — 4/4 BYTE-EXACT under reloc-masking)
- Closed the WinMain→app lifecycle: the `new CGruntzApp` ctor, the `[vtbl+0x8]`
  Init it dispatches, and two CGameApp base virtuals (the per-frame idle the pump
  calls + the game-manager free). All four byte-exact vs `dump_target.py`; the two
  no-reloc CGameApp virtuals score **100.000%**, Init **100.000%** (lone rel32 to
  the base masked), the ctor **99.286%** (lone DIR32 vptr operand masked). No new
  unit, no flag change. **No regression**: gameapp fuzzy 99.086% (was 99.056% —
  the two new byte-exact fns nudged it UP), gruntzapp 99.527%, gamewnd 99.757%,
  winmain 98.971%; every prior matched sibling held (RunMessageLoop 99.938%,
  CloseResources/ReportError 99.9%+, ~CGruntzApp 98.625% plateau).
- **CGruntzApp ctor @0x80850 (`??0CGruntzApp@@QAE@XZ`, 18 B) = the canonical
  derived-ctor shape: empty body, base-chain + vptr-store, return `this`.**
  `push esi; mov esi,ecx; call CGameApp::CGameApp; mov [esi],&CGruntzApp_vftbl;
  mov eax,esi; pop esi; ret`. An empty-bodied `CGruntzApp::CGruntzApp() {}` emits
  it exactly — MSVC auto-injects the base-ctor call and the derived vptr store
  (DIR32 to `??_7CGruntzApp@@6B@` = the @0x5e9ab4 vtable, reloc-masked). No
  CGruntzApp-specific field is set by the ctor.
- **CGruntzApp::Init @0x80930 = an OVERRIDE that forwards-then-bool-normalizes.**
  It is CGruntzApp's override of CGameApp slot +0x8 (`VirtualUnknownMethod03`),
  so it mangles `?VirtualUnknownMethod03@CGruntzApp@@UAEHPAXPAD11HHH@Z` (same name
  as the base — overrides keep the slot). `ret 0x1c` = 7 stack args; it re-pushes
  all 7 verbatim and tail-forwards to the **base** `CGameApp::VirtualUnknownMethod03`
  (`this` left in ecx untouched), then `return base(...) != 0;`. **The `!= 0` is
  the load-bearing detail: it emits the `neg eax; sbb eax,eax; neg eax` int→bool
  (0/1) idiom** — a bare `return base(...);` would omit it. The 7-arg re-push is
  the classic `[esp+0x18]`-walk (each push shifts esp +4, so the same `[esp+0x18]`
  literal reads the next-lower original arg). Declaring the override with the
  identical 7-arg signature + a qualified `CGameApp::` base call reproduces it byte
  for byte.
- **CGameApp::VirtualUnknownMethod09 @0x13dc70 (vtbl +0x20, 29 B) = the per-frame
  idle, and it is a GUARDED TAIL-CALL into the game manager.** Body:
  `if (m_240 && m_244) m_8->vtbl[+0x10]();`. The `&&` gives two `test;je tail` to a
  shared `ret`; the active-call is a **tail jump** `mov ecx,[m_8]; mov eax,[ecx];
  jmp [eax+0x10]` (void no-arg → no epilogue). To emit the `jmp [eax+0x10]` I
  modeled `m_8` (a `CGameResource*`) with four extra virtual slots so index 4
  (offset 0x10) is a real virtual (`PerFrameTick`); the placeholder slots 1..3 keep
  the index correct. **Adding virtuals to CGameResource did NOT regress the
  scalar-delete sites** (CloseResources `delete m_4/m_8`, FreeGameManager) — those
  use slot 0 regardless of how many slots follow. This is the REAL game tick the
  RunMessageLoop pump invokes on an empty queue (the +0x20 idle is now a body, not
  a stub) — its target `m_8->+0x10` is the next per-frame target one level down.
- **CGameApp::FreeGameManager @0x13dc90 (vtbl +0x24, 25 B) = `delete m_8; m_8=0;`**
  via the scalar-deleting dtor (`mov eax,[m_8vtbl]; push 1; call [eax]`) under a
  null-check, `this` spilled to esi. The existing inline `{}` stub at this slot
  became a real body (kept the 16-slot vtable order intact). The CGameApp `m_8`
  game-manager slot is now created (ctor), driven (idle tick), and freed
  (FreeGameManager) — the manager lifecycle is anchored on both ends.
- **UNLOCKED:** the CGruntzApp vtable @0x5e9ab4 now has slots 0 (dtor)/+0x8 (Init)/
  +0x18 (Run, inherited)/+0x30 (ShowError) populated and the ctor that installs it;
  the WinMain→ctor→Init→RunMessageLoop chain is matched end to end. The headline
  follow-on: **CGameApp slot +0x20 (`VirtualUnknownMethod09`) tail-calls
  `m_8->vtbl[+0x10]` — that game-manager slot is the REAL per-frame game idle/tick**
  and is the next dedicated target one indirection down (reach it via the now-named
  idle virtual). The two active gate words are pinned: **m_240/m_244 must BOTH be
  set for the idle to tick** (a run/active latch pair).

### CGameApp::RunMessageLoop — the Win32 game pump (extended `gameapp` — BYTE-EXACT, 100% fuzzy)
- Extended `src/Wap32/GameApp.cpp` + `Wap32.h`. `?RunMessageLoop@CGameApp@@UAEHXZ`
  @0x13d910 (159 B, **CGameApp vtable slot +0x18**, the slot WinMain dispatches to
  via `call [vtbl+0x18]`). **BYTE-EXACT**: 65/65 instructions equal, exactly **4
  diffs, all `DIFF_ARG_MISMATCH` on reloc-masked DIR32 IAT operands**
  (`mov edi,[TranslateMessage]`/`mov esi,[PeekMessageA]`/`mov ebx,[DispatchMessageA]`
  /`call [TranslateAcceleratorA]`); report.json scores it 100.000%. No new unit, no
  flag change (`gameapp` stays /GX); the 9 prior gameapp fns + gamewnd + the
  gruntzapp fns (which include Wap32.h) all held (gameapp fuzzy 99.056%, gamewnd
  99.757%; CloseResources/InitializeAccelerators/ReportError still 100%).
- **It is `PeekMessageA(PM_REMOVE)`, NOT `GetMessageA`** — the reloc table is ground
  truth: @0x13d92c→TranslateMessage(0x6c4428), @0x13d933→**PeekMessageA**(0x6c43c4),
  @0x13d93a→DispatchMessageA(0x6c43c0), @0x13d972→TranslateAcceleratorA(0x6c445c).
  The WinMain report's "GetMessageA @0x6c43c4" hint was WRONG (it decoded the same
  IAT slot as GetMessage); the 5-arg push shape (`push 1;push 0;push 0;lea &msg;
  push 0;push &msg`) = `PeekMessageA(&msg,0,0,0,PM_REMOVE)`, a 5-arg peek-pump, not
  the 4-arg blocking GetMessage. **Always trust the reloc symbol/arg-count over the
  decoder's guessed import name.**
- **Loop SHAPE = drain-then-idle, NOT a blocking pump:** `hwnd=(HWND)m_4->m_4; if
  (!hwnd) return 0; for(;;){ if(PeekMessage){ do{ if(msg.message==0x12/*WM_QUIT*/)
  return 1; if(m_10 && msg.hwnd==hwnd) TranslateAcceleratorA(hwnd,m_10,&msg);
  TranslateMessage(&msg); DispatchMessageA(&msg); }while(PeekMessage); } idle(); }`.
  The `for(;;){ if(Peek){ do{...}while(Peek);} idle;}` (outer peek gating an inner
  do-while peek, idle on empty) reproduces the exact two-peek CFG (LOOP_TOP peek
  `je idle`; tail peek `jne process`; `jmp LOOP_TOP` after idle).
- **The accel gate is `m_10 != 0 && msg.hwnd == hwnd`, and the accel call's return
  is IGNORED** (the prompt's "skip Translate/Dispatch when nonzero" is wrong — the
  bytes fall straight through from TranslateAcceleratorA into TranslateMessage with
  no test of its result). msg.hwnd (MSG+0x00) is compared against the cached HWND in
  `ebp`; WM_QUIT is `cmp [msg.message],0x12` (MSG+0x04).
- **The idle is `VirtualUnknownMethod09()` = CGameApp vtbl slot +0x20** (`mov ecx,
  [this]; mov edx,[ecx]; call [edx+0x20]`). RunMessageLoop took the existing slot
  +0x18 placeholder (was `VirtualUnknownMethod07`); renaming +0x18 to `RunMessageLoop`
  and keeping +0x20 as the idle virtual kept the 16-slot vtable order intact (verified
  no slot shift — all sibling fns held). **The HWND is reached via `m_4->m_4`**
  (CGameApp+0x4 = the CGameWnd, its +0x4 = the OS HWND), confirming the CGameWnd m_4
  layout. `this` is spilled to a stack slot at entry (`mov [esp+4],ecx`, reloaded as
  `[esp+0x10]`) — write it as a plain method (no `this` local needed; MSVC spills it).
- Return type = `int` (full `eax`: 0 on no-window, 1 on WM_QUIT), no-arg `__thiscall`
  → bare `ret` both exits → mangles `?RunMessageLoop@CGameApp@@UAEHXZ` (U=virtual,
  H=int, XZ=void). The `MSG` struct (28 B: hwnd/message/wParam/lParam/time/pt.x/pt.y)
  + the 4 USER32 `__stdcall` dllimports went into Wap32.h's existing import block.
- **UNLOCKED:** the main message loop is now matched end-to-end from WinMain
  (`WinMain → call[vtbl+0x18] → RunMessageLoop`). The **idle virtual CGameApp slot
  +0x20** (`VirtualUnknownMethod09`, the per-frame game-tick the pump calls on an
  empty queue) is now a named, caller-anchored target; the CGameWnd m_4→HWND chain
  and the USER32 PeekMessage/Translate/Dispatch/TranslateAccelerator IAT slots
  (0x6c43c4/0x6c4428/0x6c43c0/0x6c445c) are pinned.

### WinMain program entry (unit `winmain` — BYTE-EXACT under reloc-masking, 98.97% fuzzy)
- New TU `src/Gruntz/WinMain.cpp`. The WINAPI entry `_WinMain@16` @0x11c860
  (807 B). All **243 instructions instruction-count-equal with 0 opcode and 0
  size mismatches** (verified by diffing the objdiff instruction lists part-by-
  part); every residual diff is a reloc-masked DIR32/REL32 operand (IAT slots,
  the version/cmdline string globals, the engine call targets, the four version-
  int globals), so it is byte-exact. objdiff "exact" = 0/1 (same reloc-typing
  scoring artifact as the byte-exact `image`/`netmgr` units, which also show
  0/N). Built **/O2 /MT /GX** (the `new CGruntzApp` under a C++ EH frame needs
  the `fs:0` SEH/EH prolog — the target's `push -1; push &handler; mov fs:0`).
- **The carcass / call sequence (the deliverable):** (1) `GetModuleFileNameA`
  then the engine path-check `CheckExePath`@0x118ce0 (via thunk 0x2e6e, __cdecl
  `(path, 2, 0)`). (2) **THE PATH-CHECK BRANCH IS INVERTED from intuition:** when
  CheckExePath *succeeds* this is a secondary/lobby launch — `FindWindowA(
  "GruntzClass", 0x60aac8)`, if found restore-if-iconic (`SendMessageA WM_SYSCMD
  SC_RESTORE`) + forward a lobby `PostMessageA WM_COMMAND 0x80b7` when the cmd
  line contains "LOBBYLAUNCH" — and **this whole branch unconditionally
  `return 0`** (when no prior window exists it still returns 0; it does NOT fall
  through to startup). Only when CheckExePath *fails* (the `&&` short-circuits or
  GetModuleFileName==0) does it fall to the normal startup. Writing it as
  `if (modlen>0 && CheckExePath(...)) { hPrev=FindWindow; if(hPrev){...} return 0; }`
  reproduces the three `je <return0>` (hPrev==0 / cmdline==0 / LOBBYLAUNCH==0) and
  the trailing `return 0`. (3) Normal startup: read own FileVersion via the
  VERSION.DLL trio (GetFileVersionInfoSizeA/GetFileVersionInfoA/VerQueryValueA,
  6-byte IAT thunks @0x18b78c/786/780) into a heap buffer (`operator new` /
  `operator delete`), `sscanf`@0x120900 the "%d.%d.%d.%d" into four file-scope
  ints @0x651608/60c/610/614; a `StartupGate`@0x1f9b0 (thunk 0x2f59) check; then
  **`new CGruntzApp` (operator new(0x254) + ctor @0x80850 via thunk 0x26c1 under
  the EH frame)** stored to g_pApp@0x651600; a `SettleDelay`@0x13dfe0 busy-wait;
  the dev hot-key scan (3× `GetAsyncKeyState`, test bit 0x80000000) + 6× cmd-line
  `SubstringMatch`; an optional `DialogBoxParamA(g_hInstance, "CONFIG_ADVANCED",
  0, &AdvancedOptionsDialogProc, 0)`; then **app Init via `call [vtbl+0x8]`**
  (7 args: hInstance, "Gruntz"×2 @0x60aac8, cmdLine, 0, CW_USEDEFAULT×2) and
  **the message loop via `call [vtbl+0x18]`**, with `delete g_pApp` (scalar-
  deleting dtor) on every exit. **Locals**: a 0xFE-byte module-path buffer
  (`[esp+0x1c]`, captured by the prolog `lea eax,[esp+0xc]` *before* the 4 reg
  pushes), the VERSION query handles/out-pointers, and the EH-state slot
  `[esp+0x124]`; params at `[esp+0x12c]`(hInstance)/`+0x130`/`+0x134`(lpCmdLine)/
  `+0x138`(nShowCmd).
- **WHERE THE MAIN LOOP IS (the headline):** WinMain does NOT pump inline — it
  dispatches to **`CGameApp::RunMessageLoop` @0x13d910 (159 B, CGameApp vtable
  slot +0x18)** via `mov eax,[g_pApp]; mov eax,[eax]; call [eax+0x18]`. That fn
  is the classic Win32 pump: `GetMessageA` (IAT @0x6c43c4) loop, WM_QUIT(0x12)
  exit, accel `TranslateAcceleratorA` (@0x6c445c) gated on `app->m_4->m_4` (the
  accel table) + the WM_*==m_4 guard, `TranslateMessage`(@0x6c4428) /
  `DispatchMessageA`(@0x6c43c0), and on a GetMessage-returns-0 idle it calls the
  app idle virtual `call [vtbl+0x20]` then loops. **It is the next dedicated
  target** (RVA 0x13d910, size 159 B). The Init it calls first is CGameApp slot
  +0x8 (@0x80930 via thunk 0x30b2).
- **LEVERS that drove 93.8% → byte-exact:** (a) the **VERSION.DLL imports must be
  `__stdcall`** — declared plain (cdecl) the call sites emit a spurious
  `add esp,0x10` after VerQueryValueA (callee-cleaned in reality) and the sscanf
  arg interleave (`mov eax,[pValue]` between the 2nd/3rd push) breaks → 93.8%;
  __stdcall removes it → 95.9%. (b) **schedule `g_hInstance = hInstance;` AFTER
  the `if (!g_pApp) return 0;`** (not before the new) so the hInstance load lands
  in the g_pApp-nonnull continuation, interleaved with the `push 0x64`/SettleDelay
  call exactly like the target → 98.3%. (c) **bind the DialogBoxParamA result to
  a named `int` local** — `if (DialogBoxParamA(...) == 0)` inline emits
  `cmp eax,ebp` (reusing the ebp=0 zero-reg); `int r = ...; if (r==0)` emits
  `test eax,eax` matching the target → +0.25%. (d) **`SubstringMatch` takes
  (haystack, needle)** — the target pushes `needle` first then `cmdline`, i.e.
  arg1=cmdline; declaring `(needle, haystack)` swaps the two `push`es on all 7
  call sites → fixing the order to `(cmdline, token)` → **98.97%**. (e) the
  `new CGruntzApp` + `delete g_pApp` idioms are the standard MFC NAFXCW
  op-new-under-EH (`op_new(0x254); store EH-state 0; if(p) ctor; ...; EH-state
  -1; result`) and scalar-deleting-dtor (`mov edx,[p]; push 1; call [edx]`) —
  reuse them verbatim (see `??2@YAPAXI@Z`/`??3@YAXPAX@Z`, the slot-0 dtor).
- **UNLOCKED:** the program entry is anchored — `CGameApp::RunMessageLoop`
  @0x13d910 (the next target) + `CGameApp::Init`@0x80930 now have a caller; the
  CGruntzApp ctor @0x80850 (+ vtable @0x5e9ab4, slots +0x8 Init / +0x18 Run / 0
  dtor), the startup path-check @0x118ce0, the lobby-substring helper @0x120090,
  the resource gate @0x1f9b0, the busy-wait @0x13dfe0, and the sscanf wrapper
  @0x120900 all have callers; the four FileVersion-int globals @0x651608..0x651614,
  g_pApp @0x651600, and g_hInstance @0x651618 are pinned. The
  `AdvancedOptionsDialogProc` anchor was reused as a taken address (reloc-masked).

### CNetMgr DirectPlay manager — message handlers + config writer (unit `netmgr` — 4/4 byte-exact under reloc-masking)
- New TU `src/Net/NetMgr.{cpp,h}`. The engine **CNetMgr** (DirectPlay networking
  manager, leaked `C:\Proj\NetMgr`). All 4 fns instruction-count-equal + **zero
  structural diffs + zero non-reloc arg mismatches** (verified via the objdiff
  instruction list: every mismatch carries a `relocation` or a `reloc` arg part):
  OnMultiOptions@0x0badd0 **97.08%**, OnMultiPause@0x0bad40 **98.62%**,
  OnOutOfSync@0x0bae40 **99.41%**, ApplyCmdDelayDefaults@0x0b85a0 **99.02%**.
  Built `/O2 /MT /GX` (the config writer's three stack CString temps need the EH
  frame; the 3 handlers carry no stack C++ object so /GX adds no frame to them).
- **NO DirectPlay COM was needed for this cluster** — these are the *higher-level*
  CNetMgr state/message methods, not the raw COM marshalling. The headline for the
  COM-vtable worry: pick the state/accessor/setup methods first; they reach the
  engine via plain `call`/IAT, never `call [iface+slot]`. (When a COM-vtable fn IS
  unavoidable, model the interface as a struct whose first member is a vtable-ptr
  to a struct of fn-ptrs padded to the used slot, so `pIface->vtbl->Method()` emits
  `mov edx,[ecx]; call [edx+slot]` — reloc-masked. None used here.)
- **The multiplayer dispatcher takes a CALLBACK fn-ptr as its 2nd arg (the load-
  bearing shape).** Each handler does `MultiDispatch("MULTI_<CMD>", &Callback, 0)`
  where the dispatcher is the engine @0x4bc250 (reached via incremental-link thunk
  `0x4bc250`, `call rel32` reloc-masks) and the callback is a distinct engine fn
  (@0x4bda70/0x4bd850/0x4bddd0, each ALSO via a `jmp` thunk @0x4027fc/0x40113b/
  0x40301c — so the `push &callback` is a DIR32 reloc, masked). Model the callbacks
  as `extern "C"` **address-taken-only no-body functions** and the dispatcher as
  **`int __stdcall MultiDispatch(const char*, CallbackFn, int)`** — `__stdcall` is
  required so the call site has **NO `add esp,0xc`** (the target lets the callee pop;
  a cdecl decl emits the spurious caller-cleanup → arg-cascade). Return = a
  WM-message id compared against `0x4cc`/`0x4cd`.
- **OnOutOfSync's 3-way result test ⇒ `switch(r){case 0x4cc; case 0x4cd; default;}`**
  → the `sub eax,0x4cc; je; dec eax; je; <default>` ladder (same switch-strength-
  reduction idiom as CButeMgr's type getters). An if/else-if chain emits the wrong
  `cmp;jne` polarity. The shared-flag store (`g_sharedFlag@0x648ce0=0`) schedules
  *between* the `sub` and the first `je` (interleaved) — writing it as the stmt
  right after the dispatch call reproduces that placement.
- **PostMessage HWND chain = a 3-deep pointer walk** `((H*)((H*)m_4)->m_4)->m_4`
  (m_4@+0x4 → +0x4 → +0x4 = the engine HWND), then `PostMessageA(hwnd, 0x111
  /*WM_COMMAND*/, wParam, lParam)`; Pause/OutOfSync-0x4cc use `(0x80d7, m_1c)`,
  OutOfSync-default uses `(0x8023, 0)`. PostMessageA is a **dllimport __stdcall**
  decl → the `FF15 [IAT]` indirect form against the engine's cached USER32 slot
  @0x6c44c8 (reloc-masked vs `[__imp__PostMessageA@16]`).
- **The config writer reuses the already-matched `Utils::RegistryHelper` verbatim**
  (`#include "../Utils/RegistryHelper.h"`) — the game-manager singleton @0x64556c
  holds a `RegistryHelper*` @+0x38, and `reg->SetValueDword(name, value)` (already
  matched @0x139460 in the `registryhelper` unit, `__thiscall ret 8`) reloc-masks
  cleanly against the sibling unit's symbol. **A clean cross-TU reuse: call an
  engine method already byte-matched in another unit and the `call rel32` just
  reloc-masks** (no need to re-declare or re-match it). The value names are built
  `m_598 + "_CmdDelay"/"_Resend"/"_DynCmdDelay"` via the AFXAPI `operator+`@0x1b9f81
  (`__stdcall`, ret 0xc) into 3 stack CString temps; only 2 are written
  (m_5a4 under _CmdDelay, m_5a8 under _Resend) — **the `_DynCmdDelay` temp is built
  then discarded** (write it faithfully as a dead 3rd concat; all 3 dtors run).
- **CNetMgr LAYOUT pinned (the deliverable):** **+0x4** m_4 (→ HWND via +0x4→+0x4);
  **+0x1c** the WM_COMMAND wParam posted on resync; **+0x574** OnOutOfSync's
  per-instance reentrancy guard; **+0x584** a state word cleared on handler entry;
  **+0x598** a CString config value-name prefix; **+0x5a4/+0x5a8** the
  _CmdDelay/_Resend command-timing values. Globals: `0x648d08` (OnMultiOptions
  guard), `0x648d04` (OnMultiPause guard), `0x648ce0` (a shared flag cleared by all
  three). The game-manager singleton @0x64556c has a `RegistryHelper*` @+0x38.
- **UNLOCKED:** the CNetMgr message-handler entry points are anchored — the
  multiplayer dispatcher @0x4bc250 + its 3 command callbacks (@0x4bda70/0x4bd850/
  0x4bddd0) and the engine HWND-holder chain now have callers; RegistryHelper is
  confirmed reusable cross-TU; the rest of the CNetMgr cluster (SetupSession,
  SendChat, HandleSessionError, the DPLAYX/COM marshalling) can build on these
  member offsets.

### CGrunt animation-resolver cluster (extended `grunt` — 5/5 logic+CFG+offsets byte-exact; Death 97.1%, Generic 94.7%, Moving 92.0%, Idle 91.4%, Battlecry 89.6%)
- Extended `src/Gruntz/Grunt.{cpp,h}`. The 5 `CGrunt::Resolve*Animation`
  `__thiscall` methods (@0x045100 Moving / @0x0455f0 Death / @0x0457b0 generic
  "_JOY" / @0x045960 Idle / @0x045b60 Battlecry). Each builds an animation-KEY
  string **`"GRUNTZ_" + this->m_typeName + "_<CATEGORY>"`** (m_typeName is a
  CString @CGrunt+0x54), feeds the resolved geometry source into the grunt's
  animation player `m_38`, then does a single-char **`CButeTree::Find`** (the same
  @0x16d190 the ButeMgr getters use, but on a DIFFERENT global tree instance
  @0x6bf620 reached as `mov ecx,0x6bf620` not `lea ecx,[this+0x18]`) and caches
  the result into `m_14->m_1c`. Death/generic/Idle/Battlecry also fire a 5-arg
  on-screen "cue" gated on the grunt being inside the view rect. Category strings:
  `_MOVING`@0x60d220 / `_DEATH`@0x60d22c / `_JOY`@0x60d234 / `_IDLE`@0x60d36c /
  `_BATTLECRY`@0x60d374; prefix `GRUNTZ_`@0x60d28c; Find keys "B"/"C"/"E"/"A"/"F".
- **THREE source levers, in descending impact (all three are required):**
  1. **`AfxString` MUST have a user-declared `~AfxString()`** (the engine CString
     dtor @0x1b9cde). The key string is built from two stack CString temporaries
     (`"GRUNTZ_" + name` then `+ "_CAT"`); without a non-trivial dtor MSVC treats
     them as trivially destructible and emits **NO C++ EH frame at all** (the whole
     `fs:0` prolog/epilog + FuncInfo vanish → ~57%). Adding the dtor decl (no body,
     reloc-masked) makes the temps get destruction calls under the EH frame → 89%.
     (The unit therefore needs **/GX** — verified it does NOT add an EH frame to
     the 7 sprite creators, which carry no stack C++ object, so they stay byte-
     exact at their prior percentages.)
  2. **The two `operator+` overloads are `AFXAPI` = `__stdcall`, NOT `__cdecl`.**
     `CString operator+(LPCTSTR,const CString&)`@0x1b9ff5 and `operator+(const
     CString&,LPCTSTR)`@0x1b9f81 both `ret 0xc` (callee pops the hidden struct-
     return slot + both args). Declared plain (`__cdecl`, mangles `??H@YA…`) the
     compiler emits a spurious **`add esp,0xc` after each call** (caller-cleanup)
     and the stack-temp offsets cascade; `AfxString __stdcall operator+(…)` removes
     it → **+~3-5% across all five** (Death 95→97, generic 90.9→94.7, etc.). The
     by-value class return + the AFXAPI convention together give the exact
     `lea &temp; push arg; push lit; push &temp; call; (no add esp)` shape.
  3. **Visible-bounds gate ⇒ pin `int x=h->m_5c; int y=h->m_60;` locals.** The cue
     fires only if the grunt is in the view rect: `if(g->m_134==1){ if(m_5c<g->m_144
     && m_5c>=g->m_13c && m_60<g->m_148 && m_60>=g->m_140) cue(...);} else cue(...)`.
     Writing the field accesses inline re-reads m_5c/m_60 each compare; the target
     caches them in callee-saved regs (edx/edi, + ebp for m_144). Explicit `x`/`y`
     locals force MSVC to allocate those callee-saved regs (and emit the matching
     `push ebp`) → **+~6% on Death** (89→95). Same "pin a local to force the 4th
     callee-saved reg" family as ButeMgr::ParseTagLine. Both cue branches pass the
     SAME args (`m_10->m_188`, arg2, -1,-1,-1); only the GATE differs (the m_134!=1
     else-branch is unconditional). arg2: Death=m_ac, generic=const 0x435/0x43f,
     Idle=idx+0x431/0x43b, Battlecry=idx+0x42e/0x438.
- **CGrunt LAYOUT pinned by the resolvers (the deliverable):** **+0x14** anim-set
  lookup holder (`->m_1c` = resolved anim-set node); **+0x30** = old m_14->m_1c
  (saved before Find); **+0x38** the **animation player** (`SetAnim`@0x150540 1-arg
  `ret 4` / `SetAnimEx`@0x1504d0 2-arg `ret 8` on `this`; **`SetGeometry`@0x15c2d0
  `ret 4` on the sub-player at player+0x1a0**; player **+0x1b4** = active-anim
  descriptor, cached into **CGrunt+0x40** before the geometry call); **+0x54**
  CString m_typeName; **+0x58[]** Idle geometry sources, **+0x68[]** Battlecry geo
  sources, **+0x74** generic geo, **+0x78** death geo, **+0x7c** moving geo;
  **+0x88/+0x8c/+0x90/+0x94** Moving-only time/seed block (`m_90 = (rand()%0x5dc1 +
  0x1770)*10`, `m_88 = g@0x645588`, others 0); **+0xa8** the resolve gate / dirty
  flag (`if (m_a8) return 0;` — Death latches `m_a8=1`, Moving does NOT); **+0xac**
  the death-cue arg2. CGameRegistry (the @0x64556c singleton, already modeled):
  **+0x60** the cue sink (`Cue`@0x11b7c0 via thunk 0x33b4, `ret 0x14` = 5 args),
  **+0x134** the m_134==1 gate, **+0x13c/+0x140/+0x144/+0x148** the view rect
  (minX/minY/maxX/maxY); CGruntHud **+0x188** the cue arg.
- **Idle's unique frame-arg read:** `desc = m_38->m_1b4; elem = (desc->m_10>0)?
  *desc->m_c:0; frame = elem->m_14;` — the field read on the possibly-NULL `elem`
  is UNCONDITIONAL in the binary (`mov edi,[eax+0x14]` after the `xor eax,eax`
  fall-through), so write it as the bare ternary-then-read (a faithful UB) feeding
  `SetAnimEx(key, frame)`. Idle picks `idx = rand()%3 + 1` (1..3), Battlecry
  `idx = rand()%3` (0..2) — note Battlecry has NO `inc`.
- **The Moving resolver stores the time-block BEFORE the lookup; the others put
  the geometry-setter FIRST then the string-setter** (Moving is the only one that
  does `SetAnim` (string) before `SetGeometry`; Death/generic/Idle/Battlecry do
  `SetGeometry` then the string call). Match the per-function order from the dump.
- **PLATEAU residue (entropy-class, all 5):** (a) the `m_40 = m_38->m_1b4` store —
  target schedules it AFTER the `lea this` for the geometry setter (m_1b4 in edx,
  store after lea); mine puts m_1b4 in ecx so it stores before the lea. NO source
  lever flips it (tried: reorder store after the call → 92.8% WORSE since the
  target genuinely reads m_1b4 pre-call; a `geo` temp; an `anim` pointer local — no
  effect). Pure edx/ecx allocator coin-flip, same family as the 7 creators'.
  (b) Idle/Battlecry: the compiler hoists `idx+0xNNN` into a callee-saved reg at
  branch-entry (mine computes it at the cue site); a `cue` local made it WORSE.
  (c) the EH `fs:0` operands (FuncInfo/`__except_list`/`$L`) + the cascaded stack-
  temp esp offsets are reloc/alignment, not logic. Instruction counts equal; left
  per doctrine. objdiff "exact" = 0/12 (every call op is reloc-masked DIR32/REL32).
- **UNLOCKED:** the CGrunt animation-resolution path is anchored — the animation
  player methods (SetAnim@0x150540, SetAnimEx@0x1504d0, SetGeometry@0x15c2d0), the
  global animation-set tree @0x6bf620 (a second `CButeTree::Find`@0x16d190 caller),
  the on-screen cue sink @0x11b7c0 (registry+0x60, via thunk 0x33b4), and the
  CString `operator+` AFXAPI pair (@0x1b9ff5/@0x1b9f81) now have callers. The
  CGrunt member map grows by ~18 offsets (+0x14..+0xac) + the CGameRegistry view-
  rect/cue members (+0x60, +0x134..+0x148) for the rest of the (huge) CGrunt TU.

### CGrunt HUD sprite-creator cluster (unit `grunt` — 7/7 logic byte-exact; 5 @99.3%+, 2 @~91.6% 2-arg plateau)
- New TU `src/Gruntz/Grunt.{cpp,h}`. Seven contiguous `__thiscall` lazy HUD
  sprite creators on the engine **CGrunt** entity (@0x04d130..0x04d730:
  CreateHealth/Toy/Stamina/ToyTime/WingzTime/Powerup/Selected). All 7 are
  instruction-count-equal + structurally byte-exact; the residue is purely a
  register-allocation/scheduling coin-flip in the ~6-instr Add-call setup. Built
  plain **`/O2 /MT`** (NO /GX — no stack C++ object / no EH frame in any of them).
- **The creators DON'T `new` the sprite class** — the tomalla/decomp-xref labels
  ("creates GruntHealthSprite") are misleading. Each calls a **global HUD sprite
  FACTORY** `@0x1597b0` (`__thiscall`, `ret 0x18` = 6 stack args) reached via
  `(*(void**)0x64556c)->m_30->m_8` and passes the sprite **class-NAME string**
  (`"GruntHealthSprite"` etc., literal .rodata) + a hint int + two HUD-geometry
  values off `this->m_10` (`m_10->m_5c`, and `m_10->m_60` optionally **minus a
  per-sprite constant**: Health/Toy −0x19, Stamina/ToyTime −0x20, WingzTime −0x26,
  Powerup/Selected −0). The factory allocates+constructs the named sprite
  internally. So you need NEITHER the sprite ctor (@0x011ef0 etc.) NOR the sprite
  class layout — only the factory call shape + `sprite->m_7c` chain. The C arg
  order is `factory(0, m_5c, m_60[-N], hint, nameStr, 0x40003)` (push order is the
  reverse; 6 stack args, factory-`this` loaded into ecx LAST).
- **Shared shape**: gate (`if (slot || <stat-gate>) return 0;` — Health needs
  `m_3ec>0`, Stamina `m_3f0!=0x64`, ToyTime `m_3f4!=0`, WingzTime `m_238 && m_3f8`;
  Toy/Powerup/Selected gate only on the slot), build via factory → store into the
  CGrunt slot, run the sprite's init virtual `sprite->m_7c->[0x10](sprite)`, then
  **register** into `sprite->m_7c->m_18` (a registrar) via an Add*; on register
  FAIL: `registrar->m_38->m_8 |= 0x10000;` null the slot; `return 0;` else 1.
  ToyTime/WingzTime ALSO pre-clear sibling sprite slots (the same
  `((rec*)sprite)->m_8 |= 0x10000; slot=0;` shape on m_1c8/m_1d0 / m_1cc).
- **CGrunt sprite-pointer member offsets pinned (the deliverable):** selected
  **+0x1b8**, toy **+0x1bc**, health **+0x1c4**, stamina **+0x1c8**, toyTime
  **+0x1cc**, wingzTime **+0x1d0**, powerup **+0x1d4**; the HUD geometry source
  **+0x10** (→+0x5c/+0x60); the two Add* args **+0x1ec / +0x1f0**; the per-sprite
  stat/gate words **+0x238 (wingz gate), +0x3ec (health), +0x3f0 (stamina),
  +0x3f4 (toyTime), +0x3f8 (wingzTime)**.
- **THE LEVER for the 3-arg Add (the headline codegen finding):** the registrar
  `this` is `sprite->m_7c->m_18`, reused on the failure path (held in edi across
  the call) — so it MUST be a `reg` LOCAL (a fully-inline `sprite->m_7c->m_18->...`
  twice recomputes the `[+0x18]` load post-call → 77%). BUT the target computes the
  `[eax+0x18]` registrar load **interleaved between the arg pushes** (push c; push
  b; `mov edi,[eax+0x18]`; load a; push a), keeping `sprite->m_7c` **in eax**
  (in-place reuse of the reloaded-sprite reg). Writing `reg = sprite->m_7c->m_18;`
  directly puts m_7c in ECX and schedules the registrar AFTER all args (95.8%).
  Introducing an explicit intermediate **`CSpriteInner *inner = sprite->m_7c;
  CSpriteRegistrar *reg = inner->m_18;`** flips MSVC to the eax-in-place +
  interleaved schedule → **99.3%**. (Same family as ButeMgr's "pin a temp to steer
  the register/schedule"; here a *pointer-chain split* into a named intermediate is
  the lever.) Lone residue on the 3-arg fns = which of edx/ecx temporarily holds
  arg c vs arg b — push ORDER + VALUES byte-identical, only the temp reg-field
  differs; explicit `int c=..;int b=..;` temps did NOT flip it (pure allocator
  coin-flip, entropy-class).
- **2-arg Add (Toy/Selected) PLATEAU ~91.6%:** with ONLY two args there isn't a
  third arg to pin the schedule, so MSVC hoists arg a (m_1ec) into a reg BEFORE the
  registrar (target loads it AFTER) and puts m_7c in ecx not eax. Here the `inner`
  temp makes it WORSE (90.7%); the plain `reg = sprite->m_7c->m_18;` is best
  (91.6%). Six source forms (inner, sprite-local, preload-b, explicit a/b temps,
  full-inline, plain) normalize to one of two valid schedules; no lever flips the
  2-arg ordering. Logic/offsets/CFG byte-exact; left per doctrine. **Takeaway: the
  optimal register-chain lever is ARG-COUNT-dependent — the `inner`-split helps the
  3-arg shape but hurts the 2-arg shape; pick per-function, don't apply uniformly.**
- The factory (@0x1597b0) + the four Add* registrars (@0x07f0d0 3-arg /@0x07f920
  2-arg /@0x080380 3-arg(Powerup) /@0x07e9c0 2-arg) are reached via incremental-
  link **thunks** (the low-RVA `jmp` stubs at 0x2342/0x3f71/0x2c20/0x207c → real
  @0x7xxxx); modeled as external no-body methods so the `call rel32` reloc-masks
  (objdiff shows them as `thunk_FUN_*` vs my mangled name — benign).
- UNLOCKED: the HUD sprite-creation path is anchored — the global sprite factory
  `@0x1597b0` (`CSpriteFactory::CreateSprite`, reached via `*0x64556c->+0x30->+0x8`)
  and the 4 sprite-registrar Add* methods (@0x07f0d0/0x07f920/0x080380/0x07e9c0,
  on `sprite->m_7c->m_18`) now have callers in place; the 7 CGrunt sprite-pointer
  slots + the HUD-geometry member (+0x10→+0x5c/+0x60) are pinned for the rest of
  the (huge) CGrunt TU. `0x64556c` = the global game registry/manager pointer.

### CButeMgr .att/.bute attribute config-parser (unit `butemgr` — 11/11 byte-exact; 2 documented entropy residues)
- New TU `src/Bute/ButeMgr.{cpp,h}`. The engine **CButeMgr** — the attribute layer
  the WHOLE game reads entity stats through (GetInt alone has 117 callers, GetIntDef
  94, GetDwordDef 104). A two-level keyed store + a recursive-descent lexer/parser.
  Built **`/O2 /MT /GX`** — the getters use **args-in-registers + no ebp frames (the
  /O2 tell)**, NOT the /O1 push-memory-operand MFC shape (CButeMgr uses CString but
  is NOT MFC-`CObject`-derived; the /GX is only for ParseTagLine's `new`-under-EH
  frame and the GetString function-local static's atexit dtor). All 11 fns equal
  instruction count + 0 non-reloc body mismatches (verified vs dump_target.py +
  llvm-objdump -dr); fuzzy 94–99.5% = reloc-masked operands only.
- **CButeMgr LAYOUT (pinned from the getters/parser):** `+0x08` int m_lineNo (the
  `%d` in error msgs); **`+0x18` the store root `CButeTree` m_tree** (getters do
  `lea ecx,[this+0x18]; Find(tag)`); `+0x44` void* m_pNode (last node ParseTagLine
  created); `+0xa4` void* m_pText (a text accumulator; the parser appends at
  `*m_pText + 0xc`); **`+0xa8` char m_curChar** (lexer current char); **`+0xaa`
  short m_tokType** (token type); **`+0xae` char m_token[]** (token buffer, indexed
  by the FILE-SCOPE counter @0x6bf678, not a member); `+0x100` CString m_tagName;
  `+0x10c`/`+0x10d` two parser flag bytes (m_10c = echo-to-accumulator enable;
  m_10d = suppress-duplicate-tag-check).
- **THE SHARED TAG-LOOKUP HELPER = `CButeTree::Find` @0x16d190** (__thiscall, `ret
  4`, returns the matching record ptr or null). The store is **two-level**: every
  getter does `m_tree.Find(tag)` → a per-tag sub-tree, then `subtree.Find(key)` → a
  typed value record **`{ int type; void* pValue; }`** (type @+0, value-ptr @+4; the
  scalar value sits at `[pValue]`, the string char* IS `[pValue]`). Insert =
  @0x16db90 (__thiscall(this,key,node)). Modeled both Find/Insert as a tiny
  data-less `CButeTree` class addressed at `this+0x18` (a `char m_treeRaw[...]` +
  `reinterpret_cast` accessor — a data-less class member would be size-1 and break
  the +0x44 offset; the raw-bytes-then-cast trick keeps every field at its RVA).
- **TYPE-CHECK CODEGEN IS THE LOAD-BEARING LEVER (the getter family):** the type
  tag is checked differently per constant and the polarity dictates block layout —
  - **type `== 0`** (GetInt/GetIntDef) → write `if (rec->type == 0) return v;` →
    `cmp [eax],0; jne mismatch` (success INLINE, mismatch jumped-to-tail).
  - **type `== 1`** (GetDword/GetDwordDef) → write **`switch (rec->type){ case 1:
    return v; }`** → `mov ecx,[eax]; dec ecx; je success` (success AT TAIL, mismatch
    inline). An `if (==1)` emits `cmp [eax],1; jne` (wrong polarity, ~57%); the
    `switch` strength-reduces to load+dec+jz and floats the case body to the tail.
  - **type `== 4`** (GetString/GetStringDef) → `if (rec->type == 4)` (like type 0).
  - **multi-type (GetFloat 0|3, GetDouble 0|2)** → **`switch (rec->type){ case 0:…;
    case N:…; }`** → `mov ecx,[eax]; sub/cmp ladder` (loads type ONCE into a reg).
  Rule of thumb: **single non-zero constant or a multi-way test ⇒ `switch` (loads to
  reg, case-at-tail); `== 0` / `== 4` ⇒ `if` (cmp-mem, success-inline).**
- **THE 3-WAY ERROR LAYOUT (no-default getters) ⇒ NESTED-IF, not early-returns.**
  The target lays the three failure blocks (type-mismatch INLINE as the switch
  fall-through, then symbol-not-found, then invalid-tag) at the function TAIL in
  that order, reached by forward `je`. Writing `if(!grp){err;return} ...` early-
  returns emits each error INLINE via `jne`-skip (21–50%). Writing it as
  **`if (grp) { rec=Find(key); if (rec) { <type check> err1; return E; } err2;
  return E; } err3; return E;`** (success deepest, errors as the trailing else
  cascade) floats the cold blocks to the tail → 99%+. (Same family as the
  AdvancedOptions switch-tail idiom.) `goto`s to bottom labels did NOT help — MSVC
  inlines them; the nested-if-with-success-deepest is what flips it.
- **GetString's MFC function-local `static CString empty` magic-static:** a
  function-local `static AfxString s_empty("");` emits the exact one-shot guarded
  init: `mov al,1; mov cl,[guard]; test al,cl; jne skip; <or guard bit>; mov [guard];
  call CString::CString(lit); push dtorThunk; call atexit; skip:`. Return the empty
  on errors via `(char*)&s_empty` (the CString object address = its char* @+0).
  PLATEAU 98.0%: a pure **tag↔key ebx/edi register-alloc coin-flip** — the static
  block touches `bl`, and the target reuses ebx for `tag` after it while my build
  picks edi; logic/CFG byte-exact, entropy-class (no source lever flipped it).
- **ParseTagLine @0x1711b0 (96.9%, 1-instr residue):** `if(!ScanToken(4)) return 0;
  m_tagName = m_token; if(!m_10d){ if(tree.Find(tok)){report dup;return 0;}
  node = new CButeNode(desc,2); m_pNode=node; tree.Insert(tok,node);} return
  ScanToken(3);`. The new node carries a C++ EH frame (free-on-unwind). **Register
  alloc needed a hint**: pinning `char* tok = m_token;` + `CButeTree* t = Tree();`
  as locals reused across Find+Insert made MSVC allocate the 4th callee-saved reg
  (ebp, used as a GP reg under /O2) exactly like the target — without the hint the
  optimizer dropped ebp and the whole ebx/ebp/edi assignment + EH-slot offsets
  shifted (84%→97%). The lone residue = an MSVC5 **EH-epilogue bool-return
  normalization** (`test al,al; setne al` on the final ScanToken result) that no
  source form reproduced (`!= 0` gave `neg al`; temp-bool/direct-return omit it).
- **Two-vtable node construction (the `new CButeNode` inline-vbase idiom):** the
  target does `op_new(0x2c); ctor(desc,2); mov [node],vtblA; mov [node+8],vtblB`
  with the **two vtable stores INLINE at the new-site** (a multiply-derived node).
  Reproduce by modeling CButeNode `: public CButeNodeBase` where the base ctor is
  external/no-body (the engine @0x16dff0, __thiscall) and the **derived ctor is
  INLINE** doing `m_vtblA = &g_nodeVtblA; m_vtblB = &g_nodeVtblB;` — `new
  CButeNode(...)` then emits op_new + the null-check + base-ctor call + the two
  inline DIR32 vtable stores, exactly. (An external/no-body single ctor puts NO
  inline vtable stores; an empty base subobject is size-0 so m_vtblA stays @+0.)
- **Parse @0x1704c0 (94.2%, the recursive-descent lexer):** a 6-entry jump-table
  `switch` on the char-class. **Read the table from the EXE** (file-offset map at
  VA 0x5706a4): case0=bad-symbol(return 0), case1/2=value-char (ReadValue + opt
  token-store + opt accumulator-append + NextChar + LOOP), case3/4/5=identifier
  (ReadIdent + opt store/append + NextChar + recurse-if-`m_tokType==0` + 0-terminate
  + return 1). **The `cls > 5` bounds test IS the switch's own range-check — do NOT
  write a separate `if (>5) continue;`** (that emits a DUPLICATE `cmp;ja`); just
  `switch(cls)` with no default + cases 1/2 `break` (loop) + the rest `return`, so
  `>5` falls through the switch to the loop top. The token counter is a **file-scope
  signed `short` @0x6bf678** (`movsx` reads + `inc word`); `m_token[g_tokenLen++]`.
  Append target = `((CButeText*)((char*)m_pText + 0xc))->AppendChar(c)` (@0x192060,
  __thiscall). Lex helpers PeekClass@0x170400 / ReadValue@0x170430 / ReadIdent@
  0x170460 are __thiscall(this, kind, c) (kind starts 0x11, reassigned by ReadValue);
  NextChar@0x170390 __thiscall(this). Residue = a single trailing alignment `nop`
  before the jump table (inter-function pad; the body is byte-exact).
- **UNLOCKED:** the entire .att attribute-read path is now verifiable end-to-end —
  every `LoadAttributes`/object-stat reader (CGrunt, towerz, bridgez, etc.) that
  calls Get{Int,Dword,Float,Double,String}[Def] has its leaf getters byte-exact;
  CButeTree::Find/Insert @0x16d190/0x16db90, the lex helpers @0x1703xx-0x1704xx, the
  node ctor @0x16dff0, the CString helpers, and the error reporter @0x1706c0 are all
  anchored callees for the rest of the CButeMgr cluster (file load/save, GetTypedRef
  tag5-8 @0x173770/0x173d00/0x174240/0x1747c0).

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

### RezMgr path/key builders (extended `rezmgr` — +1 byte-exact, +1 documented plateau)
- Added `?MakeImageKey@RezMgr@@QAEHPAXPAD0@Z` @0x13e5d0 (**BYTE-EXACT**, 177/177
  bytes, 0 raw mismatches after trimming the 12 trailing-nop pad — verified via
  `llvm-objdump` byte-compare, not just the 99.32% fuzzy) and
  `?MakeRezPath@RezMgr@@QAEHXZ` @0x091670 (**PLATEAU 91.87%**, EH/CString
  entropy). The class is the engine's **CGruntzMgr** (engine_labels), modeled as
  `class RezMgr` here (names are placeholders); these two fns own the
  archive-path / image-key building. The 4 prior rezmgr fns did NOT regress.
- **`MakeImageKey(arg1, name, arg3)` is an extension DISPATCHER, not a key
  concatenator** (the tomalla "key builder" label is loose): `ext =
  strrchr(name,'.')` (engine `_strrchr` @0x120680), then a `stricmp` (engine
  `_stricmp` @0x11fdf0, 0==match) `else-if` ladder on **`.BMP`@0x61a0e4 →
  `.PCX`@0x61a0dc → `.PID`@0x61a0d4** (that is the in-code order; the prompt's
  label listed them PCX/BMP/PID). Each match hands off to a __thiscall loader
  (`LoadBmp`@0x144110 / `LoadPcx`@0x145110 both `ret 8` (arg1,name);
  `LoadPid`@0x145cd0 `ret 0xc` (arg1,name,arg3)). Returns 1 on no-ext / no-match
  / loader-success; returns 0 only when an ext matched but its loader returned 0.
  The source shape `if (ext && stricmp(ext,X)==0) { if(!Load(...)) return 0; }
  else if ...; return 1;` reproduces the per-branch **re-test of `ext != 0`**
  (`test esi; je commonRet1`) and the shared `mov eax,1` tail; the `return 0`
  reuses the dead loader-result reg (no `xor eax`). The searched string is the
  **2nd** arg (`mov ebx,[esp+0xc]` after one push), not the 1st.
- **`MakeRezPath()` layout facts** (RezMgr/CGruntzMgr): **`+0xec` CString
  m_pathA** (assembled archive path #1), **`+0xf0` CString m_pathB** (path #2),
  **`+0xf4` m_inGameDir** (= `GetGruntzDriveLetter()@0x8fa70 == cwd[0]`), **`+0xf8`
  m_haveRez**, **`+0xfc` m_haveMoviez**. Algorithm: `GetCurrentDirectoryA(0xff,
  cwd)` (→0 on fail); `m_inGameDir = drive==cwd[0]`; **main REZ**: `found=1`
  default, `Format(&m_pathA,"%s\\%s",cwd,"Gruntz.REZ")`, if `!FileExists` then
  if `drive` retry `"%c:\\DATA\\%s"` (set m_haveRez=1) else `found=0`; **FEC**:
  build `"Gruntz.FEC"`@0x611044 + `"GruntzLo.FEC"`@0x611034, COW-copy-pick
  `fec = g_lowDetail ? Lo : Hi` (CString copy-ctor @0x1b9ba3), `Format(&m_pathB,
  "%s\\%s",cwd,fec)`, retry `fecHi` when `!m_inGameDir && !FileExists && !g_low`,
  then `"%c:\\MOVIEZ\\%s"`@0x611024 when not yet found && drive (set m_haveMoviez);
  if `!found` `ReportError(0x800b,0x43e)@0x8dc60` + return 0 else return 1. The
  **low-detail/front-end-class global @0x6455d4** gates Lo-vs-Hi FEC. CString
  helpers: ctor(lit) @0x1b9d4c, copy-ctor @0x1b9ba3, dtor @0x1b9cde, and the
  cdecl `Format(CString*,fmt,...)` wrapper @0x1b2cf5 (defers to @0x1b2a4f).
  FileExists = `0x1189c0` (the winapi one, via incremental-link thunk).
- **`found` flag defaults to 1, set to 0 on failure** (NOT 0-then-set-1): the
  target `mov $1,[esp_flag]` early then `mov [esp_flag],0` only on the
  not-found tail — writing `found=0` first then 5 conditional `=1`s cascades the
  branch polarity and the flag's init (`je`↔`jne`); the default-1 form took
  87%→92%. (Same default-then-override idiom as InitializeDefaultCreateStruct.)
- **PLATEAU residue = a single MSVC5 EH-state-tracking write.** The target
  advances the C++ EH state to 0 inline (`mov [esp+ehstate],ebp`) **right before
  the first CString::Format**, just after the `m_haveRez=0` store; my build emits
  the same `m_haveRez=0` (`mov [esi+0xf8],ebp`) but OMITS that one inline
  EH-state write, shifting alignment by one instr and cascading objdiff's
  edit-distance (the rest re-syncs — it's a pure off-by-one). This is the MSVC5
  EH-state scheduling over four overlapping CString live ranges; no source lever
  reproduces a single funclet state-write without forging the exact funclet
  table. Entropy-class; left per doctrine with the full logic in place.
- UNLOCKED: the archive-resolution entry is now mapped — the BMP/PCX/PID image
  loaders (@0x144110/0x145110/0x145cd0) and `Image::LoadFromRez`@0x175a90 (a .PID
  consumer) have MakeImageKey as their dispatcher anchor; the CGruntzMgr archive
  path members (+0xec..+0xfc) + `GetGruntzDriveLetter`/`ReportError` call surface
  are pinned for the rest of the CGruntzMgr cluster.

### Image resolution path (unit `image` — 4/4 BYTE-EXACT)
- New TU `src/Image/Image.cpp` (+ `.h`). The REZ→image load path. All 4 fns
  byte-exact (verified **0 non-reloc mismatches**, instruction counts equal:
  97/106/106/95; fuzzy 99.3–99.95%, residue = reloc-masked operands only).
- **`Image::LoadFromRez` @0x175a90 (238 B, __thiscall ret 0xc) = a SECOND ext
  dispatcher, NOT a RezMgr resolver** (the tomalla "resolves a .PID via RezMgr"
  label is wrong — the bytes have NO RezMgr/CFileIO call, only strrchr+stricmp).
  Modeled as `CImage::LoadFromRez(char* name, void* a2, void* a3)`. Same idiom as
  `MakeImageKey`: `ext = strrchr(name,'.')` (engine `_strrchr`@0x120680), then a
  `_stricmp`@0x11fdf0 ladder on **`.BMP`@0x61a0e4 → `.PCX`@0x61a0dc →
  `.RID`@0x624278 → `.PID`@0x61a0d4** (note the **4th extension `.RID`@0x624278**
  — a new string, not in MakeImageKey's 3-way ladder), forwarding **(name,a2,a3)
  verbatim** to one of FIVE sibling __thiscall loaders (all `ret 0xc`):
  LoadBmp@0x175e40 / LoadPcx@0x176190 / LoadRid@0x176310 / LoadPid@0x1766a0 /
  LoadDefault@0x1767d0 (no/unknown ext). Source shape
  `if(ext && stricmp(ext,X)==0) return LoadX(...); else if ...; return Default(...)`
  reproduces the per-branch **re-test of `ext != 0`** (`test esi; je default`) and
  the tail-call shape exactly. The 5 siblings declared external/no-body (reloc-
  masked). Sizes for a future Image-loader worker: 435/294/294/294/100 B.
- **The prompt's loaders @0x144110/0x145110/0x145cd0 are a DIFFERENT class from
  the dispatcher** — they `mov ebx,ecx` and run on a file-backed image class
  (modeled `CFileImage`), NOT the CImage that owns LoadFromRez (whose siblings are
  0x175e40…). Two image classes; names are placeholders, layout-free here (these
  3 fns touch no `this` fields — `this` is only the decode-helper receiver).
- **`CFileImage::LoadBmp`@0x144110 / `LoadPcx`@0x145110 (342 B, ret 8) are the
  SAME function** (byte-identical save the FuncInfo reloc + the decode-helper call
  target): `CFileIO f; if(!f.Open(path,0,0)) return 0; len=f.GetLength(); if(!len)
  return 0; buf=operator new(len); if(!buf) return 0; if(f.Read(buf,len)!=len)
  {delete buf; return 0;} r=DecodeX(name,buf,len); delete buf; return r;`. The
  CFileIO stack object's dtor runs on EVERY exit (inlined Close under a C++ EH
  frame ⇒ **/GX**). **`name`(arg1)→the decode key; `path`(arg2)→the file opened**
  (Open reads arg2, Decode reads arg1) — two distinct args even though LoadFromRez
  passes the same string to both slots.
- **`CFileImage::LoadPid`@0x145cd0 (304 B, ret 0xc) is the same but (1) OMITS the
  `len==0` guard** — it `operator new(len)`s unconditionally and only null-checks
  the allocation (so write LoadPid WITHOUT the `if(len==0) return 0;`); **(2) the
  decoder takes a 4th pass-through arg** `DecodePid(name,buf,len,a3)` (`ret 0xc`).
  Decoder sizes (deferred, big loops): DecodeBmp@0x143fc0=322 B, DecodePcx@0x144ee0
  =**549 B**, DecodePid@0x145b10=437 B — the actual format parsers, for a dedicated
  worker. Declared external/no-body here (reloc-masked).
- **Reused the already-matched CFileIO class verbatim** (`#include "../Io/
  FileStream.h"`); added one external no-body decl `CFileIO::GetLength()`@0x1bf505
  (the virtual-Seek-to-end length probe) to FileStream.h so CFileImage's call to it
  reloc-masks (FileStream.cpp doesn't call it, so no body needed — the clean way to
  expose an unmatched engine method to a sibling TU). The global `operator new`
  @0x1b9b46 (MFC NAFXCW new-handler loop) / `operator delete`@0x1b9b82 link as
  `??2@YAPAXI@Z`/`??3@YAXPAX@Z`; plain `new`/`delete` would add a null-check/ctor —
  use the explicit `operator new(len)` / `operator delete(buf)` *function* forms to
  get the bare `push len; call ??2; add esp,4` shape.
- UNLOCKED: the image-format decode cluster is now anchored — the 3 decoders
  (0x143fc0/0x144ee0/0x145b10) and the 5 CImage siblings (0x175e40/0x176190/
  0x176310/0x1766a0/0x1767d0) have their dispatchers/wrappers in place; `.RID`
  @0x624278 is a newly-identified 4th image extension. The CFileIO file-I/O path
  is confirmed end-to-end through a higher-level consumer (Open/GetLength/Read all
  link + match from CFileImage).

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

### WWD plane reader + level-load driver (unit `wwdfile` ReadPlane 99.19%; unit `gamelevel` LoadWwd carcass ~56%)
- **`WwdFile::ReadPlane` @0x15d8d0 (195 B, `__thiscall ret 0xc`) — 99.19% fuzzy,
  byte-exact modulo 11 reloc-masked operand bytes + ONE 6-byte 2-instruction MSVC5
  scheduling swap** (an entropy-class residue per doctrine). Mangled
  `?ReadPlane@CGameLevelPlanes@@QAEPAVCPlane@@PAX00@Z`. Body: `CPlane* p = new
  CPlane(this->m_field0c, this->m_planeCount, 0)` (operator new(0x158) under a C++
  EH frame); `if (!p->Read(planeData, blockBase, &this->m_planeCtx)) { if(p)
  p->dtor(1); return 0; }`; `this->m_planes.SetAtGrow(this->m_planeCount, p)`; if
  `p->m_flags & 1` (MAIN) cache `m_mainPlane=p; m_mainIndex=m_planeCount-1`; return p.
- **THE load-bearing trick: the `new`d engine class MUST be padded to its REAL size**
  so `operator new` pushes the right constant. CPlane is 0x158 (344) B — a thin shell
  emitted `push 0xc` (sizeof the stub) and desynced everything from the alloc onward;
  padding the shell to `[0x158]` flipped it to `push 0x158` and the whole body fell
  into place. Model unmatched engine classes you `new` at FULL size, not minimal.
- **Vtable-slot virtuals reproduce `mov eax,[this]; call [eax+slot]` for free**:
  declare N dummy virtuals so the real method lands at the right vtable offset
  (CPlane's block reader is slot 10 = +0x28, the scalar-deleting dtor is slot 1 =
  +0x4). Calling `p->Read(...)` / `p->dtor(1)` then emits the exact indirect calls,
  reloc-masked. Same pattern works for CGameLevel's own vtable (LoadWwd is slot 0x38,
  the pre-load `Reset` is slot 0x44 = call `[eax+0x44]` at entry → 17 placeholder
  virtuals before Reset).
- **The one residue MSVC won't yield on**: target stores `m_mainPlane=p` BEFORE
  loading `m_planeCount` for the `-1`; the rebuild always schedules the member load
  first (independent addresses, overlaps the store). Tried single-stmt, split
  `=count; -=1`, explicit local — all keep load-first. 6 bytes, identical logic =
  documented plateau.
- **MFC `CArray::SetAtGrow` @0x5b5822 (`__thiscall ret 8`)**: `m_data@+0x4`,
  `m_size@+0x8`; grows (`SetSize(index+1,-1)` @0x5b5653) if `index>=m_size` then
  `m_data[index]=value`. CGameLevel's plane CArray sits @+0x34 so `m_data@+0x38`,
  `m_size@+0x3c` == the plane COUNT (one field, two roles). Image-set CArray @+0x48.
- **`CGameLevel::LoadWwd` @0x15d280 (633 B, `__thiscall ret 0x4`, vtable slot 0x38)
  — CARCASS ~56%** (unit `gamelevel`, /O2 /GX, `src/Gruntz/GameLevel.cpp`). FULL
  faithful control flow + member offsets recovered: `Reset()` (vtbl +0x44) → `if
  (*(u32*)hdr > 0x5F4) return 0` → `m_header(+0xE0) = *hdr` (rep movs 0x17d dwords =
  1524 B) → if `hdr->flags & 2` (COMPRESS): `operator new(mainBlockLength +
  wwdSignature + 0x40)`, `WwdFile_InflateMainBlock(hdr, buf, size)`, free-on-fail
  (the buffer is tracked by the C++ EH state so it unwinds → /GX) → `strcpy(m_levelName
  +0x6C, hdr->levelName)`; `m_flags(+8)=hdr->flags`; `m_checksum(+0xAC)=hdr->checksum`
  → plane loop `for i<numPlanes` cursor `block+planesOffset` stride 0xA0 calling
  `ReadPlane(cursor, block, &m_planeCtx)` → image-set loop on `tileDescriptionsOffset`
  (`ReadImageSet`@0x15d820 factory → vtbl +0x24 stride advance → `m_imageSets.SetAtGrow`)
  → per-plane float coord scaling (`fild startX/startY; if !(plane->flags&1) fmul
  plane->m_scaleX/Y(+0x18/+0x1c); fstp plane->m_scaledX/Y(+0x10/+0x14); RecomputePlaneCoords`
  @0x161c90) over the main plane (+0x5C) then every plane skipping m_mainIndex(+0x60).
- **Why LoadWwd stays a carcass, not a plateau**: an MSVC5 register-allocation
  divergence the source can't steer — the TARGET keeps `this` in **ebp** and the
  header/block pointer in **ebx** (callee-saved, lives across the inflate calls),
  while the rebuild assigns `this`→ebx and spills differently (frame `sub esp,0x8`
  vs the target's `sub esp,0xc` with `hdr` spilled to `[esp+0x20]`). The swap
  cascades through nearly every instruction → low fuzzy despite identical logic.
  Kept the faithful source + the symbol_names row (objdiff pairs/tracks it) but the
  unit is `wip` and the function is NOT claimed byte-exact.
- UNLOCKED: the WWD level-load path is now end-to-end in source — ReadPlane is the
  per-plane fan-out into the plane sub-readers (the tile/imageset/object readers
  0x55e130/0x562010/0x5615a0 are the CPlane::Read internals), and LoadWwd is the
  orchestrator. NEXT dedicated target: **`WwdFile::ReadPlaneObjects` @0x162af0
  (2054 B)** — the per-object record loop (WwdObjectRecord 0x11C). The plane
  sub-readers + CImageSet factory variants are the other newly-anchored callees.

### CMapMgr level/map manager (unit `mapmgr` — 7/7 byte-exact; 2 Allocate deferred)
- New TU `src/Gruntz/MapMgr.cpp` (+ `.h`). The engine **CMapMgr** (the level/map
  manager), self-located via its RTTI vftable `??_7CMapMgr@@6B@` @0x5ea3b4. Built
  `/O2 /MT /GX` (engine code; the member sub-objects with dtors put a C++ EH frame
  around the ctor/dtor). 7 fns byte-exact (verified equal instr counts + ZERO
  non-reloc structural diffs; the 98–99% fuzzy = reloc-masked operands only:
  FuncInfo `$L`, `fs:[__except_list]`, the vftable, the operator-new/delete calls,
  and the sub-object ctor/dtor + Reset call targets).
- **CMapMgr LAYOUT (≥0x60 B), pinned from the ctor (@0x09e940) + dtor (@0x09e9e0)
  + slot-0 Reset (@0x09ec30):** vftable@+0 (0x5ea3b4, **6 slots**: slot0=Reset
  @0x09ec30, slot1 @0x09f7f0, slot2 @0x09f840, slot3 @0x09f9a0, slot4 @0x09eca0,
  slot5 @0x4853f0 — a shared/base method); **m_4@+4 / m_8@+8** = two heap ptrs the
  Reset `operator delete`s; +0xc/+0x10/+0x18/+0x1c = 0; **a 0x0c-byte array
  sub-object @+0x30 (CMapArrayA)** and **another @+0x3c (CMapArrayB)**; +0x4c=0,
  **+0x50=-1**, +0x58=0, **+0x5c=1**.
- **The dtor is non-trivial: it restores the vftable, calls the slot-0 virtual
  Reset INLINE (`call ?Reset` direct), THEN the two member sub-object dtors run on
  unwind.** Reset itself frees m_4/m_8 and resets the two arrays — so the arrays are
  destroyed TWICE (once by Reset, once by the dtor's member-teardown), which is safe
  because each array dtor frees-then-zeroes (idempotent: the 2nd call sees a null
  ptr and no-ops). Reproduced by: `~CMapMgr(){ Reset(); }` with A/B as real member
  objects that have dtors + `Reset()` calling `m_colA.~CMapArrayA(); m_colB.~...()`
  explicitly. objdiff mislabels my direct `call ?Reset` as "vector_deleting_
  destructor" — it is the same `call rel32`, byte-exact.
- **The two embedded arrays are DISTINCT 0x0c-byte classes with different heap-ptr
  offsets:** CMapArrayA holds its block at **+0x04** (ctor zeroes +4,+0,+8 in that
  order; dtor frees +4), element stride **0x24** (next@+0x14, prev@+0x18);
  CMapArrayB holds its block at **+0x00** (ctor zeroes +0,+4,+8; dtor frees +0),
  element stride **0x0c** (data@+0, prev@+4, next@+8). Both ctors are 100% byte-
  exact; both dtors 99.6% (the one MapFree call reloc-masked). Modeled `operator
  new`/`delete` as external `extern "C"` no-body (0x1b9b46 / 0x1b9b82) so the
  `push len; call; add esp,4` / `push p; call; add esp,4` shapes fall out reloc-
  masked (the standard external-callee idiom).
- **vftable emitted in-TU via the CGameWnd idiom**: declared CMapMgr with all 6
  virtual slots (slot0 Reset matched; slots 1–5 are out-of-line empty stubs, not in
  symbol_names.csv) so `cl` materializes `??_7CMapMgr@@6B@` in this object and the
  ctor's `mov [esi],OFFSET vftable` reloc-masks against it.
- **Allocate@CMapArrayA @0x09e740 / Allocate@CMapArrayB @0x09e860 DEFERRED (~70% /
  ~64%, NOT claimed in symbol_names.csv).** Both are doubly-linked free-list
  builders (`block=op_new(count*stride); m_block=block; first.prev=0; for each e:
  e.prev = (e==first)?0:e-1; e.next=e+1; last.next=0;`) — every offset/stride/value
  /branch is correct, but the residue is a **pervasive MSVC5 register-allocation +
  strength-reduction divergence**: the TARGET strength-reduces the loop induction
  onto `eax = &elem.prev` (computing the elem base as `eax-stride+...` and indexing
  the prev/next stores off eax) and uses NO held-zero register (`test reg,reg` +
  immediate `mov dword,0` stores), whereas our (compiler-chosen) codegen keeps the
  element pointer in ecx and caches 0 in ebx (`cmp reg,ebx` + `mov [..],ebx`). Also
  the alloc-fail path: ColA's target returns the null block ptr directly (no `xor
  eax,eax` — reuse the loaded null, like FileExists) while ColB's emits `xor
  eax,eax` (it already has a zero reg) — so `return (int)block;` vs `return 0;` is
  per-function. **Four source forms (raw-offset cursor walking the prev field,
  element-struct pointer walk, a cached `m_block` local, ptr-cast indexing) ALL
  normalized to the identical ecx-relative codegen** — MSVC's strength-reduction
  picks the induction register from the loop data-flow, and no index-expression
  rewrite flips it. Entropy-class (same family as the CRezDir-ctor 78% and
  MakeRezPath 92% plateaus); deferred — the strides + link layout, the deliverable,
  are fully recovered. A future worker could revisit with a different opt level or
  by forging the exact loop shape.
- UNLOCKED: the CMapMgr vftable is now laid out (6 slots), so any caller that
  dispatches through it is anchored; the +0x30/+0x3c embedded-array classes
  (ctor/dtor/Allocate @0x09e7xx/0x09e8xx) and the slot-0 Reset cleanup are mapped;
  vtable slots 1–5 (0x09f7f0 / 0x09f840 / 0x09f9a0 / 0x09eca0 / 0x4853f0) are the
  next CMapMgr methods for a follow-up worker.

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

### CGruntzApp dtor + ShowError (extended `gruntzapp` — 4/4 byte-exact, reloc-masked)
- Added `??1CGruntzApp@@UAE@XZ` @0x0808b0 (**98.62%**, 24/24 instrs, 0 non-reloc
  mismatches) and `?ShowError@CGruntzApp@@UAEXXZ` @0x080ac0 (**99.46%**, 89/89,
  0 non-reloc). Both byte-exact modulo reloc-masking. The 2 prior gruntzapp fns
  did NOT regress; the matched gameapp unit did NOT regress (verified by stash:
  VirtualUnknownMethod02 stayed 97.22%, the ctor 99.00%).
- **THE MANAGER-POINTER OFFSET (the headline) = CGameApp+0x8, freed by
  CloseResources, NOT by the dtor.** The dtor delegates ALL teardown to
  `CGameApp::CloseResources`@0x13d8c0, which `delete`s `m_8`@+0x8 (the game
  MANAGER, scalar-deleting dtor via `mov eax,[ecx];push 1;call [eax]`) and
  `m_4`@+0x4 (the window). So the manager ptr is a BASE CGameApp field (+0x8),
  confirming the earlier note that no CGruntzApp-specific manager member exists —
  the store happens in VirtualUnknownMethod02 (`m_8 = InitializeGameManager()`),
  already modeled. **No new CGruntzApp instance member.**
- **The dtor IS virtual (`UAE`, `ret`)** — CGameApp went fully virtual, so the
  override mangles `??1CGruntzApp@@UAE@XZ`. Two-phase EH-framed teardown:
  (1) restore CGruntzApp vftable (0x5e9ab4); (2) run own body = `CloseResources()`
  (devirtualized in a dtor → `CGameApp::CloseResources`@0x13d8c0, reached via a
  DOUBLE incremental-link thunk 0x1b8b→0x80980→0x13d8c0, REL32-masked); (3)
  restore CGameApp base vftable (0x5e9b0c); (4) the **base `~CGameApp` is INLINED**
  → a SECOND `call CloseResources`@0x13d8c0 + the inline `dec [g_count@0x653c6c]`.
  ⇒ CloseResources is called TWICE (once per class level's dtor body) — real
  engine code, not a bug.
- **THE LEVER: `~CGameApp` must be INLINE-defined in the shared header** so the
  cross-TU `~CGruntzApp` inlines it (engine had `~CGameApp(){ CloseResources();
  --s_count; }` inline in CGameApp.h). The matched gameapp had modeled it as an
  empty out-of-line `CGameApp::~CGameApp(){}` (never diffed, so it didn't matter
  there) — CORRECTED to the real inline body in Wap32.h. The instance counter
  @0x653c6c was promoted from GameApp.cpp's file-local `static s_gameAppCount` to
  a shared `extern int g_gameAppInstanceCount` (declared in Wap32.h, defined in
  GameApp.cpp) so the inline `~CGameApp` in GruntzApp.cpp's TU resolves it; the
  rename is reloc-masked → the gameapp ctor/Method02 that reference it stayed
  byte-exact. (Removing the out-of-line `~CGameApp(){}` is required — an inline
  body + an out-of-line def is a redefinition.)
- **ShowError (`UAEXXZ`, `ret`, the +0x30 vtable override of the empty
  `CGameApp::ShowError`@0x080db0=`ret`):** builds the error string into the
  file-scope buffer `g_errorText`@0x644ea0 (`char[0x100]`) then pops the ERROR
  dialog. Algorithm: `id = m_24c ?: 0x8009` (+0x24c error id, default 0x8009);
  `detailVal = m_250` (+0x250); `detail[0]=0; if(detailVal>0) sprintf(detail,
  "(%i)",detailVal)`; `if(LoadStringA(m_c,id,buf,0xfa)<=0 &&
  LoadStringA(m_c,0x8009,buf,0xfa)<=0) strcpy(buf,"Unable to continue game.")`;
  `strcat(buf,detail)`; `while(ShowCursor(TRUE)<0);`; `DialogBoxParamA(m_c,
  "ERROR",0,&ErrorDialogProc,0)`. strcpy/strcat emit INLINE (`repne scasb` strlen
  + `rep movs`). LoadStringA/ShowCursor/DialogBoxParamA are `__declspec(dllimport)
  __stdcall` → `FF15 [IAT]` indirect (reloc-masked); the ErrorDialogProc address
  is taken (`push imm` of its incremental-link thunk 0x4033c8). m_c (hInstance,
  +0xc) is the 1st arg to all three USER32 calls. Strings: `"(%i)"`@0x60fa38,
  `"Unable to continue game."`@0x60fa18, `"ERROR"`@0x60fa10.
- **TWO source levers for ShowError (both load-bearing, both NO-RELOC diffs until
  fixed):** (1) **read BOTH error members up front** (`int id=m_24c; int
  detailVal=m_250;` before the id-default) — the optimiser hoists the m_250 load
  into eax above the `test esi; jne; mov esi,0x8009` id-default and keeps it live
  across it; reading m_250 lazily (inline at the `if`) emitted a DIFF_DELETE/
  INSERT pair (the load floated below the default). (2) **the detail buffer must
  be `char[0x20]`** to get the target's `sub esp,0x20` frame with the buffer at
  `[esp+0xc]` (referenced as `lea [esp+0x10]` post-push for sprintf, `lea
  [esp+0xc]` for the strcat strlen). `[0x10]`→`sub esp,0x10`, `[0x14]`→`0x14`;
  the frame tracks the buffer size 1:1 here, so 0x20 is required even though only
  0x14 bytes (0xc..0x1f) are usable above the 0xc-byte spill gap.
- UNLOCKED: the CGruntzApp object lifecycle is anchored end-to-end (ctor@0x80850,
  dtor@0x808b0, InitializeGameManager@0x80a20, ShowError@0x80ac0, ErrorDialogProc
  @0x80c70). `CGameApp::CloseResources`@0x13d8c0 now has a 2nd (dtor) caller; the
  game-manager pointer is pinned at CGameApp+0x8. The CGameApp vtable's
  scalar-deleting dtor `??_GCGameApp` + `??1CGameApp` now carry their real bodies
  (CloseResources + counter dec) for any future vtable/dtor-chain match.

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
