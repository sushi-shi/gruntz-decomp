# A read-modify-write on a member's member needs a LOCAL receiver — retail never emits `or [mem],imm`

tags: cpp:member cpp:local cpp:ctor | asm:or asm:mov | topic:codegen-idiom
symptoms: `or DWORD PTR [reg+disp],0x20000` (or any `<op> [mem],imm32`) in your obj where retail has the three-instruction register form `mov r,[p+8] / <store to p+N> / or r,imm / mov [p+8],r`; plus an extra `mov reg,[this+off]` reload right after the guarded store; 1-3% per site
confidence: 9/10

Retail's `GRUNTZ.EXE` contains **zero** `or DWORD PTR [r+d8],0x20000` and **59**
`or r32,0x20000`. cl5 only picks the register form when it can prove the store
that sits between the load and the OR cannot clobber the receiver pointer — which
is exactly what a LOCAL gives it. Spelled through the member three times,

```cpp
if (m_object->m_sortKey != K) {      // load m_object
    m_object->m_sortKey = K;         // store -> may alias m_object itself
    m_object->m_flags |= 0x20000;    // reload m_object; memory-form OR
}
```

cl must re-read `[this+0x10]` after the sort-key store and then folds the OR into
memory. Bind the receiver once and the whole block collapses onto retail's shape:

```cpp
CWwdGameObjectA* o = m_object;
if (o->m_sortKey != K) {
    o->m_sortKey = K;                // mov ecx,[eax+8] is hoisted ABOVE this
    o->m_flags |= 0x20000;           // or ecx,0x20000 / mov [eax+8],ecx
}
```

The local must be introduced **immediately before the guard**, not at the top of
the function: retail re-reads `m_object` for each *preceding* statement (an
intervening store to `o->m_screenX` can alias `m_object`), so a function-wide
local removes reloads retail has.

STEERABLE, and it is a whole-tree sweep, not a one-off: 35 sites, **24 functions
up / 3 marginally down**, e.g. CGruntHealthSprite ctor 94.28->97.63, CGruntToySprite
97.80, CStatusBarSprite / CGruntSelectedSprite 96.46, CToobSpikez 96.12,
CWormhole 94.25, CSecretTeleporterTrigger 96.32->97.88, CInGameText 95.83->97.01,
CKitchenSlime 98.25, CStaticHazard::LoadAttributes 49.70->52.65.

Inside a `switch` body wrap the site in braces (`C2360: initialization of 'o' is
skipped by 'case' label`). The counter for "did I get them all" is
`llvm-objdump -d build/objdiff/base/*.obj | grep 'orl .*0x20000, .*(%'` — drive it
to 0 and cross-check the same count in retail is 0.

related: [eh-ctor-vptr-restamp-position.md](eh-ctor-vptr-restamp-position.md)
(names the reg-vs-mem OR + base-ptr reload as the steerable co-residue of the
vptr-position wall).
