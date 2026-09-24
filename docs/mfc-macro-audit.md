# MFC macro restoration audit

Scope: all 282 `.cpp` and 540 `.h` files under `src/` and `include/`, starting
from `37ace99bd`. The macro reference is the pinned MSVC 5.0 SP3 SDK, especially
`AFX.H`, `AFXWIN.H`, `AFXMSG_.H`, `AFXDISP.H` and `AFXCTL.H`. Vendored libraries
and generated files are not reconstructed game source.

The search covered both macro names and the names/types emitted by their
expansions. Looking only for existing `BEGIN_MESSAGE_MAP` invocations would
have missed every reconstructed map.

| Family searched | Expansion evidence searched | Finding |
| --- | --- | --- |
| Message maps | `AFX_MSGMAP`, `AFX_MSGMAP_ENTRY`, `GetMessageMap`, `AfxSig_*`, member-pointer casts and numeric notification rows | Six complete manual map families; restored below. |
| Runtime classes, dynamic creation, serialization | `CRuntimeClass`, `GetRuntimeClass`, `IsKindOf`, class records, factory/base-class fields, class registration | No hand-expanded game-source macro family found. The dialogs inherit `CDialog::GetRuntimeClass`; its retail vtable slot does not prove a dialog-specific runtime-class record. |
| MFC exceptions | `try`, `catch`, `throw`, `AFX_EXCEPTION_LINK`, exception cleanup/throw helpers | No hand-expanded macro family found. |
| OLE, dispatch, interface, event, command and connection maps | SDK map types, map getters, interface-part fields, module-state carriers, `METHOD_PROLOGUE` patterns | No hand-expanded macro family found. |
| Assertions, trace, object zeroing and collection sentinel | assertion/trace helpers, `memset`/`ZeroMemory` on `this` or a base-sized tail, `POSITION` minus-one spellings | Existing `ASSERT`/`TRACE` calls already use macros; no positively identifiable additional expansion found. Release-erased debug code cannot be recovered by this search. |

Four direct `AfxGetModuleState()->m_hCurrentInstanceHandle` accesses in
`SoundBuffer.cpp` are possible **inline-function** restorations
(`AfxGetInstanceHandle`), outside this macro audit. Ordinary MFC method and
collection inlines were not relabeled as macros.

| Owner | Getter RVA | Map RVA | Entry RVA | Entry bytes (including terminator) |
| --- | --- | --- | --- | ---: |
| `CBattlezDlg` / `Dialogs.cpp` | `0x15aa0` | `0x1e88b0` | `0x1e88b8` | 624 |
| `CBattlezDlgColors` / `BattlezDlgColors.cpp` | `0x17ac0` | `0x1e8d10` | `0x1e8d18` | 96 |
| `CBattlezDlgCustom` / `CustomLevelDlg.cpp` | `0x183d0` | `0x1e8e98` | `0x1e8ea0` | 48 |
| `CCheckpointDlg` / `CheckpointDlg.cpp` | `0x23570` | `0x1e94b8` | `0x1e94c0` | 48 |
| `CMultiHelpDlg` / `MultiHelpDlg.cpp` | `0xbec00` | `0x1ea448` | `0x1ea450` | 24 |
| `CMultiStartDlg` / `MultiStartDlg.cpp` | `0xc2620` | `0x1ea578` | `0x1ea580` | 720 |

Each getter is six bytes, with zero calls/branches, one return and one DIR32
relocation. Each map is eight bytes with two DIR32 relocations. Before editing,
`walls diagnose`, `sema disasm`, `sema xref` and the dialog vtables corroborated
these identities. All six getters had current/banked/historical 100%.

The restoration uses `DECLARE_MESSAGE_MAP`, `BEGIN_MESSAGE_MAP`,
`END_MESSAGE_MAP`, `ON_WM_PAINT`, `ON_WM_TIMER`, `ON_WM_MEASUREITEM`,
`ON_WM_DRAWITEM` and `ON_LBN_DBLCLK`. Eight numeric `ON_CONTROL` spellings become
`ON_EN_CHANGE`/`ON_EN_KILLFOCUS`. It removes `GZ_MFC_PMSG`. Handler order,
including MultiStart's duplicated world-selection notification, is preserved.

The resource and consumer check also corrects two handler declarations. Dialog
resource 194 defines control 1301 (`0x515`) as a `LISTBOX`, corroborated by the
color dialog's `LB_*` operations; the notification is `ON_LBN_DBLCLK`, despite
the old numeric expansion spelling the equal-valued `CBN_DBLCLK`. Its sole
retail handler reference is the color dialog's entry at `+0x44`, so
`OnOkCommand` belongs to `CBattlezDlgColors`, not the unrelated `CBattlezDlg`.
The eight-byte body jumps through the receiver's inherited `OnOK` slot at
`+0xcc`. The paint handler at `0x14b10` is referenced only by the `ON_WM_PAINT`
entry and becomes `void`, matching that callback contract; its five-byte tail
jump to `CWnd::Default` is preserved.

The [message-map pattern](patterns/mfc-message-map-real-static-data.md) documents
the annotation and Clang compatibility mechanism, direct COFF/retail controls
and the previously incorrect reason for retaining expansions.

Repeat the lexical portion with:

```sh
rg -n 'AFX_.*MAP|GetMessageMap|AfxSig_|CRuntimeClass|GetRuntimeClass|IsKindOf' src include
rg -n '\b(try|catch|throw|AFX_EXCEPTION_LINK|AfxTryCleanup|AfxThrowLastCleanup)\b' src include
rg -n 'METHOD_PROLOGUE|GetDispatchMap|GetInterfaceMap|GetEventSinkMap|GetConnectionMap|m_pModuleState' src include
rg -n 'BEFORE_START_POSITION|POSITION.*-1|reinterpret_cast<POSITION>|AfxGetModuleState|ASSERT|TRACE|memset|ZeroMemory' src include
```

Then read the SDK expansion and the complete candidate source; lexical hits
alone do not establish a macro's semantics or justify a replacement.
