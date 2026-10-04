# Reinterpret-cast contracts

This directory reviews every `reinterpret_cast` in `src/` and `include/`,
including the boundaries repaired during the audit. The review began with
71 expressions. Thirty-two sites no longer need reinterpretation;
one new, genuine SDK address conversion was added. **40 expressions remain.**
The [const-cast review](../const-casts/README.md) separately records six
remaining casts and four repaired sites.

Each family page records our code and actual source/destination declarations,
ownership and allocation, why the conversion exists, Git introduction and
later changes, pinned GitHub witnesses, and safety limits. Repeated expressions
are counted individually. A successful compile or a matching instruction does
not prove alignment, type accessibility, lifetime, bounds or call compatibility.
A cast being accepted by the present declarations does not prove those
declarations describe the original program.

## Repaired contracts

- [Typed maps](typed-maps.md): five output-reference puns replaced with real
  `void*` outputs and value conversion; failed lookup preserves the caller's value.
- [ZTools callbacks](ztools-callbacks.md): teardown and traversal use functions
  with the signatures actually invoked, with typed adaptation inside them.
- [Palette key](palette-key.md): an optional string is a `const char*` parameter,
  not an integer justified by a reconstruction-generated symbol.
- [SoundFont](soundfont-callbacks.md): the requested vendor interface is 1.0;
  its slot is `ClearLoadedBank`, not the 1.01 pathname callback. The returned
  address uses a real SDK `DWORD` output object.
- [Targeting coordinates](trigger-coordinates.md): the actual parameters are
  `LONG` objects when passed to the `LONG*` coordinate API.
- [Integer payloads and cursors](integer-payloads.md): Booty traversal uses
  actual array elements and its own array bounds.
- [World records](world-records.md): the decompressed byte buffer no longer
  passes through an unused `WwdHeader*` view.
- [Shade pixels](shade-pixels.md): sixteen word-pointer casts replaced with
  indexed byte access and copies through actual word objects.

These include intentional repairs of unsafe retail behavior, not just renamed
casts. The family pages identify retained assumptions separately. In particular,
this is not a complete malformed-WWD parser audit, proof of every SDK provider,
or a census of equivalent casts hidden behind unions or `void*` detours.

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
| [include/Utils/MapTyped.h:51](../../include/Utils/MapTyped.h#L51) | `reinterpret_cast<void*>(id)` | [typed-maps](typed-maps.md) |
| [include/Wwd/WwdObjMgrInline.h:13](../../include/Wwd/WwdObjMgrInline.h#L13) | `reinterpret_cast<void*>(o->GetObjectId())` | [typed-maps](typed-maps.md) |
| [include/ZTools/ZVec.h:19](../../include/ZTools/ZVec.h#L19) | `reinterpret_cast<char*>(ZVEC_NO_SCRATCH_ADDRESS)` | [ztools-identities](ztools-identities.md) |
| [src/Bute/TypeKeyColl.cpp:486](../../src/Bute/TypeKeyColl.cpp#L486) | `reinterpret_cast<i32>(p)` | [ztools-identities](ztools-identities.md) |
| [src/Bute/TypeKeyColl.cpp:678](../../src/Bute/TypeKeyColl.cpp#L678) | `reinterpret_cast<long>(dl[slot].object)` | [ztools-identities](ztools-identities.md) |
| [src/Bute/TypeKeyColl.cpp:678](../../src/Bute/TypeKeyColl.cpp#L678) | `reinterpret_cast<long>(o)` | [ztools-identities](ztools-identities.md) |
| [src/DDrawMgr/DDrawSurfacePair.cpp:407](../../src/DDrawMgr/DDrawSurfacePair.cpp#L407) | `reinterpret_cast<GUID*>(DDCREATE_EMULATIONONLY)` | [directdraw-sentinel](directdraw-sentinel.md) |
| [src/DDrawMgr/LevelPlane.cpp:124](../../src/DDrawMgr/LevelPlane.cpp#L124) | `reinterpret_cast<const i32*>(blockBase + pd->m_tilesOffset)` | [world-records](world-records.md) |
| [src/DDrawMgr/LevelPlane.cpp:545](../../src/DDrawMgr/LevelPlane.cpp#L545) | `reinterpret_cast<const PlaneObjectRecord*>(recordCursor)` | [world-records](world-records.md) |
| [src/Gruntz/GameLevel.cpp:135](../../src/Gruntz/GameLevel.cpp#L135) | `reinterpret_cast<char*>(source)` | [world-records](world-records.md) |
| [src/Gruntz/GameLevel.cpp:148](../../src/Gruntz/GameLevel.cpp#L148) | `reinterpret_cast<char*>(InflateMainBlock(source, buf, capacity))` | [world-records](world-records.md) |
| [src/Gruntz/GameLevel.cpp:164](../../src/Gruntz/GameLevel.cpp#L164) | `reinterpret_cast<const WwdPlaneHeader*>(block + source->m_planesOffset)` | [world-records](world-records.md) |
| [src/Gruntz/GameLevel.cpp:175](../../src/Gruntz/GameLevel.cpp#L175) | `reinterpret_cast<WwdTileDescTable*>(block + source->m_tileDescriptionsOffset)` | [world-records](world-records.md) |
| [src/Gruntz/GameLevel.cpp:1535](../../src/Gruntz/GameLevel.cpp#L1535) | `reinterpret_cast<Bytef*>(src)` | [world-records](world-records.md) |
| [src/Gruntz/MultiStartDlg.cpp:97](../../src/Gruntz/MultiStartDlg.cpp#L97) | `reinterpret_cast<WNDPROC>(GetWindowLongA(editHwnd, GWL_WNDPROC))` | [win32-procedures](win32-procedures.md) |
| [src/Gruntz/MultiStartDlg.cpp:98](../../src/Gruntz/MultiStartDlg.cpp#L98) | `reinterpret_cast<LONG>(MultiMapComboEditProc)` | [win32-procedures](win32-procedures.md) |
| [src/Gruntz/MultiStartDlg.cpp:106](../../src/Gruntz/MultiStartDlg.cpp#L106) | `reinterpret_cast<LPCTSTR>(lParam)` | [win32-procedures](win32-procedures.md) |
| [src/Gruntz/SFSelectDevice.cpp:86](../../src/Gruntz/SFSelectDevice.cpp#L86) | `reinterpret_cast<SFMANAGER*>(GetProcAddress(g_sfDll, "SFManager"))` | [soundfont-callbacks](soundfont-callbacks.md) |
| [src/Gruntz/SFSelectDevice.cpp:100](../../src/Gruntz/SFSelectDevice.cpp#L100) | `reinterpret_cast<SFMANL100API*>(interfaceAddress)` | [soundfont-callbacks](soundfont-callbacks.md) |
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
