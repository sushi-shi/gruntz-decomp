# Targeting cursor: equal widths did not prove legal aliasing

This page accounts for **two removed expressions** in [CTriggerMgr::UpdateTargetingCursor](../../src/Gruntz/TriggerMgr.cpp#L419). The initial code passed its `i32 x, i32 y` argument slots to `WorldToViewport` using:

```cpp
reinterpret_cast<LONG*>(&x)
reinterpret_cast<LONG*>(&y)
```

Both were accompanied by a comment describing the alias as proven. The source pointers were `int*`: [Ints.h](../../include/Ints.h) defines `i32` as `int`. The destinations were `long*`: the actual VC5 SDK `msvc/include/WINNT.H` defines `LONG` as `long`.

## Callee and ownership contract

[CLevelPlane::WorldToViewport](../../src/Gruntz/ActionOptionsMenuBar.cpp#L381), declared in [DDrawWorkerHost.h](../../include/DDrawMgr/DDrawWorkerHost.h#L120), takes `LONG* px, LONG* py`. It repeatedly reads and writes both pointees while wrapping coordinates and translating from world to viewport space. It is not merely copying bytes, retaining an opaque address, or returning a pointer without dereferencing it.

The caller owns both by-value arguments for its entire invocation. In the affected tool-targeting arm it transforms a source `POINT` normally, then transforms the destination argument slots before checking reach and storing the preview destination. There is no allocation or escaped temporary. On x86 VC5 both scalar types are four-byte aligned and four bytes wide, but `int` and `long` remain distinct C++ types. Access through `long*` does not become legitimate solely because their representations match. The issue was incompatible typed access, not constness, lifetime or a short buffer.

## Corrected declarations

The function now declares and defines:

```cpp
i32 UpdateTargetingCursor(LONG x, LONG y);
```

The callee receives `&x, &y` directly. This makes the actual argument-slot objects match the type through which they are accessed, preserving the retail parameter-storage shape without adding invented coordinate temporaries. Existing callers pass integral pixel coordinates; Win32's range and stack width are unchanged. The inferred original parameter spelling is still not independently attested: this is the type consistent with the existing, actually dereferenced `LONG*` boundary.

The C++ mangled name changes from `?UpdateTargetingCursor@CTriggerMgr@@QAEHHH@Z` to `?UpdateTargetingCursor@CTriggerMgr@@QAEHJJ@Z`; identity metadata must follow that change. A safe alternative would copy into real `LONG` locals and back, but those extra homes are absent from the observed argument-slot contract. Replacing the casts with a union would retain the type-punning problem.

## Provenance and external comparison

[14f220d261](https://github.com/sushi-shi/gruntz-decomp/commit/14f220d26165871dfc8b23f7a3038ca9087e1f18) replaced a destination `POINT` temporary with casts of the two argument slots while recovering cursor topology. Its reasoning was that retail passed those slots directly and Win32 `LONG` had the same storage width. [5b83b5a543](https://github.com/sushi-shi/gruntz-decomp/commit/5b83b5a5437a00a62f9299febe36cc1032797b87) stabilized the proof comments. The assembly observation supports slot reuse; it does **not** prove that the source parameters were `int` or authorize dereferencing an `int` object through `long*`. Correcting the parameter types resolves that mistaken inference.

Microsoft's [pinned WordPad view code](https://github.com/microsoft/VCSamples/blob/9e1d4475555b76a17a3568369867f1d7b6cc6126/VC2010Samples/MFC/ole/wordpad/wordpvw.cpp#L796-L800) constructs a real `CRect` for the corresponding MFC rectangle API rather than reinterpreting unrelated scalar storage. This is an ownership/type-use analogue, not original Gruntz proof. The language distinction is also explicit in the standard draft's [typed-access rules](https://eel.is/c++draft/basic.lval#11): equal size is not the criterion for accessing an object's stored value through a different scalar type.

There are no remaining `reinterpret_cast` expressions at these two coordinate sites.
