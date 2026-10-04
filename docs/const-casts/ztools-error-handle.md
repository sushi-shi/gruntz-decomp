# `zErrHandling::handle`: const receiver used as an identity key

[All sites](README.md). This page covers the cast in
[`zErrHandling::handle`](../../include/ZTools/Error.h#L147).
The separate `handle_inl` site is documented [here](ztools-error-handle-inl.md).

## Our code and the conflicting declaration

```cpp
// zErrHandler:
void handle(void*, const char*, i32);

// zErrHandling:
void handle(const char* s, i32 e) const {
    zMinErr::caller_ip = __caller_ip();
    hp->handle(const_cast<zErrHandling*>(this), s, e);
}
```

The wrapper's final `const` makes `this` a pointer to const `zErrHandling`.
It can convert implicitly to `const void*`, but not to the callee's `void*`.
The mismatch concerns the receiver identity, not the error-message string:
both declarations already accept `const char*` for the message.

## Library declarations and GitHub example

There is no Windows SDK type at this boundary. The relevant vendor is the
ZTools library. Its surviving
[`ztools.h`](https://github.com/osgcc/no-one-lives-forever/blob/dfbe22fb4cc01bf7e5f54a79174fa8f108dd2f54/LT2/lithshared/incs/ztools.h#L103-L138)
has the same const wrapper and mutable `void*` identity parameter. It bridges
them with a C-style cast of `this`. Our named cast expresses that const removal
explicitly. This GitHub example is the actual related source family, not a
recommendation to cast arbitrary const objects to mutable pointers.

The source revision is `dfbe22fb4cc01bf7e5f54a79174fa8f108dd2f54`; the fetched
header's blob is `8f548108894313b5b62d5cb6b17e6f078128cbd1`. These match
`reassess-zerrhandler-typed-api` in the [lineage ledger](../../config/lithtech_lineage.tsv).
The exact header can therefore be independently checked on GitHub, even if it
is missing from a local archive bundle.

Our qualifier and named cast came with checkpoint
`a57322345f4c2fd993a8ac25e769eb61cba28645`, restoring the sourced error-owner
API. Public blame points to its containing
[PR #79 commit](https://github.com/sushi-shi/gruntz-decomp/commit/0335a5365115a8ab7d43891a42363109537dbf9f).
They were not guessed from a Gruntz symbol.

## Current use and safety

[`zErrHandler::handle`](../../src/Bute/TypeKeyColl.cpp#L382) passes the key to
[`srch`](../../src/Bute/TypeKeyColl.cpp#L670). The search compares object
identities; it does not dereference the key. Error state is written to the
handler or its table, not through the caller's object pointer. The callback
receives the message and error number, not the key.

Consequently this inspected path does not turn the cast into a write to a
const object. This conclusion concerns const removal; it is not a blanket audit
of every operation in the error implementation.

## What could remove it?

An identity-only API could consistently use `const void*` across the table,
search, and registration interfaces. That would remove this mismatch without
making the receiver mutable. The present `void*` API is retained as a sourced
library contract, not because the machine ABI fundamentally requires it.
Changing only this call or only one declaration would leave the family inconsistent.
