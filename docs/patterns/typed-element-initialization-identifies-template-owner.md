# Typed element construction identifies the template owner above an erased base

tags: cpp:template cpp:container cpp:ctor | asm:call asm:loop | topic:identity topic:source-oracle

When a supposedly generic container method calls one specific element type's
constructor, inspect the complete typed wrapper above it. A template's expanded
body may have been assigned to its erased base and then given a separately
invented class name.

The Gruntz function at 0x310f0 was named `_zdvec::IndexToPtr`, yet its
retail tail reads initialization pointer/count fields, calls CString construction
and advances four bytes per element. The pinned original `ztools.h` places that
work in `zDArray<T>::operator[]` above `_zdvec::get`. The global construction and
destruction family independently agrees with `zDArray<CString>`. The audit and
exact source/blob references are in [the template audit](../template-model-audit.md).

The failed assumption was that the address arithmetic and erased return ABI
identified the base method. They did not: the element lifetime operation and
complete source family determine ownership. Likewise, a pointer and reference
return can share the machine ABI while exposing different source APIs.

For reverse use, pair the suspicious accessor with allocation, growth,
construction and destruction sites. Require agreement on element type, stride,
overflow behavior, initialization range, base layout and lifetime. Then restore
the complete template family, including its existing sibling specializations;
do not move a type-specific loop into a generic base to preserve one caller.

The negative control is `CButeTree`: a no-op destructor callback and erased
pointer payload fit several `zSymTab<T>` arguments. That evidence proposes a
template family but does not license an arbitrary specialization. The confirmed replacement now compiles: both typed index bodies and the
shared lifetime functions remain exact. The caller inlining residues remain
open; the complete application measurements are in the template audit.
