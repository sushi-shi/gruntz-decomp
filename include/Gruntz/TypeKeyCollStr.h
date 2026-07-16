// TypeKeyCollStr.h - the "out of memory" error literal owned by TypeKeyColl.cpp
// (CZErrSink::Set uses its address; DATA()-bound in TypeKeyColl.cpp). A NARROW, owner-
// only decl header: included solely by TypeKeyColl.cpp so the definition can drop the
// `extern "C"` keyword while keeping the exact C-linkage symbol + DATA() binding, WITHOUT
// putting this file-private literal into the widely-shared <Gruntz/TypeKeyColl.h> (which
// would ripple regalloc in unrelated TypeKeyColl.h includers via the decl-count butterfly).
// Used as an address ((u32)s_out_of_memory) - no scalar constant-propagation -> byte-neutral.
#ifndef GRUNTZ_TYPEKEYCOLLSTR_H
#define GRUNTZ_TYPEKEYCOLLSTR_H

extern "C" const char s_out_of_memory[]; // 0x61adf4  CZErrSink error text

#endif // GRUNTZ_TYPEKEYCOLLSTR_H
