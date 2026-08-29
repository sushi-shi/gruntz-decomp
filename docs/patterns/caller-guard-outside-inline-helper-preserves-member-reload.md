# A caller guard outside an inline helper preserves the guarded member reload

## Detection signature

Several call sites expand the same typed helper.  Each expansion tests a member and
then uses that member again, but the base keeps the tested value in a register while
retail reloads it from the object after the successful branch.  Calls, branches,
returns, relocations, and arithmetic all agree.

The reload can distinguish these two source layers even though VC5 inlines both:

```cpp
DrawRect(object, rect);              // DrawRect tests rect.left internally

if (rect.left != invalid) {
    DrawRect(object, rect);          // DrawRect owns only translation and drawing
}
```

With the guard inside the helper, C1 exposes the test and following arithmetic as one
expression region and C2 reuses the value.  With the guard in the caller, the inlined
helper body begins a distinct ownership region and retail's reload survives.

## Controlled A/B

`CDDrawChildGroup::DrawObjectDebugGeometry` at `0x15a210` repeats the operation for
area, switch, and extent rectangles:

| Source shape | Size / instructions | Fuzzy |
|---|---:|---:|
| typed helper owns guard and body | `0x426` / 345 | 93.7333% |
| caller guard + nested four-add helper | target topology / 348 | 94.11% |
| caller guard + typed draw helper body | `0x444` / 351 | 95.51% |

The caller-guard states reproduce all three target reloads after the sentinel tests.
The direct-body form is the retained humane base.  The remaining three instructions
are one accumulator copy per expansion, not missing behavior or an API call.

Negative controls on the new base were bounded: reversing the commutative left sum and
introducing explicit left/top locals are byte-flat at 95.51%; the literal aggregate
initializer reaches 92.38%; copying the rectangle and applying four `+=` operations
reaches 80.76% and changes the enclosing loop allocation; keeping the guard inside and
adding only the nested four-add helper reaches 92.04%.  `TRect2<T>::Offset`, MFC
`CRect::OffsetRect`, and Win32 `OffsetRect` belong to different owners or add calls
absent from retail.

## Safe reverse use

When retail reloads a member immediately after a successful guard but the base reuses
the tested value, inspect which layer owns the guard.  If a repeated inline operation
has an independently meaningful precondition, test the precondition at each caller and
leave the helper responsible for the operation itself.  Confirm that the new state
creates a reload absent from the baseline before composing on the dip or improvement.
Do not infer a rectangle method merely from four additions; receiver identity and the
retail call set still decide whether an `Offset` API is authentic.
