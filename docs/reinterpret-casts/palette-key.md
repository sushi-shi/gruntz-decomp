# Palette trailing-data loader: corrected string parameter

**Fixed:** the former integer-to-string-pointer cast concealed an unnecessarily integer-typed parameter. The complete function consumes that argument only as an optional name string, and its sibling APIs already express that domain. Its old justification cited a reconstructed symbol as though it were an original symbol.

## Original occurrence and final source

The one original cast in [DDrawSurfacePair.cpp](../../src/DDrawMgr/DDrawSurfacePair.cpp), `CDDrawPaletteRegistry::LoadPaletteFromTrailingData`, was:

```cpp
// key was an i32 parameter
const char* keyArg = reinterpret_cast<const char*>(key);
char buf[0x50];
if (keyArg != NULL) {
    strcpy(buf, keyArg);
} else {
    strcpy(buf, src->GetName());
}
```

The declaration in [DDrawPaletteRegistry.h](../../include/DDrawMgr/DDrawPaletteRegistry.h) and its definition now take `const char* key`. The body uses that parameter directly:

```cpp
char buf[0x50];
if (key != NULL) {
    strcpy(buf, key);
} else {
    strcpy(buf, src->GetName());
}
```

The same class's `LoadPaletteFromSource`, `CreatePaletteFromRgb` and `LoadPaletteFromFile` take optional `const char*` names. No direct source caller of the trailing-data member was found; it remains a virtual member, so absence of a named call does not prove unreachability. There is no supplied original C++ symbol proving that this parameter was an integer. Retyping changes reconstructed C++ mangling, while keeping the target's pointer-sized argument slot and vtable position; the combined build must verify the actual emitted body and metadata.

## Storage and safety

`src` supplies resource data. The function allocates a palette resource, initializes it from trailing data, and stores it in the registry's `CMapStringToOb` under the copied name. `key` is borrowed only for the immediate `strcpy`; the registry's string key does not retain the caller's character-buffer pointer. A non-null key must designate a live readable NUL-terminated string. Character alignment imposes no stronger requirement, and reading through `const char*` is appropriate for that storage.

The former conversion relied on preserving a string address in signed 32-bit `i32`. That is not an SDK requirement and would be width-unsafe if generalized to wider pointers. The corrected interface carries the address as a pointer throughout. This does **not** fix the separate unchecked `strcpy` into an 80-byte local buffer: an oversized key or fallback name can overflow it. No runtime length bound is proved here, and no unrelated buffer behavior was silently changed.

## Why the integer survived

| Commit | Evidence and interpretation |
| --- | --- |
| [e8d67ad1](https://github.com/sushi-shi/gruntz-decomp/commit/e8d67ad189db33c40ec4426c975230a3806659c1) | Earlier `Factory_165a90` reconstruction declared the second argument as `i32 a2` but ignored it, instead selecting a resource-field pointer. That body was incomplete evidence for the parameter's type. |
| [0c27a576](https://github.com/sushi-shi/gruntz-decomp/commit/0c27a576702946d7f77dc770d52b0d503ef96939) | Corrected the second argument's stack-slot use: it is the optional key string. Kept `i32` and an `AddrWord` conversion, claiming a “retail mangled name” containing `Factory_165a90@CDDrawWorkerMapSmall`. Those identities are reconstruction labels; the claim alone does not establish original integer typing. |
| [3b09d668](https://github.com/sushi-shi/gruntz-decomp/commit/3b09d668f2f3f65c69c40ad95f8e9afc5cdc30f0) | Removed the union overlay and introduced the visible `reinterpret_cast<const char*>`, preserving the earlier integer choice rather than independently proving it. |

The pinned [corrective stack-slot diff](https://github.com/sushi-shi/gruntz-decomp/commit/0c27a576702946d7f77dc770d52b0d503ef96939) and the class's sibling source are the concrete comparison for this site. They are explicitly reconstruction evidence, not a surviving Monolith declaration or an external example laundering the integer signature. Current [retail function inventory](../../config/retail/functions.tsv) has no original name attached to RVA `0x165a90`. The old cast has been removed based on the actual string domain; the audit does not claim uniquely recovered original naming. No external source is quoted.
