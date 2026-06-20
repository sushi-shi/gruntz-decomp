# gruntz.nvim

In-editor [objdiff](https://github.com/encounter/objdiff) for the **gruntz**
binary-matching project. For the function under your cursor: its **target**
(retail) asm, its **base** (recompiled) asm, a **side-by-side diff**, and its
**match %** — without leaving Neovim to start the objdiff TUI. Plus a one-key
**build** that recompiles and tells you what moved.

Presentation only — no extra tool. Every asm/diff view is one
`objdiff-cli diff … --format json` invocation, and the function under the cursor
resolves through the data the pipeline already emits:

- `build/gen/symbol_names.csv` — the join table: retail address → mangled symbol
  + unit.
- `build/objdiff/report.json` — per-function match %.

The retail **address** in each function's `RVA(0x…, 0x…)` macro (see
`include/rva.h`) is the key. It moves with the function text, so cursor→function
resolution never goes stale — unlike line tables.

## Interface

`:Gruntz {target|base|diff|status|hints|autobuild}` (tab-completable) and
`:GruntzBuild [args]`.

Buffer-local chords on C/C++ buffers:

| chord | action                                                         |
|-------|----------------------------------------------------------------|
| `vt`  | **target** asm of the function at the cursor                   |
| `vb`  | **base** asm                                                   |
| `vd`  | **diff** — target ∣ base side-by-side (the "objdiff look")      |
| `vs`  | **status** — match-% overview, current unit's functions listed |
| `vB`  | **build** (`:GruntzBuild`)                                     |
| `V`   | **peek** — match % + metadata (size, rva, unit) in a float     |

- **target / base** open a reusable scratch split (`filetype=asm`, so mnemonics/
  registers/labels are syntax-highlighted); the header carries the symbol and its
  match %. Intra-function jump targets get synthetic labels (`.L1:` …) and
  branches reference them (`jne short .L1`) instead of raw offsets. When a **base**
  view is open and a build changes the base, it turns into a `previous build → now`
  diff — you see exactly what your edit did to the compiled output. (Target asm is
  fixed by the retail binary, so it just re-renders.)
- **diff** opens two scratch buffers in a native Neovim **diff** split (target
  left, base right): scrollbound, with diverging instructions highlighted. Each
  pane's winbar labels it `TARGET (retail)` / `BASE (recompiled)` with the % so
  the sides are never ambiguous. The jump labels also keep the diff tight — a
  branch matches on both sides even when its byte offset shifts.
- **status** lists every started unit by match % (worst first); the current
  file's unit is marked `>` and expanded to its functions. `<CR>` on a function
  row opens its diff.
- **peek** floats the current function's match % and metadata (size, retail
  address, unit progress) — instant, no objdiff call; inside any plugin view,
  `<CR>`/`V` instead follow a `0x…` (or status row) to that function's diff.
  `q` closes.

## Inline % hints

Like coc's inlay hints: every `RVA(0x…, 0x…)` line in a source buffer is tagged
with its function's match % as end-of-line virtual text, colored by how close it
is (`✓ 100%` green, partial yellow, low red, `— n/a` if not built). So the match
state is visible *as you read the code*, no command needed. Refreshed on
enter/save and after `:GruntzBuild`. Toggle with `:Gruntz hints`, or disable by
default with `hints = false` in `setup`.

`:GruntzLog` shows the resolved `objdiff-cli` path and every invocation — handy
when a stale dev shell ships an old binary.

## `:GruntzBuild` — build and report what moved

`vB` snapshots the current `report.json`, runs the build, and on success shows a
**compact corner popup** with the build time, the overall % change, and the
current unit's `before → after` deltas for the functions that moved (`↑`/`↓`/`+`,
capped — `vs` has the full table). The popup never steals focus (handy when the
async build lands while you're typing elsewhere) and fades after a few seconds.
Extra args pass through to the build, e.g. `:GruntzBuild build/objdiff/base/netmgr.obj`
to limit ninja to one TU.

> **Launch nvim from `nix develop .#build` for fast builds.** The build then runs
> `gruntz build` **directly** (~0.5–1s). Launched from the plain `nix develop`
> shell it still works, but each build is wrapped in `nix develop .#build`, which
> adds ~2.5s of shell-setup overhead *per build*. The plugin auto-detects which
> shell it's in (via `$MSVC_DIR`).

### Build on save (`:Gruntz autobuild`)

Off by default. When on, **saving any TU** (`*.c`/`*.cpp`/`*.cc`) kicks a quiet
incremental rebuild — **no log window**, just a small `building …` corner note
that closes into the usual result popup — so the **inline %s update right after
you save** (and any open `vd`/`vt`/`vb`/`vs` view re-renders in place, keeping
your cursor and scroll). The tight loop: edit a function, `:w`, watch its `%`
move and the diff update. Enable per
session with `:Gruntz autobuild`, or by default with `build_on_save = true` in
`setup`. (Skips the redundant init-on-shell-entry to stay snappy.) **Rapid saves
supersede:** a new build cancels the in-flight one (latest wins), so saving
several times quickly doesn't queue builds or race `ninja` on `build/`.

The `autobuild` and `hints` toggles are **remembered per checkout** in
`build/gruntz-nvim.json` (gitignored), so each worktree keeps its own setting
across restarts; a remembered toggle wins over the `setup` default.

## Requirements

`objdiff-cli` on `PATH` — launch nvim from the project dev shell (`nix
develop`). A populated `build/` (run `gruntz build` once if it's a fresh
checkout). The build command additionally needs the `.#build` shell, which it
enters itself.

## Install

**You usually don't have to.** The project dev shell wraps `nvim` so it
auto-loads this plugin — when you `nix develop` (either shell) you'll see:

```
[gruntz] nvim       : WRAPPED -> nvim now auto-loads editor/nvim (...)
```

The wrap is a small script on `PATH` that execs your own nvim with
`--cmd "set rtp^=…/editor/nvim"` (so your config and plugins are untouched, and
plain `nvim` outside the shell is unchanged). Defined in `flake.nix`
(`nvimShimHook`).

To load it **everywhere** (outside the dev shell too), add it to your plugin
manager instead — e.g. lazy.nvim, pointing at this in-repo directory:

```lua
{ dir = "~/Projects/gruntz/editor/nvim", ft = { "c", "cpp" } }
```

Optional config (defaults shown):

```lua
require("gruntz").setup({
  keymaps = true,            -- set false to bind your own
  hints = true,              -- inline match-% after each RVA(...)
  build_on_save = false,     -- rebuild quietly on every TU save
  split = "botright vsplit", -- where asm/status views open
})
```

Outside a gruntz checkout (no `config/units.toml` above the buffer) the plugin
stays inert.
