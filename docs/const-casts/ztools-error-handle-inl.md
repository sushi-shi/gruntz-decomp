# `zErrHandling::handle_inl`: inline error wrapper's identity key

[All sites](README.md). This page covers the second error-wrapper cast, in
[`zErrHandling::handle_inl`](../../include/ZTools/Error.h#L152).
Its sibling is documented [separately](ztools-error-handle.md).

## Our code and the conflicting declaration

```cpp
// zErrHandler:
void handle(void*, const char*, i32);

// zErrHandling:
void handle_inl(const char* s, i32 e) const {
    zMinErr::caller_ip = __ip();
    hp->handle(const_cast<zErrHandling*>(this), s, e);
}
```

`this` points to const because the wrapper is const-qualified. The receiving
identity parameter is `void*`, which cannot accept that pointer implicitly.
The cast changes that view; it neither copies nor modifies the receiver.
Unlike `handle`, this wrapper captures `__ip()` rather than `__caller_ip()`.
That distinction explains why both wrapper sites exist; it does not change
the pointer mismatch.

## Library declarations and GitHub example

No Windows SDK declaration forces this cast. The surviving ZTools
[`handle_inl` source](https://github.com/osgcc/no-one-lives-forever/blob/dfbe22fb4cc01bf7e5f54a79174fa8f108dd2f54/LT2/lithshared/incs/ztools.h#L128-L138)
declares the wrapper const and casts its receiver to the error handler's
mutable identity-pointer type. It is a direct source-family witness for this
contract. Our named cast replaces that source's C-style conversion; it is not
evidence that the original Gruntz source used the `const_cast` keyword.

The fetched header's blob is `8f548108894313b5b62d5cb6b17e6f078128cbd1`, from
revision `dfbe22fb4cc01bf7e5f54a79174fa8f108dd2f54`. The same identities are
recorded under `reassess-zerrhandler-typed-api` in the
[lineage ledger](../../config/lithtech_lineage.tsv).

Our declaration and cast were introduced by checkpoint
`a57322345f4c2fd993a8ac25e769eb61cba28645`, restoring that source API, and
published in the [PR #79 commit](https://github.com/sushi-shi/gruntz-decomp/commit/0335a5365115a8ab7d43891a42363109537dbf9f).
Later provenance must distinguish this adoption from mere formatting or file moves.

## Current use and safety

The shared [`zErrHandler::handle`](../../src/Bute/TypeKeyColl.cpp#L382) and
[`srch`](../../src/Bute/TypeKeyColl.cpp#L670) use the receiver as an identity
key and do not dereference it. Updates affect the error handler/table.
Therefore the inspected path does not write to a const receiver through this cast.

## What could remove it?

Propagating `const void*` through the complete identity-key API would remove
the need to drop constness here and in `handle`. That is a possible deliberate
library-interface cleanup. It is not required by the current source-backed
model, and should not be described as recovery of the original declaration
without further evidence.
