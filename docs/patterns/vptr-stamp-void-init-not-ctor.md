# A missing constructor result can identify a destructor, not a manual vptr method

tags: cpp:constructor cpp:destructor cpp:implicit | asm:mov asm:xor | topic:identity topic:compiler-artifact

A standalone VC5 constructor returns `this` in EAX. A body that instead leaves
EAX holding a zero used for stores is evidence against that constructor model.
It does not distinguish an ordinary void method from a destructor. Inspect the
complete class lifetime, deleting destructor, vtable, and base/member cleanup
before naming the routine.

The earlier version of this pattern called 0xbb40
`CRandomAmbientSound::BaseInit`, manually assigned a vptr, and interpreted its
100% masked score as proof of that method. The complete class model disproves
that interpretation. The routine is `CRandomAmbientSound::~CRandomAmbientSound`;
0xb940 is its `CAmbientPosSound` sibling. Both retail bodies are fifteen bytes:

```asm
xor eax,eax
mov DWORD PTR [ecx],offset ??_7CUserBase@@6B@
mov DWORD PTR [ecx+4],eax
mov DWORD PTR [ecx+3ch],eax
ret
```

The two cleared fields are `CAmbientSound::m_sound` and `m_listNode`. Its authored
destructor clears those members; its `CUserBase` base supplies the compiler's
vptr store. Both derived sound classes need no destructor declaration.

At `d6cddd606`, removing the two derived empty inline destructors in the real
`worldsoundset` TU preserves all fifteen bytes and the single ordered DIR32
relocation to `??_7CUserBase@@6B@`. The compiler generates the complete cleanup
naturally, with the existing `RVA_COMPGEN` labels identifying its retail copies.
The source contains no manual vptr expression.

Reverse audit old init/teardown helpers that write a vptr and have no constructor
result. The missing EAX result is only the first discriminator; deleting/EH
callers, exact base/member cleanup, and ordered vtable identity select the
destructor interpretation. A masked match of a manual store never establishes
the source entity. See the complete
[omission controls](empty-special-member-calls-and-vptr-stores.md).
