# MenuSparkle: loading the upper delay bound

**Disposition:** repaired. The upper archive value is read into writable
local storage; animation timing continues to use the fixed bound. Both values
are consumed in order and existing return-value handling is preserved:

```cpp
i32 storedLo;
i32 storedHi;
arc->Read(&storedLo, sizeof(storedLo));
arc->Read(&storedHi, sizeof(storedHi));
```

This intentionally changes the retail read destinations. Loaded values are
not applied to const globals: retail animation consumers use folded constants,
and no supported mutable timing contract has been established. The source
below describes the **removed cast**, its actual defect and provenance.


## Our code and actual types

[MenuSparkle.cpp](../../src/Gruntz/MenuSparkle.cpp) previously contained:

```cpp
DATA(0x001ea3d8)
const i32 g_menuSparkleHi = 5000;

arc->Read(const_cast<i32*>(&g_menuSparkleHi), sizeof(g_menuSparkleHi));
```

The load occurs after the [lower-bound read](menu-sparkle-low.md), within `SERIAL_LOAD`. Its declaration in [MenuSparkleSerial.h](../../include/Gruntz/MenuSparkleSerial.h) is also const. [Ints.h](../../include/Ints.h) makes `i32` an `int`, four bytes under the target ABI. This is not merely an overqualified pointer to a mutable underlying object.

[CFileMemBase](../../include/Io/FileMem.h) declares:

```cpp
virtual i32 Read(void* buf, i32 n) = 0;
virtual i32 Write(const void* buf, i32 n) = 0;
```

Removing constness permits the pointer conversion needed by the writable `Read` interface, but does not make the object writable. Saving through `Write` is compatible with the object's type and requires no cast. [CFileMem::Read](../../src/DDrawMgr/DDrawSurfacePair.cpp) passes its buffer onward to MFC `CFile::Read` and checks the number of bytes read. Its own result is a success flag. The sparkle caller ignores that flag for both bounds and returns success after the calls; the second call is attempted even if the first returns failure normally.

The actual VC5 SDK contract is `virtual UINT Read(void* lpBuf, UINT nCount)` in `msvc/include/AFX.H:1232`. `UINT` is `unsigned int` in `WINDEF.H:141`. The count here is four, so signed-to-unsigned conversion is not the problem. MFC `CArchive::Read` separately has a writable `void*` parameter (`AFX.H:1727`), but our `CFileMemBase` is not MFC `CArchive`. The documented `CFileMem` backend reaches `CFile`; this audit does not establish which dynamic backend every runtime sparkle load uses.

## Observed retail destination

At image base `0x400000`, RVA `0x1ea3d8` / VA `0x5ea3d8` holds 5000. It is in `.rdata`, section characteristics `0x40000040`, with no `IMAGE_SCN_MEM_WRITE` flag. The serializer's mode-7 path pushes length four at RVA `0xae22e`, pushes this address at `0xae230`, and calls archive slot `+0x2c` at `0xae237`. Its mode-4 save path passes the same address and calls slot `+0x30` at `0xae25f`. [SerialArchive.h](../../include/Gruntz/SerialArchive.h) defines these modes as load and save. Thus the observed load destination is the upper-bound storage itself, not a local or instance field mislabeled by source prose.

The constructor and animation update use immediate divisor 4001 and addition 1000 rather than bound loads. Together with read-only placement this supports the reconstructed const-scalar model, but cannot prove that original source had two const scalar globals or these casts. Read-only section placement is an image property, not a recovered C++ type. A different aggregate or custom placement remains a hypothesis requiring evidence. The project's [qualifier-inference policy](../data-attribution.md#qualifiers-and-memory-protection) preserves this distinction.

Under the [C++ const-object rule](https://eel.is/c++draft/dcl.type.cv#4), constructing the cast is not itself undefined behavior; modifying its genuinely const target is. Runtime may instead fail in the archive/backend or fault on memory protection. No game execution or ordinary-gameplay reachability proof was performed, so neither a guaranteed fault nor a harmless unreachable path is established. An earlier lower-bound read could also fail or throw before this one runs.

## Git origin and why it changed

This is our reconstruction's history, not original-source attestation:

| Commit | Change and stated reason | Assessment |
| --- | --- | --- |
| [f4b8f21e](https://github.com/sushi-shi/gruntz-decomp/commit/f4b8f21e1b65c16ba985ccd155122e953e80dc4f) | Added mutable extern `g_5ea3d8` and the serializer; reported an instruction match. | No initialized definition or section proof; calling the target `.data` was incorrect. |
| [3ebde4c7](https://github.com/sushi-shi/gruntz-decomp/commit/3ebde4c78d1296adb95266985edbe66d40e0c06e) | Added a mutable definition initialized to 5000 while replacing extern-only storage. | The value is corroborated; ordinary writable storage was not established, and the commit described the retail scalars as `.rdata`. |
| [810685b7](https://github.com/sushi-shi/gruntz-decomp/commit/810685b7166b1d4d47e4b381251703c200005247) | Introduced the semantic upper-bound name. | No qualifier change. |
| [de654f00](https://github.com/sushi-shi/gruntz-decomp/commit/de654f00c13e39cd6b6284fdc574652e2084faaa) | Moved extern declarations to the MenuSparkle header. | Did not change the existing mutable definition. |
| [95720259](https://github.com/sushi-shi/gruntz-decomp/commit/95720259045535511fcf3c8b759c0ee59c5dee76) | Changed definition/declaration to const and added the cast, citing read-only section/data-attribution evidence. | Observed protection supports the model; claiming uniquely proven constness, original cast syntax, or guaranteed fault exceeded the evidence. |
| [5ce5a559](https://github.com/sushi-shi/gruntz-decomp/commit/5ce5a5591734cd05ab99f05f1675dea84853d09a) | Removed explanatory comments during prose pruning. | Retained the unsafe call; no independent site-specific safety reason was recorded. |

The repaired load does not claim that making the global mutable would be an authentic reconstruction correction.

## GitHub comparison: a writable CFile destination

Microsoft's [DibLook `ReadDIBFile`](https://github.com/microsoft/VCSamples/blob/9e1d4475555b76a17a3568369867f1d7b6cc6126/VC2008Samples/MFC/general/diblook/myfile.cpp#L171-L215) reads the file header into a non-const local `BITMAPFILEHEADER`, then reads pixel data into allocated, locked memory. Its historical pointer casts do not remove constness from those objects. This is an actual `CFile::Read` caller at the pinned Microsoft revision, illustrating writable destinations; it is not an analogue that makes a read-only global safe.

Its actual read expression is:

```cpp
file.Read((LPSTR)&bmfHeader, sizeof(bmfHeader))
```

For archive-owned members, Microsoft's [DrawCli load implementation](https://github.com/microsoft/VCSamples/blob/9e1d4475555b76a17a3568369867f1d7b6cc6126/VC2008Samples/MFC/ole/drawcli/drawobj.cpp#L50-L74) writes into [non-const pen/brush members](https://github.com/microsoft/VCSamples/blob/9e1d4475555b76a17a3568369867f1d7b6cc6126/VC2008Samples/MFC/ole/drawcli/drawobj.h#L68-L70). Neither sample establishes MenuSparkle's original ownership.

An original illustration of the writable-member alternative is:

```cpp
struct WritableSparkleBounds {
    i32 low;
    i32 high;
};
WritableSparkleBounds loaded = {1000, 5000};
i32 ok = arc->Read(&loaded.high, sizeof(loaded.high));
```

This is explanatory code, not an applied patch or recovered source. Replacing the retail destination with it changes destination identity, persistence and failure behavior. Making the existing global mutable changes storage and may change folded consumers. Such a repair must be described as a behavioral/storage change, not harmless cast removal. No safe byte-identical alternative has been established. The current repair intentionally changes the read destinations to writable locals.
