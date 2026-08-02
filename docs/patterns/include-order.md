# The canonical `#include` block

Gated by `gruntz.audit.include_order` (normal tier). Fixer:

```
python -m gruntz.audit.include_order --fix-dupes --fix
```

The tree was reconstructed function-by-function, so includes accreted wherever a
type was first needed - appended mid-file, duplicated, unordered. Three sweeps
(2026-08-02) closed that out; this file is the resulting contract.

## The order

Groups separated by ONE blank line. Within a group, sort case-insensitively -
except group 3, which carries an explicit dependency rank.

| # | group | contents |
|---|-------|----------|
| 0 | config `#define`s | `#define CGAMEOBJECT_OOL_CTOR`, `#define DIRECTINPUT_VERSION 0x0500` |
| 1 | `<rva.h>` | the label-macro header, always first |
| 2 | the TU's own header | `X.cpp` -> `<Dir/X.h>` (`.cpp` only) |
| 3 | platform preludes | `<Mfc.h>` `<MfcNoInline.h>` `<MfcWin.h>` `<Win32.h>` |
| 4 | project / engine | everything else under `include/` |
| 5 | library | CRT, DirectX, 3rd-party, raw `<afxtempl.h>`/`<afxcmn.h>` |

## Why the own header leads, and why group 3 is ranked, not sorted

The own header parses FIRST (after `<rva.h>`): everything below it is a
dependency, in widening order - and parsing it before any prelude is what keeps
it honest. A header that needs `<Mfc.h>` has to say so itself rather than
inherit it from whatever its `.cpp` happened to include earlier; a leaner
surfaces as a compile error in its own TU instead of silently working.

This is safe for the `<MfcNoInline.h>` device TUs: the device pulls `<Mfc.h>`
itself (inlines still ON while afx.h/afxcoll.h parse - identical to a
self-sufficient own header having pulled it) and then undefines
`_AFX_ENABLE_INLINES`, which only affects MFC headers parsed AFTER it
(`<afxwin.h>` via `<MfcWin.h>`, raw `<afxtempl.h>` in group 5). An own header
must simply never pull `<MfcWin.h>`/`<afxtempl.h>` itself in a device TU.

The preludes are not ordinary libraries; they configure how every header parsed
after them is *seen*, so their order is a dependency, not a preference:

- `<Mfc.h>` defines `VC_EXTRALEAN`, then pulls `<afx.h>` + `<afxcoll.h>`.
- `<MfcNoInline.h>` pulls `<Mfc.h>`, then undefines `_AFX_ENABLE_INLINES` for
  **every** compiler - a per-TU codegen device (retail's objects for those units
  show the out-of-line MFC accessors), not an LSP workaround.
- `<MfcWin.h>` pulls `<Mfc.h>`, suppresses the same inlines **for clang only**,
  then pulls `<afxwin.h>`.

Both device headers pull `<Mfc.h>` themselves, so each is atomic: it does the
whole job on its own and cannot be defeated by where it lands.

Alphabetical order would land a device *after* the header it configures, so the
rank is explicit (`PRELUDE_RANK`).

## Only a prelude with LOGIC earns a wrapper

`<MfcWin.h>` and `<MfcNoInline.h>` exist because they carry a device that was
otherwise copy-pasted at 13 sites. `<afxtempl.h>` and `<afxcmn.h>` need no logic,
so they stay raw in group 5 - a wrapper with nothing in it is just indirection.

**A device header must pull `<Mfc.h>` itself, and that is not optional.**
`_AFX_ENABLE_INLINES` is *defined by* `<afx.h>`; undefining it before afx.h has
ever been parsed is a silent no-op. For `<MfcWin.h>` the failure is loud - the
clang label pass dies on `afxwin1.inl:1027` ("a type specifier is required for
all declarations"), the TU contributes zero labels, and the `[labels]` gate
trips (not the compiler). For `<MfcNoInline.h>` it would be silent: the undef
simply does nothing and the TU quietly compiles against the inline accessors.

The canonical order does sort `<Mfc.h>` ahead of both, so leaning on that would
"work" - but a device that only works because of where it is sorted is a
coincidence, not a device. Each wrapper is therefore atomic.

## `<Mfc.h>` is a superset of `<Win32.h>`

Both declare `INT_PTR` and `timeGetTime`; `<Mfc.h>` additionally reaches
`windows.h` through afx. A TU that includes `<Win32.h>` **and** reaches MFC
transitively gets `windows.h` before `afx.h` and dies on

```
afxv_w32.h(14) : fatal error C1189: WINDOWS.H already included.
                 MFC apps must not #include <windows.h>
```

so the redundant `<Win32.h>` is dropped (21 files). Conversely a TU that parses
raw DirectX/multimedia headers needs *some* prelude in group 3 - group 5 is last,
so `<dsound.h>`/`<mmsystem.h>` would otherwise see no Win32 types
(`mmsystem.h(360): 'UINT' : missing decl-specifiers`).

## Traps

**Comments inside the block are load-bearing.** A comment that introduces an
include travels with it when the block is sorted; a comment with no include after
it belongs to the *code* below and must stay put. Getting this wrong once ate 38
`// @early-stop` markers and a `// clang-format off`. The fixer now asserts
line-conservation on every rewrite (`assert_conserved`): the output must be a
permutation of the input, minus exactly the duplicate includes it meant to drop.

**`CRect`/`CPoint`/`CSize` are the GAME's types, not MFC's.**
`include/Wap32/Rect.h` defines `struct CRect : public tagRECT`, kept local
because label-generation clang cannot consume MFC's `CRect` inlines. Treating
those spellings as MFC evidence pulls `<afxwin.h>` in and hits
`C2011: 'CRect' : 'class' type redefinition` across 11 TUs.

**Self-sufficiency is CLOSED and GATED** (2026-08-02). A standalone-compile
sweep of every header under `include/` (one probe TU per header, MSVC 5.0 under
wine) proved all 419 compile on their own; the audit's `missing prelude` count
gates at 0. The sweep's real defects, all fixed by one verified prelude each:
`Image/FileImage.h` + `Gruntz/LightFxRender.h` (`<Win32.h>` -> `<Mfc.h>`; as
own headers of MFC TUs they landed windows.h before afx -> C1189),
`DDrawMgr/DirPal.h` (`LOGPALETTE`/`PALETTEENTRY` with no supply at all),
`Dsndmgr/WaveFormatPtr.h` (raw `<mmsystem.h>` with no Win32 types),
`Gruntz/CustomWorldInfoDlg.h` (`INT_PTR`/`CALLBACK` typedefs - not
fwd-declarable). Earlier hand-fixes: `Wwd/WwdObjMgr.h`, `Gruntz/SBI_MenuItem.h`,
`Net/NetCmdSlot.h`, `Gruntz/MapMgr.h`.

**Which prelude a header gets is decided by its INCLUDER side.** `<Mfc.h>` for
headers whose includers are MFC TUs (the default - a superset of `<Win32.h>`
that cannot trip C1189); `<Win32.h>` only where every includer is a pure-Win32
TU (`ProcAddr.h`, `Gruntz/SFSelectDevice.h`, `Wap32/Rect.h`) - dragging afx
into those TUs would change their codegen. A vendored SDK header that pulls
`<windows.h>` itself (`SFMAN.H`) already supplies the surface.

**The audit's evidence honors what a compiler would.** A token satisfied by a
forward declaration (`class CString;` in the file or one hop down its project
includes) or an elaborated-type-specifier (`class CString* text`,
`struct tagRECT* dst`) needs no prelude; supply through the header's OWN
includes (transitive) is self-sufficiency, not leaning. That is what makes a
flagged header a REAL defect and the gate safe - the old CRect/C2011 auto-fix
hazard applied to the blind token match, not to this.

## MANUAL files

Six files hold a device the fixer will not reorder around; it reports them and
leaves them untouched rather than mangling them.

| file | device |
|------|--------|
| `src/Bute/TypeKeyColl.cpp` | `#undef isspace`/`isdigit` + `#pragma function(memcpy)` after `<ctype.h>` |
| `src/Image/PaletteCopy.cpp`, `ImageProbe.cpp`, `src/DDrawMgr/DDPageMgr.cpp` | `#undef u8..s64` after `<smack.h>`, which defines them as macros |
| `src/Gruntz/BattlezMapConfig.cpp`, `GruntArrivalScan.cpp` | an include below a mid-file `#undef` |

`include/MfcWin.h` also reports MANUAL - correctly, it *is* the device.
