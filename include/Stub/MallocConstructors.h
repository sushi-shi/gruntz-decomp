#ifndef GRUNTZ_STUB_MALLOCCONSTRUCTORS_H
#define GRUNTZ_STUB_MALLOCCONSTRUCTORS_H
#include <rva.h>
// MallocConstructors.h - classes whose constructors gruntz.analysis.grind_ctors
// discovered at operator-new/malloc sites. Each carries the size measured at its
// allocation site (SIZE()); a real RTTI name where the class wasn't defined
// elsewhere, else a MallocCtor_<rva> placeholder (no recoverable name).

// non-RTTI ctor @ 0x0015b390, sizeof 0x1fc
class MallocCtor_15b390 {
public:
    MallocCtor_15b390();
    char m_data[0x1fc]; // operator-new size; real fields TBD
};
SIZE(MallocCtor_15b390, 0x1fc);

// non-RTTI ctor @ 0x00192390, sizeof 0x64
class MallocCtor_192390 {
public:
    MallocCtor_192390();
    char m_data[0x64]; // operator-new size; real fields TBD
};
SIZE(MallocCtor_192390, 0x64);

// non-RTTI ctor @ 0x00169ad0 (size TBD)
SIZE_UNKNOWN(MallocCtor_169ad0);
class MallocCtor_169ad0 {
public:
    MallocCtor_169ad0();
};

// non-RTTI ctor @ 0x00169fb0, sizeof 0x5c
class MallocCtor_169fb0 {
public:
    MallocCtor_169fb0();
    char m_data[0x5c]; // operator-new size; real fields TBD
};
SIZE(MallocCtor_169fb0, 0x5c);

// non-RTTI ctor @ 0x001698c0, sizeof 0x58
class MallocCtor_1698c0 {
public:
    MallocCtor_1698c0();
    char m_data[0x58]; // operator-new size; real fields TBD
};
SIZE(MallocCtor_1698c0, 0x58);

// non-RTTI ctor @ 0x0013aa10, sizeof 0x94
class MallocCtor_13aa10 {
public:
    MallocCtor_13aa10();
    char m_data[0x94]; // operator-new size; real fields TBD
};
SIZE(MallocCtor_13aa10, 0x94);

// non-RTTI ctor @ 0x00169700, sizeof 0x5c
class MallocCtor_169700 {
public:
    MallocCtor_169700();
    char m_data[0x5c]; // operator-new size; real fields TBD
};
SIZE(MallocCtor_169700, 0x5c);

// non-RTTI ctor @ 0x0016c570 (size TBD)
SIZE_UNKNOWN(MallocCtor_16c570);
class MallocCtor_16c570 {
public:
    MallocCtor_16c570();
};

// non-RTTI ctor @ 0x0013cac0 (size TBD)
SIZE_UNKNOWN(MallocCtor_13cac0);
class MallocCtor_13cac0 {
public:
    MallocCtor_13cac0();
};

// istream::istream(streambuf*) @ 0x0016b510 - MSVC5 LIBCMT iostreams. sizeof 0x60
// (vbptr/_fGline/x_gcount = 0x0c + ios vbase 0x54). Carries the /GX EH frame and
// the most-derived-flag vbase init (constructs the ios vbase, stamps the istream
// vftable 0x5f045c, calls ios::init); LEFT STUBBED like the sibling ostream ctor
// (0x16bfa0) - the non-polymorphic model can't make cl emit the vbase-ctor vftable
// store identically. The istream member methods are matched in src/Crt/IStream.cpp.
SIZE_UNKNOWN(MallocCtor_16b510);
class MallocCtor_16b510 {
public:
    MallocCtor_16b510();
};

// non-RTTI ctor @ 0x00136180, sizeof 0x58
class MallocCtor_136180 {
public:
    MallocCtor_136180();
    char m_data[0x58]; // operator-new size; real fields TBD
};
SIZE(MallocCtor_136180, 0x58);

// non-RTTI ctor @ 0x0016bfa0 (size TBD)
SIZE_UNKNOWN(MallocCtor_16bfa0);
class MallocCtor_16bfa0 {
public:
    MallocCtor_16bfa0();
};

// non-RTTI ctor @ 0x00139bf0, sizeof 0x30
class MallocCtor_139bf0 {
public:
    MallocCtor_139bf0();
    char m_data[0x30]; // operator-new size; real fields TBD
};
SIZE(MallocCtor_139bf0, 0x30);

// non-RTTI ctor @ 0x00184960 (size TBD)
SIZE_UNKNOWN(MallocCtor_184960);
class MallocCtor_184960 {
public:
    MallocCtor_184960();
};

// non-RTTI ctor @ 0x00139c80 (size TBD)
SIZE_UNKNOWN(MallocCtor_139c80);
class MallocCtor_139c80 {
public:
    MallocCtor_139c80();
};

// non-RTTI ctor @ 0x00168e70 (size TBD)
SIZE_UNKNOWN(MallocCtor_168e70);
class MallocCtor_168e70 {
public:
    MallocCtor_168e70();
};

#endif
