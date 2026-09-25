# Runtime DLLs

[flake.nix](../flake.nix) owns the pinned runtime artifacts. The dev shell exposes
their directory as `GRUNTZ_RUNTIME`; the runtime package contains `MSS32.DLL`
and `SMACKW32.DLL`.

Build-time import libraries and runtime DLL implementations are different.
[implib.py](../scripts/gruntz/graph/implib.py) can reconstruct the import libraries
from retail's imports; that does not provide executable DLL implementations.

The SFMAN32 source in the flake is currently guarded by an availability flag and
an unpinned placeholder. Do not substitute an arbitrary same-named DLL: the API
must match Miles' use. Enabling it requires a reviewed source and content hash,
not removing the guard or weakening the fetch check.

`gruntz play` is the runtime entry point; consult its help for candidate versus
retail mode. Do not treat a successful link as a successful runtime check.
Generated installations and fetched binaries belong outside `docs/`.

Previous download searches and hashes are recoverable through Git history.
The flake, not a duplicated prose inventory, defines what is actually packaged.
