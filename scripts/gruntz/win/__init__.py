"""gruntz.win - the Windows port of flake.nix.

flake.nix provisions pinned tools and exports a dev-shell environment on Linux;
this package does the same on native Windows, where MSVC 5.0 runs WITHOUT wine.

- manifest.py : the hash-pinned artifact table (the flake `fetchurl`/derivations).
- fetch.py    : download + SHA256 verify + extract.
- provision.py: realise the artifacts into build/win-toolchain/ (the `nix build`).
- env.py      : the env-var map derived from that layout (the shellHook, minus wine).
- activate.py : emit activate.ps1 / activate.bat (so a shell can `nix develop`-style enter).
- bootstrap.py: the orchestrator behind the repo-root `bootstrap_windows.py`.

Run it via the repo-root launcher: ``py bootstrap_windows.py`` (or ``python -m
gruntz.win`` once scripts/ is on PYTHONPATH). See docs/windows-setup.md.
"""
