# Targeting cursor: equal widths did not prove legal aliasing

This page accounts for **two removed expressions** in [CTriggerMgr::UpdateTargetingCursor](../../src/Gruntz/TriggerMgr.cpp#L419). The former code passed its `i32 x, i32 y` argument slots to `WorldToViewport` using:

```cpp
reinterpret_cast<LONG*>(&x)
reinterpret_cast<LONG*>(&y)
```

The former comments described this alias as proven. That wording establishes neither valid typed access nor the original parameter types. The source pointers were `int*`: [Ints.h](../../include/Ints.h) defines `i32` as `int`. The destinations are `long*`: the actual VC5 SDK `msvc/include/WINNT.H` defines `LONG` as `long`.

## Callee and ownership contract

[CLevelPlane::WorldToViewport](../../src/Gruntz/ActionOptionsMenuBar.cpp#L381), declared in [DDrawWorkerHost.h](../../include/DDrawMgr/DDrawWorkerHost.h#L120), takes `LONG* px, LONG* py`. It repeatedly reads and writes both pointees while wrapping coordinates and translating from world to viewport space. It is not merely copying bytes, retaining an opaque address, or returning a pointer without dereferencing it.

The caller owns both by-value arguments for its entire invocation. In the affected tool-targeting arm it transforms a source `POINT` normally, then transforms the destination argument slots before checking reach and storing the preview destination. There is no allocation or escaped temporary. On x86 VC5 both scalar types are four-byte aligned and four bytes wide, but `int` and `long` remain distinct C++ types. Access through `long*` does not become legitimate solely because their representations match. The issue is incompatible typed access, not constness, lifetime or a short buffer.

## Corrected parameter ownership

The declaration in [TriggerMgr.h](../../include/Gruntz/TriggerMgr.h) and its definition now agree on `i32 UpdateTargetingCursor(LONG x, LONG y)`. The callee accepts `LONG*`, so the caller can pass its parameter slots directly:

```cpp
m_world->GetLevel()->m_mainPlane->WorldToViewport(&x, &y);
```

The by-value coordinate objects therefore have the type through which they are read and written. No extra buffer, cast, union, or escaped pointer is needed. Callers still supply coordinate values; this changes the reconstructed C++ parameter types and mangled identity while retaining two signed 32-bit arguments in the target ABI. It is supported by the callee's typed contract and observed argument-slot reuse, not claimed as a uniquely recovered original declaration.

An initial combined cleanup comparison lost the cursor's prior result. Isolating the retained `MapLookupById` temporary-output change identified a separate shared-helper cause: restoring that helper recovered the cursor, and reapplying this `LONG` repair preserved the recovery. Consequently this coordinate repair remains, while the map's unsafe output-reference seam is explicitly deferred in [typed maps](typed-maps.md). The comparison does not excuse that remaining aliasing hazard.

## Provenance and external comparison

[14f220d261](https://github.com/sushi-shi/gruntz-decomp/commit/14f220d26165871dfc8b23f7a3038ca9087e1f18) replaced a destination `POINT` temporary with casts of the two argument slots while recovering cursor topology. Its reasoning was that retail passed those slots directly and Win32 `LONG` had the same storage width. [5b83b5a543](https://github.com/sushi-shi/gruntz-decomp/commit/5b83b5a5437a00a62f9299febe36cc1032797b87) stabilized the proof comments. The assembly observation supports slot reuse; it does **not** prove that the source parameters were `int` or authorize dereferencing an `int` object through `long*`. Changing the parameter types could resolve that mistaken inference.

Microsoft's [pinned WordPad view code](https://github.com/microsoft/VCSamples/blob/9e1d4475555b76a17a3568369867f1d7b6cc6126/VC2010Samples/MFC/ole/wordpad/wordpvw.cpp#L796-L800) constructs a real `CRect` for the corresponding MFC rectangle API rather than reinterpreting unrelated scalar storage. This is an ownership/type-use analogue, not original Gruntz proof. The language distinction is also explicit in the standard draft's [typed-access rules](https://eel.is/c++draft/basic.lval#11): equal size is not the criterion for accessing an object's stored value through a different scalar type.

Both coordinate casts and their overstrong proof comments have been removed. No runtime game test was performed.
