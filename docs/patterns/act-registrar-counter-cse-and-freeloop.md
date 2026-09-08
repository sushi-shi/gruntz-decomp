# Action registration: counter lifetime and template expansion

tags: cpp:local cpp:loop cpp:global cpp:template | asm:mov asm:push asm:dec asm:test | topic:codegen-idiom

The action registrar family exposes two independent facts: the integer ID's
lifetime, and the compiler's expansion of a typed array accessor. The old
hand-expanded implementation reached exact states, but its raw/typed/report
variants did not establish separate authored APIs. The surviving `zDArray<T>`
family now supplies the construction loop through `operator[]`.

## Counter lifetime

In the common create path, retail copies a loaded global counter into a
callee-saved ID while the value also feeds the name lookup. The established
source order is insertion, ID capture, name assignment, then increment:

```cpp
ActInsertId(key, g_typeCounter);
id = g_typeCounter;
g_typeColl[g_typeCounter] = key;
++g_typeCounter;
```

Earlier controlled builds of the hand-expanded body showed that replacing
the lookup's global argument with `id` could remove the scratch-register to
callee-saved-register copy. That observation describes two-consumer CSE in
that source family; it is not a blanket instruction to replace every local
argument with a global. The final registration block in `RegisterGruntActions`
uses the captured ID. Both source expressions remain meaningful after the
typed accessor restoration.

## The repeated loop constructs array elements

Retail calls `CString::CString`, never `CString::~CString`, on slots newly
allocated by `_zdvec::GrowTo`. The original audit checked 54 sites across 35
functions. Its initial destructor interpretation was wrong despite exact
scores under the older comparison pipeline. Ordered referents are required;
that historical failure is not the current strict-relocation contract.

The surviving primary owns the post-decrement construction loop:

```cpp
T* rv = AsElem(IndexToPtr(i));
T* p = AsElem(m_alloc);
for (i32 j = m_grown; j--; ++p) {
    T* t = new (p) T;
}
return *rv;
```

The old recommendation to spell `p->CString::CString()` in registrars is
withdrawn. Callers now use the typed accessor. The two emitted functions at
0x312a0 and 0x310f0 are respectively `_zdvec::IndexToPtr` and
`zDArray<CString>::operator[]`. A call to the first followed by a construction
loop is a nested inline cut; a call to the second retains the outer boundary.
It does not establish that developers deliberately chose two indexing APIs.

Both specialized indexers remain exact after replacing caller expansions.
The array-only follow-up takes `RegisterGruntActions` from 5.06812% to
97.792915% without per-site raw/typed implementations. Caller residues remain
open and all historical maxima are preserved. The controlled mechanism and
negative lifetime controls are documented in
[typed container use](typed-container-use-replaces-manual-compiler-methods.md).
