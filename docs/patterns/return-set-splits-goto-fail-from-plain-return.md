# The return SET says which guards are `goto fail` and which are a plain `return 0`
tags: cpp:goto cpp:guard cpp:eh | asm:ret asm:xor asm:je | topic:codegen-idiom
symptoms: diagnose says CFG with target rets > base rets; retail has one
`xor eax,eax`/`pop`/`ret` tail that several guards share AND one bare `pop`/`ret`
whose EAX is already zero from the guard's own `test`; ours funnels every guard
into a single shared `xor eax,eax` tail
confidence: 8/10

A multi-guard function that returns 0 from several places does NOT have one
return shape. cl gives a guard its OWN `pop/ret` when the tested value is
already in EAX (so EAX is provably 0 on the taken edge) and merges the rest into
a shared `xor eax,eax` tail. So the retail return set is a direct read of the
source: guards that land on the shared `xor` tail were written `goto fail`,
and a guard with its own zero-free `ret` was written as a plain `return 0`.

```cpp
    if (host == NULL) { goto fail; }        // -> je <shared xor tail>
    if (owner == NULL) { goto fail; }       // -> je <shared xor tail>
    ...
    if (key == NULL) { return 0; }          // -> jne <body>; pop esi; ret  (EAX already 0)
    ...
    if (rec == NULL) { goto fail; }         // -> jne <body>, fall into the tail
    ...
fail:
    return 0;
```
```asm
  0e7344: 85 c0        test eax,eax          ; eax = key
  0e7349: 75 04        jne  <body>
  0e734b: 5e           pop  esi              ; no xor - EAX is 0 on this edge
  0e734c: c2 2c 00     ret  0x2c
  ...
  0e7373: 33 c0        xor  eax,eax          ; the SHARED goto-fail tail
  0e7375: 5e           pop  esi
  0e7376: c2 2c 00     ret  0x2c
```
STEERABLE. `CSBI_ImageSet::SetupImage` 0xe72f0 was `if (host == NULL || owner ==
NULL) return 0;` with three more plain `return 0`s: 68.31%, 3 rets vs retail's 4.
Rewritten to the guard split above (the shape its 99.86% sibling
`CSBI_ImageSetAni::Init` 0xe7980 already used) it reaches retail's exact block
layout and 81.77%; the residue is the host/owner register colour.
