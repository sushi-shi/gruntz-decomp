# Recover typed access to MFC pointer collections

Many class members are raw MFC pointer collections (`CPtrList`, `CPtrArray`,
`CObList`, `CObArray`, `CMapStringToPtr`, `CMapPtrToPtr`) although every
consumer treats each one as holding a single pointer type. Callers repeat casts
around `GetAt()`/`GetData()`/`GetNext()`, choose between `Add` and `InsertAt`
by hand, and duplicate element-lifetime policy. The goal is to recover the
source layer the original developers wrote around each collection.

## Settled

- The containers stay native MFC. Retail constructs native containers with no
  derived-vptr store, while `CTypedPtrArray`/`CTypedPtrList`/`CTypedPtrMap`
  would add one. Verified for the status-bar reward queue (`CStatusBarMgr`
  +0x530, `CPtrArray::CPtrArray` at 0xb59e7), the status-bar tab lists (vector
  construction at 0xb58f3), the `CDDrawChildGroup` ID maps (`CMapPtrToPtr` at
  0x1b85b1) and the sound/animation registry maps (`CMapStringToPtr` at
  0x1b8247). History: `git show 4e7b6ee0b^:docs/todos/recover-typed-mfc-pointer-collections.md`.
- Keep retail's resolved MFC calls. The reward queue's append path is
  `CPtrArray::Add` (expanding to `SetAtGrow`) and its interior insertion is
  `CPtrArray::InsertAt`; a helper must not merge them.

## Resolved families

- `CStatusBarMgr::m_rewardQueue`: `GetReward(i)` plus `ClearRewardQueue()`
  (recycle every `Coord` to `g_coordPool`, then `SetSize(0, -1)`); insertion
  keeps `InsertAt`/`Add`, deserialization stores with `SetAt`.
- `CGrunt::m_coordList`: `GetHeadCoord()`/`GetTailCoord()` beside the
  existing `CoordHead()`/`CoordTail()` positions, at every head/tail read
  (CGrunt and CBattlezMapConfig). `ValidateUnitPath` keeps its local head
  `POSITION` (converting it costs 0.7). `OnObjectRemoved` and
  `LoadStateRecord` now use `RECYCLE_GRUNT_COORDS_VIA_NEXTDATA`. The other
  recycle shapes (`RecycleCoords`, `RecycleGruntCoords`, `ARR_RECYCLE`) are
  measured variants left as they are; walks and `RemoveHead` stay raw.

- `DirectInputMgr2::m_joysticks`: bounds-checked `GetJoystick(i)` (NULL out
  of range), the shape `Shutdown` expands; `CInputState::SelectDevices` and
  `CGruntzMgr::Run` call it. `PollJoysticks`/`ResetJoystickStates` index
  without the check in retail and stay raw.

- `CStatusBarMgr::m_tabLists`: `DELETE_STATUS_ITEMS(list)` deletes every item
  and empties the list (ResetWidgets, ClearTabGroup, SetTab, ExitMode). It is
  a macro: `ClearTabGroup` re-reads `m_activeTab` for `RemoveAll`, which an
  inline taking the index drops (100 -> 66.5). Render/refresh/hit-test walks
  stay raw.

- `CMenuPage::m_items`: the existing `NextItem`/`PrevItem` now cover every
  walk, including the column moves.

- `CDDrawDeviceManager::m_displayModes`: `GetModeDesc(i)` and
  `FreeDisplayModes()` (Clear, EnumerateDisplayModes); the merge and the
  enumeration callback use `CPtrArray::Add`, the sort swap `SetAt`.

- `SetAtGrow(GetSize(), x)` is `Add(x)` at 27 sites across the array
  families. `CGameLevel::ReadPlane`/`ReadObjectPlane` keep the longhand:
  `Add` drops both from 100 to about 95.7.

- `CGameLevel::m_imageSets`: the existing `CollisionAtHandle` now replaces
  its open-coded clear-check and image-set lookup at 9 sites (TriggerMgr,
  TriggerMgrGrid, BrickzCellFlags, TileTriggerContainer, Play, LookupTile,
  AxisProbe). `CPlay`'s raw tile-token lookups (`GetAt(tcidx)`, unmasked)
  stay raw.

- `CGameLevel::m_planes`/`m_imageSets`: `RELEASE_LEVEL_CHILDREN` is now only
  the delete-and-empty of both arrays and also covers `Unload`, which clears
  the viewport between it and the main-plane reset. `GetPlane` stays the
  checked accessor; the unchecked plane walks stay raw.

- `CGrunt::m_payloads`: `HeadPayload()` (NULL when empty),
  `DeleteHeadPayload()` and `DeleteAllPayloads()` replace the three
  open-coded drain loops (OnObjectRemoved, LoadStateRecord,
  LoadGruntTypeTable). Serialization walks stay raw.

- `CAniElement::m_records`: the existing checked `RecordAt(i)` replaces the
  open-coded `GetSize() > 0 ? GetAt(0) : NULL` and cast-around-`GetAt`
  forms at 13 sites (CGrunt, CAniAdvanceCursor, the `UserLogic.h` and
  `DEATH_FRAME` macros).

- `CGruntzMgr::m_stateStack`: accessed directly (the `CPtrArray* st` alias
  locals were byte-neutral); `TopState`/`PushState`/`PopTopIfMatches`/
  `ClearStateStack` are the owner's real out-of-line operations, so element
  casts inside them stay.

- `CPlay` coordinate arrays: `FreeStartMarkers()` and
  `FreePlacedObjectCells(group)` (in `PlayInline.h`; `Play.h` cannot take
  `CoordPool.h` without perturbing its includers) cover ReleaseResources,
  FreeListTeardown and LoadPlayState; count-based appends are `Add`. The
  camera bookmarks stay open-coded: ReleaseResources resets
  `m_cameraBookmarkIndex` between the recycle loop and `SetSize`.

## Open

For each collection, decide which of these the source was:

- a standard MFC member operation not yet modeled;
- an owner-specific typed inline accessor or operation (the holista skill);
- deliberate raw access (bulk serialization, storage management).

Method, per collection family:

1. Inventory the owner, element type, allocation source, teardown, indexing,
   insertion/removal policy, and serialization of every use
   (`scripts/audit-template-models.py` emits `mfc-pointer-members.json`).
2. Check surviving Monolith source (`config/lithtech_lineage.tsv`,
   `~/Projects/monolith-sources`) for the owner's own accessors.
3. Adopt a helper only when it explains every caller, applies at every site,
   and keeps every exact function exact. Leave a field raw rather than invent
   a per-site wrapper; a byte-flat cast reduction alone is not proof.

Surviving Monolith source linked into Gruntz (`libs/dibmgr`, and NOLF's
`LtWnd`) walks raw `CPtrList`s with a cast at each `GetNext`, so a per-site
cast in a plain walk is the era idiom, not missing API. Remaining leads:

- `CTileTriggerContainer::m_idleLogics`: `WireTileSwitchLogic` repeats the
  "record move on linked idle logics" walk per switch kind; an inline
  `RecordLinkedMoves(key)` drops it 90.29 -> 88.85 (function-scope
  `pos`/`anyHit` suggest copy-pasted cases).
- Coordinate recycle-and-empty blocks also open-coded for
  `CBattlezMapConfig::m_candArray` (FreeArrays, SerializeState; the
  waypoint loop differs in its NULL check), `CGruntzMapMgr::m_arr`,
  `CTriggerMgr::m_recList`, `CProjectile::m_hitList` and the
  `CGrunt::m_coordList` variants; no single helper explains them yet.
- Untouched owners (Wwd `CWwdGameObject::m_children`, Image `CDibMgr`,
  `CFontConfig`, `CWorldSoundSet`, `CVoiceManager`, `CDDSurface`, `CNetMgr`,
  `CGruntzCmdMgr`) walk or own their lists in out-of-line methods.

Scale now: about 237 casts around element access in 54 files (from 314 by
the same count).
