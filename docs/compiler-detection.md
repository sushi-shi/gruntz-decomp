# Compiler identification

The project pins MSVC 5.0 with the Visual Studio 97 SP3 toolchain. The packaging
checks are executable in
[create-toolchain-release.py](../scripts/create-toolchain-release.py):

```text
link.exe:    5.10.7303
cvtres.exe:  5.00.1668
required libraries: LIBCMT.LIB, NAFXCW.LIB
```

The retail PE's linker-version field and Rich-header records supplied evidence
for this selection. Those metadata do not by themselves recover each object's
compiler invocation, SDK revision, flags, or source boundaries. Matching against
retail with the pinned toolchain remains necessary.

The [historical fingerprint investigation](https://github.com/sushi-shi/gruntz-decomp/blob/b27b05deb249e4cacbb29f55f17b469ecfe56f26/docs/compiler-detection.md)
contains the original decode and provenance. Its software survey and session
commands are not current setup instructions.

Use [toolchain setup](toolchain-vc50-sp3.md) to reproduce the package and
[compiler profiles](compiler-flags.md) for the active manifest.
