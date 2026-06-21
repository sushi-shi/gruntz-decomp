# Per-function codegen needs the TU's COMPLETE fn-set in ascending-RVA order — scattered = plateau
tags: topic:tu-layout topic:wall topic:codegen-idiom
symptoms: correct-shape functions stuck at 70-99% across a whole unit, 0 byte-exact, unit spans scattered RVAs
confidence: 8/10

MSVC5 /O2 builds each function's register allocation / scheduling / EH-state numbering from the
compiler's symbol table, which is a function of the **set and ORDER of functions in the TU**.
Retail lays each TU's functions out in source-definition order = **ascending RVA**, contiguous.
So a `.cpp` only byte-matches retail when it holds the SAME function set as the retail TU, in
ascending-RVA order. Cherry-picking scattered functions into a semantic grab-bag gives them the
wrong neighbors → the whole unit plateaus (correct shapes, 0 exact). FIX: define a class/TU's
functions in ascending-RVA order and COMPLETE the contiguous run; then the entropy resolves.
RVA-reordering already-green functions is a LAST RESORT (only when one is stuck and all other
options are exhausted — don't churn working functions). See `docs/link-order-investigation.md`.

WALL until the TU is complete + ordered. Evidence: grunt unit (17/19 reconstructed, 0 exact); iconloaders/spriteresource (conflate distinct retail TUs — split them).
