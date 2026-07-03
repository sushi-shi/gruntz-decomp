# Access-mismatch candidates (0)

A cross-class, cross-TU retail caller calls these directly, but the symbol's access (mangled name) is not public - a non-public target mangles to a symbol the caller never emits, so the reloc will not pair. Make it public (the CLightEffect::Setup = Q lesson).

| callee rva | callee | current access | cross-class callers |
|---|---|---|---|
