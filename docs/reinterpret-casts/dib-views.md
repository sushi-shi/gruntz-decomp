# DIB descriptors and 16-bit pixels

Four casts live in [Image.h](../../include/Image/Image.h). They adapt the
surviving DIB library's byte storage and extended bitmap descriptor to Win32.

| Site | Expression | Source and destination |
| --- | --- | --- |
| `CDib::GetBuf16` | `reinterpret_cast<u16*>(m_pBytes)` | `u8*` to `unsigned short*` |
| `CDib::Blt(HDC,i32,i32)` | `reinterpret_cast<BITMAPINFO*>(&m_bmi)` | `DIB_BMI256*` to SDK `BITMAPINFO*` |
| `CDib::Blt(HDC,i32,i32,i32,i32)` | `reinterpret_cast<BITMAPINFO*>(&m_bmi)` | Same descriptor conversion, separate overload |
| `CDib::StretchBlt` | `reinterpret_cast<BITMAPINFO*>(&m_bmi)` | Same descriptor conversion, separate operation |

## What forces the conversions

Our actual storage is:

```cpp
struct DIB_BMI256 {
    BITMAPINFOHEADER m_hdr;
    RGBQUAD m_colors[256];
};
// CDib members:
DIB_BMI256 m_bmi;
u8* m_pBytes;
```

The build SDK's `msvc/include/WINGDI.H` declares `BITMAPINFO` with a
`BITMAPINFOHEADER bmiHeader` followed by `RGBQUAD bmiColors[1]` (line 546).
`StretchDIBits` accepts `const BITMAPINFO*` (line 2734). These two structure
types are distinct even though their header and color-table offsets agree.
The three functions pass the converted descriptor to that API; they do not
dereference it as an unrelated C++ class in our implementation.

[Microsoft's API contract](https://learn.microsoft.com/en-us/windows/win32/api/wingdi/nf-wingdi-stretchdibits)
interprets the table according to `DIB_PAL_COLORS` or `DIB_RGB_COLORS`.
This is a variable-sized Win32 record convention, not general permission to
alias arbitrary prefix-compatible C++ structures. The storage includes space
for 256 entries. Our eight-bit path initializes word-sized palette indices;
the other path passes RGB information. Those details, not just the cast, are
part of the contract.

`GetBuf16` instead returns a writable pixel view. Its sole current caller,
[`CDib::Init(HDC,CDib*,CDibPal*)`](../../src/Image/ImagePool.cpp), first creates
a 16-bit DIB and then writes RGB555 words. `CDib::Init(HDC,...)` obtains
`m_pBytes` from `CreateDIBSection`; `Term` deletes the bitmap. The pointer is
therefore an API-owned pixel allocation, not a declared `u8` member array.
The caller must keep the bitmap alive and use the view only for a compatible
pixel format. The accessor itself neither checks depth nor establishes
alignment or array bounds. A successful pointer conversion alone is no such
proof. Row pitch/dimension calculations remain separate obligations.

## Original-family source and Git provenance

The related LithTech
[dib.h](https://github.com/jsj2008/lithtech/blob/845119cd2da5bca34e8d87ee867f9b7cf999636c/libs/dibmgr/dib.h#L105-L115)
contains the word-returning accessor, and its
[inline blitters](https://github.com/jsj2008/lithtech/blob/845119cd2da5bca34e8d87ee867f9b7cf999636c/libs/dibmgr/dib.h#L207-L255)
convert the extended descriptor before calling `StretchDIBits`. This is an
actual source-family witness, stronger than an unrelated usage example.
Its blob is `60497eb009eb491926c9596dfce8d5f52a260a87`, exactly the
`src-libs-dibmgr-dib-h` entry in the [lineage ledger](../../config/lithtech_lineage.tsv).
The accompanying
[dib.cpp](https://github.com/jsj2008/lithtech/blob/845119cd2da5bca34e8d87ee867f9b7cf999636c/libs/dibmgr/dib.cpp)
also supplies the allocation/use family; its blob matches
`f1a29855a0bad5d0d72eac258ce130e1da25b717`.

[15e5ad72da](https://github.com/sushi-shi/gruntz-decomp/commit/15e5ad72da97750c00247af0fe0b74ce11b79ee0)
adopted the complete CDib family and introduced these named conversions.
The reason was recovery of that API and owner, not a claim that every cast
was independently proved portable. Related later source does not uniquely
recover Gruntz's original spelling.

## Disposition

Retain these four explicit boundaries: the inspected use is consistent with
the Windows DIB contract and source family. Replacing a cast with a
`void*` detour or an inactive union member would not improve its safety.
Any future caller of `GetBuf16` needs the same format, lifetime, alignment,
and extent review. This inventory does not certify the other descriptor and
palette conversions in `ImagePool.cpp`, which use different cast syntax.
