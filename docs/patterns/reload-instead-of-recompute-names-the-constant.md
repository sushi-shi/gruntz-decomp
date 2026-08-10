# A site that RELOADS a frame slot where you recompute names the constant you invented

tags: cpp:const cpp:struct cpp:local | asm:lea asm:add asm:mov | topic:mis-model topic:tooling
symptoms: a widget/rect builder stuck in the 70s-80s with `--branches --diff` clean; retail's
frame is bigger than yours by 4 bytes per shared constant; at some call sites retail computes
`lea eax,[by_reg+N]` and at others it just does `mov eax,[esp+M]` with no arithmetic anywhere
near; `gruntz.audit.immediates --strong` shows an `OURS-ONLY !0xNN` row
confidence: 10/10

When a builder fills the same `RECT` argument at many sites, the constants that are SHARED
between sites get CSE'd into frame slots and reloaded; the ones that are unique to a site are
computed in registers. So retail's own choice of *reload vs recompute* is a decoder for the
source constants:

* **recompute** (`lea eax,[ebp+0x1df]`, `add ecx,0x82`) — this site introduces the value.
* **reload** (`mov eax,[esp+0x48]` feeding `mov [ecx+4],eax`) — this site REPEATS a value an
  earlier site already wrote. If your source computes something different here, your constant
  is invented.

That is also where the frame difference comes from: each shared constant that has to survive a
call costs a dword, which is why `member-aggregate-copied-not-field-by-field.md`'s "the frame
size proves it" applies even when there is no aggregate local at all.

## Decoding it

`--diff` is enough for the single-site case (the recompute shows up as a `+`/`-` pair against a
reload). For the whole function, walk the retail disasm tracking `esp` — resetting it to the
frame base at every `call`, since every callee here is `__thiscall`/`__stdcall` and cleans its
own arguments — and record, per slot, which `lea <r>,[<by>+N]` last stored it:

```
102889 STORE S+0x2c  by+0x135      <- lea eax,[ebp+0x135]; mov [esp+0x48],eax
1030f8 LOAD  S+0x2c  by+0x135      <- mov eax,[esp+0x48];  mov [ecx+4],eax
```

The `LOAD` line is the answer for that site. Cross-check with `gruntz.audit.immediates
--strong`: a value the source has and retail does not is the constant to replace (mind the
caveat that the sieve does not count `lea` displacements, and that retail encodes a subtraction
as `add reg,0xffffffbb`, so `0x45` "missing" is a false positive).

## Evidence (2026-08-10, `src/Gruntz/`)

* `CStatusBarMgr::BuildStatusBarTabs` 0xffde0 — **78.20 → 95.98**. `SBICMD_DOCK_RIGHT`
  reloads both of dockLeft's slots, so the two dock buttons share one band and sit SIDE BY
  SIDE (we had them stacked); the five tab buttons share `by + 0x82 .. by + 0xad`, and the
  `by + 0x99` we had was in no retail immediate at all. Filed for three sessions as a
  register-allocation wall.
* `CStatusBarMgr::LoadTabSprites` 0x102250 — **86.68 → 91.63**. One site's top was
  `by + 0x1a6`; retail reloads the slot holding `by + 0x135`.

Both functions' `--branches --diff` was already 100% clean, which is exactly why the bug
survived: a wrong CONSTANT changes no branch, no block and no ret — only bytes.
