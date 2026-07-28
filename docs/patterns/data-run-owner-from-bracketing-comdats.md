# A `.data` global's owner TU is the object whose COMDAT literals BRACKET it
tags: data:layout data:attribution | topic:identity topic:tu-partition
symptoms: `.data` interleave, data-tu-order pool exemption, global defined in the wrong .cpp, ILT thunk DATA_SYMBOL, `_MultiPauseCallback`, `??_C@` literal run, `??_R0` type descriptor
confidence: 9/10

Retail's `.data` is laid out **one contiguous run per contributing `.obj`**, in link
order, and *within* a run the object's **ordinary `.data` comes FIRST, then that
object's own COMDATs** (`??_R0` RTTI type descriptors, then the `??_C@` string
literals it won). So an ordinary word sandwiched between two literal blocks belongs
to whichever object owns the block **after** it — and a `.text` band order that
matches the `.data` block order confirms the object identity.

That makes a stray global's real owner recoverable without any debug info:

1. Enumerate every `.text` base-relocation whose target lands in the window, and map
   the reloc SITE to its containing function (`symbol_names.csv` + `functions.json`).
   Each singly-referenced string literal names the object that WON it, i.e. the
   object whose run contains that address.
2. Read the block sequence around the global: `[…prev obj's literals…][ordinary
   words][next obj's RTTI + literals…]`. The global belongs with the block that
   FOLLOWS it.
3. Cross-check: one object contributes ONE run, so if a candidate owner already has a
   run elsewhere in `.data`, it cannot also own this one.

Worked (2026-07-28): `g_localVersion`/`g_remoteVersion`/`g_dplayAppGuid` were defined
in `Multi.cpp`, but CMulti's own run is `[0x211d88,0x2121e0)` — `g_dropPlayerId`,
`??_R0?AVCNetMgr@@@8`, then every `MULTI_*` literal. The three sit 8 KB lower at
`[0x20fa70,0x20fae0)`, bracketed by `CGruntzMapMgr::LoadAttributes`' `'Black'…'Brown'`
below and the CGameLevel area titles + `CGameMgr`/`CGruntzMgr` RTTI + every
`CGruntzMgr::Run`/`Close`/`HandleCommand` literal above — the same run that holds
`g_pendingFrame`. One `.obj` cannot own two runs, so they are GruntzMgr's.
`g_pAreaMgr` fell out the same way: it opens AreaMgr.obj's run, immediately before
`'IMAGEZ_%s'/'OBJECTZ_'/'SOUNDZ_%s'/'ANIZ_%s'`.

**The trap that hides this.** `gruntz.audit.data_tu_order` bands a `.cpp`'s `DATA()`
rows per storage class and exempts a band that swallows ≥4 other files' defs as a
"pool". A `DATA_SYMBOL()` naming an **ILT jmp-thunk** is a `.text` rva, not data — if
the band model counts it, the band starts near `0x1000`, becomes a pool, and the TU
stops being checked at all. Four such rows in `Multi.cpp` masked the real interleave
for a whole campaign. The audit now drops any row whose rva is in an executable
section (`gruntz.core.pe.PE().exec_ranges`).

```
0x0020fa4c STR  brickzload   'Black' 'Gold' 'Blue' 'Red' 'Brown'   <- prev obj's literals
0x0020fa70 DATA g_localVersion / g_remoteVersion / <unreferenced> / g_dplayAppGuid
0x0020fac8 DATA g_pendingFrame                                     <- SAME run
0x0020fae0 STR  gamelevel    'Gruntz in Space' …                   <- this obj's literals
0x0020fb98 RTTI .?AVCGameMgr@@ .?AVCGruntzMgr@@ .?AVCMapMgr@@ …
0x0020fc1c STR  CGruntzMgr::Run's 'RezSync' 'General' …
```
