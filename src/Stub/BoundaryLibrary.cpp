// Library-code stubs split out of Boundary.cpp by static classification
// (see docs/boundary-classification.tsv). These boundary FUN_ bodies are
// statically-linked MFC/CRT library functions FID did not anchor - kept as
// neutral 2-line stubs so they still delink + objdiff like any backlog stub,
// segregated out of the game-code matching worklist in Boundary.cpp.
//
// The CRT/STL block is MSVC5 LIBCMT iostreams (istream/ostream/streambuf/
// strstreambuf) in the iostream implementation region ~0x169000-0x16cd00 and
// the ostream-operator region ~0x191d20-0x192830 (virtual-inheritance ios:
// `mov [reg-0xc]` vbase reads, vtable stamps in the 0x5f03xx cluster).
#include <rva.h>

// ---- MFC ----
// (none: every NAFXCW library function in the boundary range was already FID-
//  anchored; the residue near MFC is game code that *uses* MFC collections.)

// ---- CRT/STL ----
RVA(0x001690f0, 0x70)
void Boundary_1690f0() {} // proximity: CAniRecord@-0x140 | strstreambuf@+0x70
RVA(0x00169610, 0xb7)
void Boundary_169610() {} // proximity: strstreambuf@-0x180 | CButeMgrHelper@+0x1b0
RVA(0x00169e10, 0x3c)
void Boundary_169e10() {} // proximity: CButeMgrHelper@-0x40 | istream@+0x600
RVA(0x00169ed0, 0xae)
void Boundary_169ed0() {} // proximity: CButeMgrHelper@-0x100 | istream@+0x540
RVA(0x0016a0a0, 0x14)
void Boundary_16a0a0() {} // proximity: CButeMgrHelper@-0x2d0 | istream@+0x370
RVA(0x0016a0c0, 0xb3)
void Boundary_16a0c0() {} // proximity: CButeMgrHelper@-0x2f0 | istream@+0x350
RVA(0x0016a180, 0xbd)
void Boundary_16a180() {} // proximity: CButeMgrHelper@-0x3b0 | istream@+0x290
RVA(0x0016a240, 0x13)
void Boundary_16a240() {} // proximity: CButeMgrHelper@-0x470 | istream@+0x1d0
RVA(0x0016a260, 0x72)
void Boundary_16a260() {} // proximity: CButeMgrHelper@-0x490 | istream@+0x1b0
RVA(0x0016a340, 0x69)
void Boundary_16a340() {} // proximity: CButeMgrHelper@-0x570 | istream@+0xd0
RVA(0x0016a3b0, 0x5b)
void Boundary_16a3b0() {} // proximity: CButeMgrHelper@-0x5e0 | istream@+0x60
RVA(0x0016a590, 0xae)
void Boundary_16a590() {} // proximity: istream@-0x80 | ostream@+0x590
RVA(0x0016a670, 0xec)
void Boundary_16a670() {} // proximity: istream@-0x160 | ostream@+0x4b0
RVA(0x0016a760, 0xb3)
void Boundary_16a760() {} // proximity: istream@-0x250 | ostream@+0x3c0
RVA(0x0016a820, 0xbd)
void Boundary_16a820() {} // proximity: istream@-0x310 | ostream@+0x300
RVA(0x0016a8e0, 0x13)
void Boundary_16a8e0() {} // proximity: istream@-0x3d0 | ostream@+0x240
RVA(0x0016a900, 0x72)
void Boundary_16a900() {} // proximity: istream@-0x3f0 | ostream@+0x220
RVA(0x0016a9e0, 0x69)
void Boundary_16a9e0() {} // proximity: istream@-0x4d0 | ostream@+0x140
RVA(0x0016aa50, 0x5b)
void Boundary_16aa50() {} // proximity: istream@-0x540 | ostream@+0xd0
RVA(0x0016aab0, 0x64)
void Boundary_16aab0() {} // proximity: istream@-0x5a0 | ostream@+0x70
RVA(0x0016ab70, 0x20)
void Boundary_16ab70() {} // proximity: ostream@-0x50 | streambuf@+0x500
RVA(0x0016abb0, 0x22)
void Boundary_16abb0() {} // proximity: ostream@-0x90 | streambuf@+0x4c0
RVA(0x0016abe0, 0x63)
void Boundary_16abe0() {} // proximity: ostream@-0xc0 | streambuf@+0x490
RVA(0x0016ac50, 0x72)
void Boundary_16ac50() {} // proximity: ostream@-0x130 | streambuf@+0x420
RVA(0x0016acd0, 0x73)
void Boundary_16acd0() {} // proximity: ostream@-0x1b0 | streambuf@+0x3a0
RVA(0x0016ad50, 0x8c)
void Boundary_16ad50() {} // proximity: ostream@-0x230 | streambuf@+0x320
RVA(0x0016aef0, 0xf7)
void Boundary_16aef0() {} // proximity: ostream@-0x3d0 | streambuf@+0x180
RVA(0x0016aff0, 0x74)
void Boundary_16aff0() {} // proximity: ostream@-0x4d0 | streambuf@+0x80
RVA(0x0016b410, 0x69)
void Boundary_16b410() {} // proximity: streambuf@-0x70 | CButeMgrHelper@+0x240
RVA(0x0016b480, 0x55)
void Boundary_16b480() {} // proximity: streambuf@-0xe0 | CButeMgrHelper@+0x1d0
RVA(0x0016b5b0, 0x9b)
void Boundary_16b5b0() {} // proximity: streambuf@-0x210 | CButeMgrHelper@+0xa0
RVA(0x0016b820, 0x113)
void Boundary_16b820() {} // proximity: istream@-0x100 | ostream@+0x4f0
RVA(0x0016b940, 0x63)
void Boundary_16b940() {} // proximity: istream@-0x220 | ostream@+0x3d0
RVA(0x0016b9b0, 0xbe)
void Boundary_16b9b0() {} // proximity: istream@-0x290 | ostream@+0x360
RVA(0x0016ba70, 0x93)
void Boundary_16ba70() {} // proximity: istream@-0x350 | ostream@+0x2a0
RVA(0x0016bb10, 0xad)
void Boundary_16bb10() {} // proximity: istream@-0x3f0 | ostream@+0x200
RVA(0x0016bbc0, 0x76)
void Boundary_16bbc0() {} // proximity: istream@-0x4a0 | ostream@+0x150
RVA(0x0016bc70, 0x7b)
void Boundary_16bc70() {} // proximity: istream@-0x550 | ostream@+0xa0
RVA(0x0016bcf0, 0x13)
void Boundary_16bcf0() {} // proximity: istream@-0x5d0 | ostream@+0x20
RVA(0x0016be90, 0x99)
void Boundary_16be90() {} // proximity: ostream@-0x100 | CButeMgrHelper@+0x230
RVA(0x0016bf30, 0x3c)
void Boundary_16bf30() {} // proximity: ostream@-0x1a0 | CButeMgrHelper@+0x190
RVA(0x0016c030, 0x8e)
void Boundary_16c030() {} // proximity: ostream@-0x2a0 | CButeMgrHelper@+0x90
RVA(0x0016c180, 0x76)
void Boundary_16c180() {} // proximity: CButeMgrHelper@-0xc0 | streambuf@+0xb80
RVA(0x0016c230, 0x7b)
void Boundary_16c230() {} // proximity: CButeMgrHelper@-0x170 | streambuf@+0xad0
RVA(0x0016c2b0, 0x13)
void Boundary_16c2b0() {} // proximity: CButeMgrHelper@-0x1f0 | streambuf@+0xa50
RVA(0x0016c2d0, 0x1f5)
void Boundary_16c2d0() {} // proximity: CButeMgrHelper@-0x210 | streambuf@+0xa30
RVA(0x0016c740, 0x8f)
void Boundary_16c740() {} // proximity: CButeMgrHelper@-0x680 | streambuf@+0x5c0
RVA(0x0016c800, 0x97)
void Boundary_16c800() {} // proximity: CButeMgrHelper@-0x740 | streambuf@+0x500
RVA(0x0016c8a0, 0xa2)
void Boundary_16c8a0() {} // proximity: CButeMgrHelper@-0x7e0 | streambuf@+0x460
RVA(0x0016c950, 0x64)
void Boundary_16c950() {} // proximity: CButeMgrHelper@-0x890 | streambuf@+0x3b0
RVA(0x0016ca00, 0x80)
void Boundary_16ca00() {} // proximity: CButeMgrHelper@-0x940 | streambuf@+0x300
RVA(0x0016ca80, 0x19e)
void Boundary_16ca80() {} // proximity: CButeMgrHelper@-0x9c0 | streambuf@+0x280
RVA(0x00191d20, 0xc9)
void Boundary_191d20() {} // proximity: CWwdGridIter@-0xf0 | ostream@+0x340
RVA(0x00191df0, 0x137)
void Boundary_191df0() {} // proximity: CWwdGridIter@-0x1c0 | ostream@+0x270
RVA(0x00191f30, 0xaa)
void Boundary_191f30() {} // proximity: CWwdGridIter@-0x300 | ostream@+0x130
RVA(0x00191fe0, 0x7e)
void Boundary_191fe0() {} // proximity: CWwdGridIter@-0x3b0 | ostream@+0x80
RVA(0x00192120, 0xb9)
void Boundary_192120() {} // proximity: ostream@-0xc0 | CFileIO@+0x2ceb7
RVA(0x001921e0, 0xb5)
void Boundary_1921e0() {} // proximity: ostream@-0x180 | CFileIO@+0x2cdf7
RVA(0x001922a0, 0xb9)
void Boundary_1922a0() {} // proximity: ostream@-0x240 | CFileIO@+0x2cd37
RVA(0x00192480, 0xbe)
void Boundary_192480() {} // proximity: ostream@-0x420 | CFileIO@+0x2cb57
RVA(0x00192540, 0xc8)
void Boundary_192540() {} // proximity: ostream@-0x4e0 | CFileIO@+0x2ca97
RVA(0x00192610, 0x13)
void Boundary_192610() {} // proximity: ostream@-0x5b0 | CFileIO@+0x2c9c7
RVA(0x00192630, 0xae)
void Boundary_192630() {} // proximity: ostream@-0x5d0 | CFileIO@+0x2c9a7
RVA(0x001926e0, 0x94)
void Boundary_1926e0() {} // proximity: ostream@-0x680 | CFileIO@+0x2c8f7
RVA(0x00192780, 0xa4)
void Boundary_192780() {} // proximity: ostream@-0x720 | CFileIO@+0x2c857
RVA(0x00192830, 0xfb)
void Boundary_192830() {} // proximity: ostream@-0x7d0 | CFileIO@+0x2c7a7
