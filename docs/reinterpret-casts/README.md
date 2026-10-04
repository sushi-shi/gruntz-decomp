# Reinterpret-cast contracts

This directory reviews every `reinterpret_cast` in `src/` and `include/`,
including repaired and retained unsafe boundaries. The review began with
71 expressions. Eleven original sites no longer need reinterpretation and one
SDK output-storage cast is now explicit. **61 expressions remain.**
The [const-cast review](../const-casts/README.md) separately records all ten
remaining const casts.

Each family page records our code and actual source/destination declarations,
ownership and allocation, why the conversion exists, Git introduction and
later changes, pinned GitHub witnesses, and safety limits. Repeated expressions
are counted individually. A successful compile or a matching instruction does
not prove alignment, type accessibility, lifetime, bounds or call compatibility.
A cast being accepted by the present declarations does not prove those
declarations describe the original program.

## Repaired and retained contracts

- [Typed maps](typed-maps.md): three output-reference puns replaced with real
  `void*` outputs and value conversion. String-map and integer-ID lookup retain their unsafe
  output-reference casts to preserve its matching implementation.
- [ZTools callbacks](ztools-callbacks.md): teardown and traversal use functions
  with the signatures actually invoked, with typed adaptation inside them.
- [Palette key](palette-key.md): an optional string is a `const char*` parameter,
  not an integer justified by a reconstruction-generated symbol.
- [SoundFont](soundfont-callbacks.md): the requested vendor interface is 1.0;
  its slot is `ClearLoadedBank`, not the 1.01 pathname callback. The SDK query
  still writes into pointer storage through `DWORD*`, a retained aliasing hazard.
- [Flag positions](integer-payloads.md): traversal now ends at its own array
  boundary. The separate cheat-row cursor still crosses record members and
  compares integer addresses; that source-model defect remains.
- [Targeting coordinates](trigger-coordinates.md): real `LONG` parameters
  replace the incompatible `int` argument-slot views and remove both casts.
- [World records](world-records.md) and [shade pixels](shade-pixels.md)
  retain their reviewed casts and explain the changes needed to remove them.

A cast may be necessary for the current declarations or instruction shape
without being required by the SDK or safe under the C++ object model.
The family pages separate those cases. Known unsafe behavior is explicitly
retained in several places to preserve the reconstruction's matching shape;
this is not a complete UB repair, malformed-WWD parser audit, or census of
equivalent conversions hidden behind unions or `void*` detours.

## Every remaining expression

The code links locate each occurrence; the family page supplies the symbol,
contract and provenance. Source line numbers can move.

| Code | Current expression | Contract |
| --- | --- | --- |
| [include/Gruntz/ActRegistry.h:11](../../include/Gruntz/ActRegistry.h#L11) | `reinterpret_cast<i32>(g_buteTree.lookup(key))` | [integer-payloads](integer-payloads.md) |
| [include/Gruntz/ActRegistry.h:15](../../include/Gruntz/ActRegistry.h#L15) | `reinterpret_cast<i32*>(id)` | [integer-payloads](integer-payloads.md) |
| [include/Image/Image.h:60](../../include/Image/Image.h#L60) | `reinterpret_cast<u16*>(m_pBytes)` | [dib-views](dib-views.md) |
| [include/Image/Image.h:208](../../include/Image/Image.h#L208) | `reinterpret_cast<BITMAPINFO*>(&m_bmi)` | [dib-views](dib-views.md) |
| [include/Image/Image.h:246](../../include/Image/Image.h#L246) | `reinterpret_cast<BITMAPINFO*>(&m_bmi)` | [dib-views](dib-views.md) |
| [include/Image/Image.h:295](../../include/Image/Image.h#L295) | `reinterpret_cast<BITMAPINFO*>(&m_bmi)` | [dib-views](dib-views.md) |
| [include/Utils/FreeNodePool.h:55](../../include/Utils/FreeNodePool.h#L55) | `reinterpret_cast<Node*>(static_cast<char*>(payload) - m_linkOffset)` | [node-pool](node-pool.md) |
| [include/Utils/MapTyped.h:8](../../include/Utils/MapTyped.h#L8) | `reinterpret_cast<void*&>(out)` | [typed-maps](typed-maps.md) |
| [include/Utils/MapTyped.h:45](../../include/Utils/MapTyped.h#L45) | `reinterpret_cast<void*>(id)` | [typed-maps](typed-maps.md) |
| [include/Utils/MapTyped.h:45](../../include/Utils/MapTyped.h#L45) | `reinterpret_cast<void*&>(out)` | [typed-maps](typed-maps.md) |
| [include/Wwd/WwdObjMgrInline.h:13](../../include/Wwd/WwdObjMgrInline.h#L13) | `reinterpret_cast<void*>(o->GetObjectId())` | [typed-maps](typed-maps.md) |
| [include/ZTools/ZVec.h:19](../../include/ZTools/ZVec.h#L19) | `reinterpret_cast<char*>(ZVEC_NO_SCRATCH_ADDRESS)` | [ztools-identities](ztools-identities.md) |
| [src/Bute/TypeKeyColl.cpp:486](../../src/Bute/TypeKeyColl.cpp#L486) | `reinterpret_cast<i32>(p)` | [ztools-identities](ztools-identities.md) |
| [src/Bute/TypeKeyColl.cpp:678](../../src/Bute/TypeKeyColl.cpp#L678) | `reinterpret_cast<long>(dl[slot].object)` | [ztools-identities](ztools-identities.md) |
| [src/Bute/TypeKeyColl.cpp:678](../../src/Bute/TypeKeyColl.cpp#L678) | `reinterpret_cast<long>(o)` | [ztools-identities](ztools-identities.md) |
| [src/DDrawMgr/DDrawShadeBlit.cpp:1185](../../src/DDrawMgr/DDrawShadeBlit.cpp#L1185) | `reinterpret_cast<u16*>(dst)` | [shade-pixels](shade-pixels.md) |
| [src/DDrawMgr/DDrawShadeBlit.cpp:1186](../../src/DDrawMgr/DDrawShadeBlit.cpp#L1186) | `reinterpret_cast<u16*>(g_scratch)` | [shade-pixels](shade-pixels.md) |
| [src/DDrawMgr/DDrawShadeBlit.cpp:1207](../../src/DDrawMgr/DDrawShadeBlit.cpp#L1207) | `reinterpret_cast<u16*>(dst)` | [shade-pixels](shade-pixels.md) |
| [src/DDrawMgr/DDrawShadeBlit.cpp:1208](../../src/DDrawMgr/DDrawShadeBlit.cpp#L1208) | `reinterpret_cast<u16*>(src)` | [shade-pixels](shade-pixels.md) |
| [src/DDrawMgr/DDrawShadeBlit.cpp:1209](../../src/DDrawMgr/DDrawShadeBlit.cpp#L1209) | `reinterpret_cast<u16*>(g_scratch)` | [shade-pixels](shade-pixels.md) |
| [src/DDrawMgr/DDrawShadeBlit.cpp:1368](../../src/DDrawMgr/DDrawShadeBlit.cpp#L1368) | `reinterpret_cast<u16*>(&g_scratch[count * 2 - 2])` | [shade-pixels](shade-pixels.md) |
| [src/DDrawMgr/DDrawShadeBlit.cpp:1369](../../src/DDrawMgr/DDrawShadeBlit.cpp#L1369) | `reinterpret_cast<u16*>(dst)` | [shade-pixels](shade-pixels.md) |
| [src/DDrawMgr/DDrawShadeBlit.cpp:1370](../../src/DDrawMgr/DDrawShadeBlit.cpp#L1370) | `reinterpret_cast<u16*>(src)` | [shade-pixels](shade-pixels.md) |
| [src/DDrawMgr/DDrawShadeBlit.cpp:1524](../../src/DDrawMgr/DDrawShadeBlit.cpp#L1524) | `reinterpret_cast<u16*>(dst)` | [shade-pixels](shade-pixels.md) |
| [src/DDrawMgr/DDrawShadeBlit.cpp:1525](../../src/DDrawMgr/DDrawShadeBlit.cpp#L1525) | `reinterpret_cast<u16*>(g_scratch)` | [shade-pixels](shade-pixels.md) |
| [src/DDrawMgr/DDrawShadeBlit.cpp:1540](../../src/DDrawMgr/DDrawShadeBlit.cpp#L1540) | `reinterpret_cast<u16*>(dst)` | [shade-pixels](shade-pixels.md) |
| [src/DDrawMgr/DDrawShadeBlit.cpp:1541](../../src/DDrawMgr/DDrawShadeBlit.cpp#L1541) | `reinterpret_cast<u16*>(src)` | [shade-pixels](shade-pixels.md) |
| [src/DDrawMgr/DDrawShadeBlit.cpp:1542](../../src/DDrawMgr/DDrawShadeBlit.cpp#L1542) | `reinterpret_cast<u16*>(g_scratch)` | [shade-pixels](shade-pixels.md) |
| [src/DDrawMgr/DDrawShadeBlit.cpp:1634](../../src/DDrawMgr/DDrawShadeBlit.cpp#L1634) | `reinterpret_cast<u16*>(dst)` | [shade-pixels](shade-pixels.md) |
| [src/DDrawMgr/DDrawShadeBlit.cpp:1635](../../src/DDrawMgr/DDrawShadeBlit.cpp#L1635) | `reinterpret_cast<u16*>(src)` | [shade-pixels](shade-pixels.md) |
| [src/DDrawMgr/DDrawShadeBlit.cpp:1636](../../src/DDrawMgr/DDrawShadeBlit.cpp#L1636) | `reinterpret_cast<u16*>(&g_scratch[count * 2 - 2])` | [shade-pixels](shade-pixels.md) |
| [src/DDrawMgr/DDrawSurfacePair.cpp:407](../../src/DDrawMgr/DDrawSurfacePair.cpp#L407) | `reinterpret_cast<GUID*>(DDCREATE_EMULATIONONLY)` | [directdraw-sentinel](directdraw-sentinel.md) |
| [src/DDrawMgr/LevelPlane.cpp:124](../../src/DDrawMgr/LevelPlane.cpp#L124) | `reinterpret_cast<const i32*>(blockBase + pd->m_tilesOffset)` | [world-records](world-records.md) |
| [src/DDrawMgr/LevelPlane.cpp:545](../../src/DDrawMgr/LevelPlane.cpp#L545) | `reinterpret_cast<const PlaneObjectRecord*>(recordCursor)` | [world-records](world-records.md) |
| [src/Gruntz/BootyStateActivate.cpp:233](../../src/Gruntz/BootyStateActivate.cpp#L233) | `reinterpret_cast<i32>( g_bootyCheatMessages[24].m_description + sizeof(BootyCheatMessage) )` | [integer-payloads](integer-payloads.md) |
| [src/Gruntz/BootyStateActivate.cpp:247](../../src/Gruntz/BootyStateActivate.cpp#L247) | `reinterpret_cast<i32>(p)` | [integer-payloads](integer-payloads.md) |
| [src/Gruntz/GameLevel.cpp:135](../../src/Gruntz/GameLevel.cpp#L135) | `reinterpret_cast<char*>(source)` | [world-records](world-records.md) |
| [src/Gruntz/GameLevel.cpp:148](../../src/Gruntz/GameLevel.cpp#L148) | `reinterpret_cast<WwdHeader*>(InflateMainBlock(source, buf, capacity))` | [world-records](world-records.md) |
| [src/Gruntz/GameLevel.cpp:154](../../src/Gruntz/GameLevel.cpp#L154) | `reinterpret_cast<char*>(hdr)` | [world-records](world-records.md) |
| [src/Gruntz/GameLevel.cpp:166](../../src/Gruntz/GameLevel.cpp#L166) | `reinterpret_cast<const WwdPlaneHeader*>(block + source->m_planesOffset)` | [world-records](world-records.md) |
| [src/Gruntz/GameLevel.cpp:177](../../src/Gruntz/GameLevel.cpp#L177) | `reinterpret_cast<WwdTileDescTable*>(block + source->m_tileDescriptionsOffset)` | [world-records](world-records.md) |
| [src/Gruntz/GameLevel.cpp:1547](../../src/Gruntz/GameLevel.cpp#L1547) | `reinterpret_cast<Bytef*>(src)` | [world-records](world-records.md) |
| [src/Gruntz/MultiStartDlg.cpp:97](../../src/Gruntz/MultiStartDlg.cpp#L97) | `reinterpret_cast<WNDPROC>(GetWindowLongA(editHwnd, GWL_WNDPROC))` | [win32-procedures](win32-procedures.md) |
| [src/Gruntz/MultiStartDlg.cpp:98](../../src/Gruntz/MultiStartDlg.cpp#L98) | `reinterpret_cast<LONG>(MultiMapComboEditProc)` | [win32-procedures](win32-procedures.md) |
| [src/Gruntz/MultiStartDlg.cpp:106](../../src/Gruntz/MultiStartDlg.cpp#L106) | `reinterpret_cast<LPCTSTR>(lParam)` | [win32-procedures](win32-procedures.md) |
| [src/Gruntz/SFSelectDevice.cpp:86](../../src/Gruntz/SFSelectDevice.cpp#L86) | `reinterpret_cast<SFMANAGER*>(GetProcAddress(g_sfDll, "SFManager"))` | [soundfont-callbacks](soundfont-callbacks.md) |
| [src/Gruntz/SFSelectDevice.cpp:95](../../src/Gruntz/SFSelectDevice.cpp#L95) | `reinterpret_cast<PDWORD>(&g_sfDevice)` | [soundfont-callbacks](soundfont-callbacks.md) |
| [src/Gruntz/SerialObjectFactory.cpp:339](../../src/Gruntz/SerialObjectFactory.cpp#L339) | `reinterpret_cast<i32>(payload)` | [integer-payloads](integer-payloads.md) |
| [src/Gruntz/Utils.cpp:192](../../src/Gruntz/Utils.cpp#L192) | `reinterpret_cast<CREATESNAPSHOT>(GetProcAddress(hKernel, "CreateToolhelp32Snapshot"))` | [win32-procedures](win32-procedures.md) |
| [src/Gruntz/Utils.cpp:198](../../src/Gruntz/Utils.cpp#L198) | `reinterpret_cast<PROCESSWALK>(GetProcAddress(hKernel, "Process32First"))` | [win32-procedures](win32-procedures.md) |
| [src/Gruntz/Utils.cpp:204](../../src/Gruntz/Utils.cpp#L204) | `reinterpret_cast<PROCESSWALK>(GetProcAddress(hKernel, "Process32Next"))` | [win32-procedures](win32-procedures.md) |
| [src/Gruntz/Utils.cpp:282](../../src/Gruntz/Utils.cpp#L282) | `reinterpret_cast<CREATESNAPSHOT>(GetProcAddress(hKernel, "CreateToolhelp32Snapshot"))` | [win32-procedures](win32-procedures.md) |
| [src/Gruntz/Utils.cpp:288](../../src/Gruntz/Utils.cpp#L288) | `reinterpret_cast<MODULEWALK>(GetProcAddress(hKernel, "Module32First"))` | [win32-procedures](win32-procedures.md) |
| [src/Gruntz/Utils.cpp:294](../../src/Gruntz/Utils.cpp#L294) | `reinterpret_cast<MODULEWALK>(GetProcAddress(hKernel, "Module32Next"))` | [win32-procedures](win32-procedures.md) |
| [src/Io/SaveGame.cpp:144](../../src/Io/SaveGame.cpp#L144) | `reinterpret_cast<u8*>(GetSlot(i))` | [save-bytes](save-bytes.md) |
| [src/Io/SaveGame.cpp:158](../../src/Io/SaveGame.cpp#L158) | `reinterpret_cast<u8*>(GetSlot(i))` | [save-bytes](save-bytes.md) |
| [src/Net/NetMgr.cpp:253](../../src/Net/NetMgr.cpp#L253) | `reinterpret_cast<LPARAM>(provider)` | [net-records](net-records.md) |
| [src/Net/NetMgr.cpp:286](../../src/Net/NetMgr.cpp#L286) | `reinterpret_cast<CNetProviderNode*>(itemData)` | [net-records](net-records.md) |
| [src/Net/NetMgr.cpp:431](../../src/Net/NetMgr.cpp#L431) | `reinterpret_cast<CNetSessionListNode*>(itemData)` | [net-records](net-records.md) |
| [src/Net/NetMgr.cpp:494](../../src/Net/NetMgr.cpp#L494) | `reinterpret_cast<LPDPSESSIONDESC2>(descriptionBytes)` | [net-records](net-records.md) |
| [src/Wwd/WwdGameObject.cpp:461](../../src/Wwd/WwdGameObject.cpp#L461) | `reinterpret_cast<void*>(node)` | [typed-maps](typed-maps.md) |

## Maintaining the review

Run `rg -n 'reinterpret_cast' src include` and count expressions, not just
lines: one line can contain two conversions. Update the inventory and the
corresponding contract when adding, changing or removing a site. Keep repaired
sites documented where they explain the replacement contract; do not make a
vanished cast look like an unreviewed omission.

Follow `git blame` through moves and mechanical sweeps with the introducing
diff or `git log -S`. Separate a vendor declaration, related-family source,
a target-specific ABI convention, and an inferred game signature. Source
lineage adoption decisions stay in [the ledger](../../config/lithtech_lineage.tsv).
Pin GitHub code examples to full commits and explain whether they establish
the actual contract or merely illustrate similar usage. None is an automatic
safety exemption.
