# gruntz — Claude working notes

Binary-matching decompilation of **Gruntz** (Monolith Productions, 1999), which
runs on the **WAP32** 2D engine (shared with *Claw* and *Get Medieval*). Goal:
C++ that, compiled with the original toolchain (**MSVC 5.0**), produces COFF
objects matching the retail `GRUNTZ.EXE`, verified with **objdiff**.

**Current stage: investigation + environment scaffolding.** The pipeline is
designed and the build env is wired, but the matching loop is **not running
yet** — there is no `scripts/`, orchestrator, or `match.db` here (those live in
the reference harness, see below). Don't assume vostok-style tooling exists in
*this* repo until it's been ported.

The full investigation, decisions, and ranked plan are in
**`REVERSE_ENGINEERING_PLAN.md`** — treat it as the source of truth and keep it
current when something fundamental changes.

## Tools come from Nix

- `nix develop` (default) — works today, **no MSVC**: `vostok-delinker`,
  `objdiff`/`objdiff-cli`, `ghidra`, `llvm-pdbutil`, python/rg/file/xxd/jq.
  Enough for analysis + **target-side delink** + objdiff.
- `nix develop .#build` — adds the MSVC 5.0 toolchain under `wine` for the
  **base/recompile** side. Needs the `gruntz-toolchain` tarball, which is **not
  built yet** (`flake.nix` uses `lib.fakeHash` as a placeholder — see plan §15).

`GRUNTZ_EXE` is exported pointing at the Internet-Archive-fetched binary.

## Target facts — MEASURED, do not re-derive

- `binaries/retail_en/GRUNTZ.EXE` — EN v1.0, 2,511,872 B, MD5 `81c7f648…`.
- Built with **MSVC 5.0** (PE optional-header linker **5.10**; Rich C/C++ module
  build **8034**, cvtres **1668**). **CRT + MFC statically linked.**
- **231 RTTI mangled class names**; `.reloc` **present**; **no debug directory /
  no PDB** (none ever shipped or leaked for any WAP32 game).
- Leaked source paths reveal the modular layout:
  `C:\Proj\{DDrawMgr,DinMgr2,Dsndmgr,NetMgr,Gruntz}\` over shared `incs\`.
- Engine siblings for cross-diff: `binaries/claw_retail/CLAW.EXE` (v1.2),
  `binaries/getmed_retail/MEDIEVAL.EXE`. Same base classes
  (`CGameApp/CGameMgr/CGameWnd/CWapX/CUserLogic`) + same toolchain. **Anchor diff
  pair: GRUNTZ ↔ CLAW.** Claw/GM have `.reloc` **stripped** → only Gruntz is
  delinkable.

## The pipeline (plan §14)

```
GRUNTZ.EXE → Ghidra (auto-analyse + RTTI + FLIRT + leaked names)
           → fake PDB (wandel/pdbgen; patch a CodeView entry into the PE first)
           → vostok-delinker → per-symbol COFF "target" objects
           → objdiff vs "base" objects compiled with MSVC 5.0 under wine
```

Gotchas baked in from reading the delinker source:
- It **hard-requires `.reloc`** and **panics unless every absolute `.text` reloc
  target is a named function** → Ghidra function coverage must be complete.
- It needs Public + per-module **Procedure** (length-carrying) + Data symbols;
  **no section contributions** (so pdbgen's output is structurally enough).
- Byte-matching needs the **exact** cl.exe (build 8034) — sourcing it is the
  long pole.

## Reference harness — `refs/vostok` (PRIVATE)

`refs/vostok` is a clone of `srp-survarium/vostok` (X-Ray 2.0 / Survarium
matching decomp), branch `feature/agentic-matching-loop-2`. It is the **proven
template** for everything here: the flake mechanism, `scripts/{rebuild,
generate_delink,generate_objdiff_config,match_db,match_score}.py`, the
`docs/binary_matching/*` notes, and the `.claude/` orchestrator + `/match` loop.
**Mirror and adapt** its scripts for Gruntz (VS2008→MSVC 5.0, real PDB→fake PDB)
— don't blind-copy. It is a **private** repo: keep its contents out of any public
history or external service. (`/refs/` is gitignored.)

`refs/tomalla-gruntz` is a partial from-scratch Gruntz recreation — harvest its
methodology (`@address`/`@offset`/`@vftable`/`@bug` annotations) and WAP32
base-class layouts; the author recommends restarting the codebase.

## Conventions

- **Don't commit** `binaries/` or `refs/` (game binaries are copyright; the
  vostok clone is private + 661 MB). The flake re-fetches the game binary.
- Once decomp source begins, adopt the vostok/tomalla annotation style
  (`@address: <rva>`, `@offset`, `@vftable`, `@bug`, `@todo`) and the MFC
  `C`-prefix naming recovered from RTTI.
- Keep `REVERSE_ENGINEERING_PLAN.md` and `README.md` current when the build/diff
  flow, tools, or paths change.
- `flake.lock` is committed; `.gitignore` already excludes generated outputs.
