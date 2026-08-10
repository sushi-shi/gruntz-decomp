# `/Ob1` inline-budget harness (cl 5.0)

Reproduces the measurements in
`docs/patterns/inline-budget-emits-ool-comdat.md` § "The rule".
The decision model itself is reverse-engineered from the VC6 back end in the
sibling homm3 project (`homm3-decomp/docs/vc6/inliner.md`, executable as
`python3 -m homm3.vc6.inline_model --predict --spec sites.json`); everything
below is that model re-validated against **our** pinned cl 5.0.

```sh
# generate: S statements in the callee, N call sites, PAD statements of
# caller mass ahead of them
python3 tools/inline-budget/gen_harness.py S N PAD > h.cpp

# compile with the unit flags and count the sites cl REJECTED
wine "$MSVC_DIR/bin/cl.exe" /nologo /c /O2 /MT /GX /GR h.cpp
llvm-objdump -dr h.obj | grep -c 'IMAGE_REL_I386_REL32.*?leaf@@YAXH@Z'
```

`floor(1000 / cb) = expanded` at PAD=0 brackets a callee's front-end size
estimate `cb`; that is the titration used for every `cb` figure in the
pattern doc. Count rejections as `call` + tail `jmp` (cl tail-jump-optimizes
a rejected final site).

`/Ob0` enumerates a real function's candidate set - every inline expansion
becomes a visible `call`:

```sh
wine "$MSVC_DIR/bin/cl.exe" /nologo /c /O2 /MT /GX /GR /Ob0 -Iinclude \
    -Ivendor/miles-6.0c -Ivendor/sfman-1.01 -Ivendor/smacker-3.2f \
    -Ivendor/zlib-1.0.4 /Fo/tmp/u_ob0.obj src/<Module>/<TU>.cpp
llvm-objdump -dr /tmp/u_ob0.obj | awk '/<\?Fn@@...>:/{f=1;next} /^[0-9a-f]+ </{if(f)exit} f'
```
