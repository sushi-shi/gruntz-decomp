# MenuSparkle: loading the lower delay bound

**Disposition:** retained unsafe archive destination, with an operational warning. The cast is legal; writing through it would modify an object actually defined `const` in this reconstruction and therefore has undefined behavior. Retail supplies the same read-only address. Neither fact proves the original source used this declaration or this cast.

## Our code and the writable-buffer contract

[MenuSparkle.cpp](../../src/Gruntz/MenuSparkle.cpp) defines and loads the object as follows; the call is in the `SERIAL_LOAD` branch of `CMenuSparkle::SerializeDispatch`:

```cpp
DATA(0x001ea3d4)
const i32 g_menuSparkleLo = 1000;

arc->Read(const_cast<i32*>(&g_menuSparkleLo), sizeof(g_menuSparkleLo));
```

[MenuSparkleSerial.h](../../include/Gruntz/MenuSparkleSerial.h) also declares it `extern const i32`. Thus this is not a writable object merely viewed through a const pointer. [Ints.h](../../include/Ints.h) defines `i32` as `int`, which is four bytes in the target VC5 ABI. The cast changes `const i32*` to `i32*`; ordinary conversion then supplies `void*`.

The actual interface in [FileMem.h](../../include/Io/FileMem.h) is:

```cpp
virtual i32 Read(void* buf, i32 n) = 0;
virtual i32 Write(const void* buf, i32 n) = 0;
```

`Read` needs writable output storage; `Write` consumes input and accepts const storage. The save call therefore needs no cast. [CFileMem::Read](../../src/DDrawMgr/DDrawSurfacePair.cpp) forwards the destination to `m_file.Read`, compares the byte count, and returns a success flag. It does not copy into a temporary buffer or make the destination writable.

The shipped VC5 SDK's `msvc/include/AFX.H:1232` declares `CFile::Read` as `virtual UINT Read(void* lpBuf, UINT nCount)`. `WINDEF.H:141` defines `UINT` as `unsigned int`. The four-byte count is representable in both the project's signed count and the SDK's unsigned count; no count conversion explains away the destination conflict. `CFileMemBase` is the project's virtual interface, not an alias for MFC `CArchive`. A concrete `CFileMem` backend reaches MFC `CFile`; the dynamic backend at every possible sparkle load has not been established.

## Retail facts and inference limits

The PE image has base `0x400000`. RVA `0x1ea3d4` / VA `0x5ea3d4` contains the four-byte value 1000 in `.rdata`, whose characteristics are `0x40000040` without `IMAGE_SCN_MEM_WRITE`. In the serializer's mode-7 path, the instruction at RVA `0xae222` passes that VA, the preceding instruction passes length four, and the call at `0xae229` uses the archive's `Read` vtable slot `+0x2c`. The mode-4 path supplies the same address to `Write` at slot `+0x30`. These are actual destination operands, not deductions from the names of the globals. [SerialArchive.h](../../include/Gruntz/SerialArchive.h) records the corresponding mode values.

The constructor and animation update use immediate range arithmetic: divisor 4001 and addition 1000, rather than loading the two bound objects. Read-only placement plus this folding supports the current const-scalar model. It does **not** uniquely establish original C++ constness, scalar versus aggregate ownership, or original `const_cast` syntax. A custom placement scheme is not ruled out merely by section flags, but no alternative source model has been established. See [qualifiers and memory protection](../data-attribution.md#qualifiers-and-memory-protection).

The [C++ rule for const objects](https://eel.is/c++draft/dcl.type.cv#4) distinguishes forming the cast from modifying its target. A write to this const object is undefined behavior in our source. A backend or operating system might instead reject the destination or fault. The audit did not execute the game or prove that ordinary gameplay reaches this load branch; a guaranteed crash is not established. This serializer also ignores the two `Read` return values.

## Git origin and why it changed

These commits are reconstruction history, not surviving original MenuSparkle source:

| Commit | Change and stated reason | What the evidence warrants |
| --- | --- | --- |
| [f4b8f21e](https://github.com/sushi-shi/gruntz-decomp/commit/f4b8f21e1b65c16ba985ccd155122e953e80dc4f) | Introduced mutable extern `g_5ea3d4` and serializer calls; reported matching instructions. | No definition or section proof; its `.data` description was mistaken. |
| [3ebde4c7](https://github.com/sushi-shi/gruntz-decomp/commit/3ebde4c78d1296adb95266985edbe66d40e0c06e) | Added initialized mutable storage to replace extern-only globals. | Value 1000 was correct; the commit itself identified retail `.rdata`, which did not justify ordinary writable storage. |
| [810685b7](https://github.com/sushi-shi/gruntz-decomp/commit/810685b7166b1d4d47e4b381251703c200005247) | Named the lower sparkle parameter. | Semantic rename only. |
| [de654f00](https://github.com/sushi-shi/gruntz-decomp/commit/de654f00c13e39cd6b6284fdc574652e2084faaa) | Moved extern declarations into the owner header while retiring catch-all globals. | Declaration ownership cleanup, not a storage change. |
| [95720259](https://github.com/sushi-shi/gruntz-decomp/commit/95720259045535511fcf3c8b759c0ee59c5dee76) | Added `const` and this cast after read-only-section/data-attribution findings. | The storage evidence is valid; claims that it proved the original qualifier/cast or guaranteed a fault were too strong. |
| [5ce5a559](https://github.com/sushi-shi/gruntz-decomp/commit/5ce5a5591734cd05ab99f05f1675dea84853d09a) | Pruned source prose, removing the explanations while retaining the cast. | No site-specific safety justification; behavior unchanged. |

The current warning restores the operational hazard without asserting a unique original declaration. This document does not remove the cast or alter storage.

## GitHub comparison: genuinely writable archive members

Microsoft's [DrawCli serialization implementation](https://github.com/microsoft/VCSamples/blob/9e1d4475555b76a17a3568369867f1d7b6cc6126/VC2008Samples/MFC/ole/drawcli/drawobj.cpp#L50-L74) loads pen and brush records through `CArchive::Read`. Their [member declarations](https://github.com/microsoft/VCSamples/blob/9e1d4475555b76a17a3568369867f1d7b6cc6126/VC2008Samples/MFC/ole/drawcli/drawobj.h#L68-L70) are non-const. This is a real writable-member example, not evidence that writing a const global is safe or that Gruntz used that class structure. The links pin Microsoft sample revision `9e1d4475555b76a17a3568369867f1d7b6cc6126`.

The sample's actual read is:

```cpp
ar.Read(&m_logpen,sizeof(LOGPEN));
```

For comparison only, the following is newly written illustrative code, not recovered Gruntz source or a quotation from that sample:

```cpp
struct WritableSparkleBounds {
    i32 low;
    i32 high;
};
WritableSparkleBounds loaded = {1000, 5000};
i32 ok = arc->Read(&loaded.low, sizeof(loaded.low));
```

This avoids the const-object violation because `loaded` is writable. Substituting it in Gruntz would change the read destination and subsequent state handling; making the existing global mutable would change its storage model and potentially constant folding. Either requires an explicit behavioral repair design, not a claim of source-neutral cast cleanup. No such substitution has been made. The [upper-bound site](menu-sparkle-high.md) has the same hazard independently.
