
## Companion screen and its false positives: the ORDERED STRING-REFERENT diff

Diffing the two sides' ordered string-literal referents finds a missing or
extra bute key / asset name, which the masked score barely registers. Run over
all 579 queue rows it produced three divergences; ALL THREE were artifacts, and
each names a distinct trap:

* **A hoisted/CSE'd string pointer serves two call sites.**
  `CStatusBarMgr::BuildTabzDialog` reads 16 dialog strings against retail's 15,
  with `..._QUITTOMAINMENU` twice on our side and once on retail's - which
  looks exactly like an extra widget in the single-player end-of-level dialog.
  It is not: both sides call `operator new` **16 times**, so the widget count
  is identical. Retail simply references that pooled literal once and reuses
  the pointer, while we emit two relocations (`diagnose` classes the row
  INLINE/CALL-SET: retail 78 relocs / 61 calls against our 77 / 62).
  **Count the allocations and calls before believing a string-count delta.**
* **Block placement reorders whole arms.** `CRollingBall::Update` has the same
  28 strings on both sides, with the FALL/FALLBALL pair and the SINK/SINKHOLE
  pair emitted in the opposite order - an arm-placement difference, not a
  different asset.
* **Cross-jump merge degree duplicates a key.** `CGrunt::LoadGruntTypeTable`
  shows retail with an extra `"Powerupz"`: retail duplicates the `Health1`
  arm's `GetInt` while cl merges all three health arms. Same keys, same
  default, same arithmetic.

So the screen is worth running - it is cheap and it reads a dimension scoring
ignores - but every hit needs the allocation/call census before it is called a
defect.
