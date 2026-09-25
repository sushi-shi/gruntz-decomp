# Compiler profiles

[config/units.toml](../config/units.toml) is the executable manifest for compiler
profiles and per-TU selections. Inspect it before changing flags.

The standard profiles use MSVC 5.0 SP3:

```text
C:                /O2 /MT
C++:              /O2 /MT /GX
C++ with RTTI:    /O2 /MT /GX /GR
```

The manifest also has explicit no-EH profiles. Do not infer a project-wide switch
from one function's prologue or change flags solely to improve a local score.

Compile-profile compatibility is not recovery of the original IDE command line.
Multiple options can produce identical output on a selected witness. Historical
flag comparisons are available in
[the prior investigation](https://github.com/sushi-shi/gruntz-decomp/blob/b27b05deb249e4cacbb29f55f17b469ecfe56f26/docs/compiler-flags.md);
they do not establish every literal option or every TU's original setting.

For an actual profile change, retain a same-source control with the pinned
compiler. Compare complete code, data, calling conventions, ordered relocations,
and runtime-library directives across affected owners. Byte-neutral output on
one function does not establish that a flag is globally harmless.

See [compiler identification](compiler-detection.md) for binary provenance,
[the build system](build-system.md) for generated commands, and
[linking](linker-flags.md) for the separate candidate-image configuration.
