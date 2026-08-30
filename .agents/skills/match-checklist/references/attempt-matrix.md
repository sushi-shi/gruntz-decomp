# Required source-form attempt matrix

Use this as a per-function execution checklist, not a list of vague ideas.
Copy the rows that can affect the selected body into its Function Match Plan;
dispose every row individually. A real-TU compile is the default. Skip a
compile only when retail instructions, ordered referents, source lineage,
historical source hashes, ABI/layout evidence, or call/CFG topology proves the
form inapplicable.

## Source and identity

- [ ] Recover the best historical source hash and isolate which body,
  declaration, helper population, or TU-state transition moved MAX.
- [ ] Search the complete surviving owner/family, repeated game/sample copies,
  sibling binaries, and paired Debug/Release objects.
- [ ] Verify owner TU, member/free/static identity, calling convention, receiver
  static type, parameter order, return type, cv/ref, overload, and mangling.
- [ ] Propagate signedness and storage width across callers and callees: `u8`,
  `char`, `bool`, word/dword, enum, pointer/integer, and SDK boundary types.
- [ ] Verify class/base/subobject/member identity, aggregate extent, field
  order, packing, and ordered relocation/addend identity.

## Inline, helper, macro, and operator forms

Record every applicable row separately; this section may never be summarized
as only “tried an inline helper.”

- [ ] Hand-expanded body versus an authentic helper boundary.
- [ ] Header `inline` member function, including receiver cv/ref qualification.
- [ ] Header `inline` free function or `static inline` helper.
- [ ] Macro expansion, especially when the body expands multiple times and
  each expansion may need independent allocation.
- [ ] Thin forwarding wrapper or overload adapter around the implementation.
- [ ] Accessor/getter/setter, `operator[]`, conversion operator, assignment
  operator, constructor, destructor, or small value-type method boundary.
- [ ] Nested helper composition versus one flat helper; test both only when the
  nested boundary is source-plausible.
- [ ] Member receiver versus free helper, and object/aggregate arguments versus
  scalar fields; preserve by-value/by-reference and const forms.
- [ ] Helper-internal local/out-param reset versus caller-owned local/reset.
- [ ] Helper statement, assignment, and store order, including semantic member
  order before transcribing scheduled stores.
- [ ] Repeated helper calls versus one cached result; repeated source calls can
  prevent CSE, while a real cached result may own the call-crossing lifetime.
- [ ] Inline expression versus named result/reference at the call site.
- [ ] Body visibility and declaration placement: header-visible, same-TU
  definition, declaration-only/out-of-line, and locally defined COMDAT evidence.
- [ ] Caller call-site population and `/Ob1` budget only after eligibility is
  proved; measure `cb` before using budget as a lever.
- [ ] Include/declaration order and nested expansion order when an authentic
  helper changes the caller's IL tuple population or EH state.

## Locals, parameters, scope, and aggregates

- [ ] Mutate/reuse a parameter or local in place versus define a fresh derived
  local; test the snapshot/cursor mirror when the original value is needed.
- [ ] Directly test a call versus bind a one-use result local.
- [ ] Remove or restore a one-use pointer/member/union copy local according to
  whether retail reloads or carries the value across calls.
- [ ] Named array-element pointer versus repeated indexed member access.
- [ ] Distinct old/new/result locals versus one deliberately reused result.
- [ ] Declaration order and first-use/creation order as separate A/Bs.
- [ ] Declaration initializer versus later assignment, `for` initializer,
  branch-local initializer, deliberate uninitialized local, and late store
  after the value's first consumer.
- [ ] Function scope versus nested/sibling block scopes; test whether disjoint
  phases should share a stack slot or remain live in separate homes.
- [ ] Address-taken local/out-param home, escaped member address, and reuse of a
  dead incoming-parameter home.
- [ ] Aggregate versus scalars, whole-object assignment versus field stores,
  named versus unnamed by-value temporary, and source-visible constructor.
- [ ] Loop parameter/cursor versus derived cursor, index versus pointer walk,
  and whether an induction value remains live after the loop.
- [ ] Cached receiver/global/member versus repeated reads at the authored use
  sites; inspect reloads across calls and inside loops.

## Statements, expressions, and control flow

- [ ] One expression versus sequenced statements; direct member stores versus
  one result passed through setters.
- [ ] Operand/evaluation order and grouping/parentheses where VC5 preserves the
  source boundary; record commutative byte-flat controls.
- [ ] Ternary versus split `if`, materialized predicate versus short-circuit
  expression, compound update versus copy/writeback, and clamp/min/max helper.
- [ ] Guard polarity, compare operand order, positive versus negative nesting,
  `switch` versus ordered `if` chain, and real default fallthrough.
- [ ] Separate early returns, explicit state/result carrier, shared tail,
  `goto fail`, total `||`/`&&` merge, and duplicated symmetric arms.
- [ ] Store order in both symmetric arms and distinct per-arm local identities
  where cross-jumping differs.
- [ ] `for`, `while`, guarded `do/while`, infinite loop plus `break`, explicit
  `continue`, post-decrement/countdown, and entry/latch update order.
- [ ] Loop invariant/cache placement before the guard, in the preheader, or in
  the body; treat a trampoline as a pressure readout first.
- [ ] Era-authentic `abs`, squared-distance, min/max/clamp, `memcpy`, `memset`,
  `strlen`, `strcmp`, MFC/Win32 helper, or project macro instead of a manual
  transcription.

## Ownership, EH, data, and compiler state

- [ ] `new`/`new[]`/`delete`/`delete[]`, constructor/destructor layer, guarded
  delete/nulling, and typed teardown adapter according to ownership evidence.
- [ ] Local destructible-object census, constructor order, EH states, cleanup
  target, and inline ctor/dtor cuts.
- [ ] Global/local-static/header-COMMON ownership, string/FP literal identity,
  raw constants, and ordered referents.
- [ ] First divergence absent from baseline before composing a dipped state;
  do not chase a feature the baseline already had.
- [ ] Only after every structural row is disposed: deterministic syntax
  variants for the diagnosed register/schedule feature.
- [ ] Only after body variants are bounded: disposable, measured TU/C1-state
  probes or N-island/M-frontier permutation; bank exact and remove probes.

## Completion rule

No unchecked row may remain when the function is declared exact or bounded.
An unsupported mutation is not mandatory, but its row still needs a concrete
`checked — no evidence` or `proved inapplicable` verdict. Preserve humane,
source-plausible code and the MAX gate throughout the final kept state.
