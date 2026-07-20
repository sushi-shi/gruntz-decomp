#include <Ints.h>
#include <Gruntz/TypeKeyColl.h>
#include <rva.h>

RVA(0x00008710, 0x2b)
zDArray* zDArray::Construct(i32 lo, i32 hi) {
    BaseConstruct(4, lo, hi, reinterpret_cast<void*>(1));
    *reinterpret_cast<volatile i32*>(&hi) = reinterpret_cast<i32>(m_alloc); // write-back to the hi param slot (retail keeps it)
    // vptr install dropped -> compiler-emitted vtable (% ok per drive-to-0)
    return this;
}
