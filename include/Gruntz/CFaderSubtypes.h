// CFaderSubtypes.h - the six concrete CFader screen-fader subtypes the
// CFaderMgr::Add factory allocates. Each is a real polymorphic CFader subclass
// (: public CFader): its ctor chains ??0CFader, stamps its own vftable, and zeroes
// its subtype fields; its two motion virtuals (v1/v2) override the CFader pure
// virtuals (slots 1/2). The ctor/dtor bodies live in CFader.cpp; this header is the
// single owner of the declarations so CFaderMgr.cpp can `new` the real subtype and
// call the inherited CFader::SetTimers/Set2c directly (no (CFaderImpl*) cross-cast,
// no (CFader*) upcast).
//
// nFaderType (0..5) -> subtype / pInit type-id / operator-new size:
//   0 -> CFader1816c0 (id 1, 0x494)   3 -> CFader17f9a0 (id 4, 0x5c)
//   1 -> CFader180410 (id 2, 0x206c)  4 -> CFaderFlat    (id 5, 0x50)
//   2 -> CFaderSine   (id 3, 0x7d5c)  5 -> CFader17e940  (id 6, 0x6c)
//
// Field names are placeholders (m_<hexoffset>); only offsets + code bytes are
// load-bearing. The per-subtype `operator new` returns the exact retail allocation
// size so `new CFaderXxx` emits `push <size>; call operator new` without padding the
// modeled (partial) layout. The default-init builder (CFaderInit::BuildXxx) and the
// apply/copy method (CFaderXxx::ApplyInit) are external/reloc-masked engine methods.
#ifndef GRUNTZ_GRUNTZ_CFADERSUBTYPES_H
#define GRUNTZ_GRUNTZ_CFADERSUBTYPES_H

#include <Ints.h>
#include <rva.h>

#include <Gruntz/CFader.h> // the polymorphic base (SetTimers/Set2c/virtual dtor)

// The default-init descriptor built on the CFaderMgr::Add stack when pInit is null:
// an embedded CString (~CString on every exit forces the /GX frame) plus the
// subtype's default parameters. Each subtype's ApplyInit consumes it; when pInit is
// supplied instead, the same engine method is reached through CopyFrom(CFader*)
// (pInit is a CFxMode transition descriptor the caller passes through CFader*).
// Defined in CFaderMgr.cpp.
struct CFaderInit;

// ===========================================================================
// CFader17e940 (ctor 0x17e940, size 0x6c): embeds a nested polymorphic sub-object
// at +0x58 (own vftable 0x5f07d8). See CFader.cpp for the ctor/member-order notes.
// ===========================================================================
SIZE_UNKNOWN(CFader17e940Sub);
struct CFader17e940Sub { // nested sub-object at +0x58 (own vftable 0x5f07d8)
    virtual void v0();   // one virtual -> its own vtable (reloc-masks 0x5f07d8)
    i32 m_04;            // +0x5c
    i32 m_08;            // +0x60
    i32 m_0c;            // +0x64
    i32 m_10;            // +0x68
    CFader17e940Sub() {
        m_04 = 0;
        m_10 = 0;
        m_0c = 0;
        m_08 = 0;
    }
};

SIZE(CFader17e940, 0x6c);
VTBL(CFader17e940, 0x001f07c0);
class CFader17e940 : public CFader {
public:
    CFader17e940();             // 0x17e940
    virtual void v1() OVERRIDE; // slot 1 -> 0x17ef00 (overrides CFader pure)
    virtual void v2() OVERRIDE; // slot 2 -> 0x17f120 (overrides CFader pure)

    void* operator new(u32) {
        return ::operator new(0x6c);
    }
    i32 ApplyInit(CFaderInit* src); // 0x17ea00 (apply the built default init)
    i32 CopyFrom(CFader* src);      // 0x17ea00 (same method; copy from the pInit descriptor)

    char _pad34[0x58 - 0x34]; // +0x34..+0x57
    CFader17e940Sub m_58;     // +0x58..+0x6b (member vptr + 4 fields)
};

// ===========================================================================
// CFaderSine (ctor 0x17fdb0, size 0x7d5c): motion virtuals 0x17ff30 / 0x180400.
// ===========================================================================
SIZE(CFaderSine, 0x7d5c);
VTBL(CFaderSine, 0x001f0848);
class CFaderSine : public CFader {
public:
    CFaderSine();                   // 0x17fdb0
    virtual ~CFaderSine() OVERRIDE; // 0x17fdf0
    virtual void v1() OVERRIDE;     // slot 1 -> 0x17ff30 (overrides CFader pure)
    virtual void v2() OVERRIDE;     // slot 2 -> 0x180400 (overrides CFader pure)

    void* operator new(u32) {
        return ::operator new(0x7d5c);
    }
    i32 ApplyInit(CFaderInit* src); // 0x17fe00 (apply the built default init)
    i32 CopyFrom(CFader* src);      // 0x17fe00 (same method; copy from the pInit descriptor)

    char _pad34[0x4c - 0x34]; // +0x34..+0x4b
    i32 m_4c;                 // +0x4c
    i32 m_50;                 // +0x50
};

// ===========================================================================
// CFaderFlat (ctor 0x17f530, size 0x50): motion virtuals 0x17f660 / 0x17f950.
// ===========================================================================
SIZE(CFaderFlat, 0x50);
VTBL(CFaderFlat, 0x001f07f8);
class CFaderFlat : public CFader {
public:
    CFaderFlat();               // 0x17f530
    virtual void v1() OVERRIDE; // slot 1 -> 0x17f660 (overrides CFader pure)
    virtual void v2() OVERRIDE; // slot 2 -> 0x17f950 (overrides CFader pure)

    void* operator new(u32) {
        return ::operator new(0x50);
    }
    i32 ApplyInit(CFaderInit* src); // 0x17f5e0 (apply the built default init)
    i32 CopyFrom(CFader* src);      // 0x17f5e0 (same method; copy from the pInit descriptor)

    char _pad34[0x4c - 0x34]; // +0x34..+0x4b
    i32 m_4c;                 // +0x4c
};

// ===========================================================================
// CFader180410 (ctor 0x180410, size 0x206c): motion virtuals 0x180640 / 0x1814f0.
// ===========================================================================
SIZE(CFader180410, 0x206c);
VTBL(CFader180410, 0x001f0870);
class CFader180410 : public CFader {
public:
    CFader180410();             // 0x180410
    virtual void v1() OVERRIDE; // slot 1 -> 0x180640 (overrides CFader pure)
    virtual void v2() OVERRIDE; // slot 2 -> 0x1814f0 (overrides CFader pure)

    void* operator new(u32) {
        return ::operator new(0x206c);
    }
    i32 ApplyInit(CFaderInit* src); // 0x1804a0 (apply the built default init)
    i32 CopyFrom(CFader* src);      // 0x1804a0 (same method; copy from the pInit descriptor)

    char _pad34[0x40 - 0x34]; // +0x34..+0x3f
    i32 m_40;                 // +0x40
};

// ===========================================================================
// CFader17f9a0 (ctor 0x17f9a0, size 0x5c): motion virtuals 0x17fc60 / 0x17fda0.
// ===========================================================================
SIZE(CFader17f9a0, 0x5c);
VTBL(CFader17f9a0, 0x001f0810);
class CFader17f9a0 : public CFader {
public:
    CFader17f9a0();             // 0x17f9a0
    virtual void v1() OVERRIDE; // slot 1 -> 0x17fc60 (overrides CFader pure)
    virtual void v2() OVERRIDE; // slot 2 -> 0x17fda0 (overrides CFader pure)

    void* operator new(u32) {
        return ::operator new(0x5c);
    }
    i32 ApplyInit(CFaderInit* src); // 0x17fa40 (apply the built default init)
    i32 CopyFrom(CFader* src);      // 0x17fa40 (same method; copy from the pInit descriptor)

    char _pad34[0x40 - 0x34]; // +0x34..+0x3f
    i32 m_40;                 // +0x40
    i32 m_44;                 // +0x44
    i32 m_48;                 // +0x48
    char _pad4c[0x50 - 0x4c]; // +0x4c
    i32 m_50;                 // +0x50
};

// ===========================================================================
// CFader1816c0 (ctor 0x1816c0, size 0x494): motion virtuals 0x181b00 / 0x182900.
// ===========================================================================
SIZE(CFader1816c0, 0x494);
VTBL(CFader1816c0, 0x001f0890);
class CFader1816c0 : public CFader {
public:
    CFader1816c0();             // 0x1816c0
    virtual void v1() OVERRIDE; // slot 1 -> 0x181b00 (overrides CFader pure)
    virtual void v2() OVERRIDE; // slot 2 -> 0x182900 (overrides CFader pure)

    void* operator new(u32) {
        return ::operator new(0x494);
    }
    i32 ApplyInit(CFaderInit* src); // 0x1817e0 (apply the built default init)
    i32 CopyFrom(CFader* src);      // 0x1817e0 (same method; copy from the pInit descriptor)

    char _pad34[0x44 - 0x34];    // +0x34..+0x43
    i32 m_44;                    // +0x44
    i32 m_48;                    // +0x48
    i32 m_4c;                    // +0x4c
    char _pad50[0x478 - 0x50];   // +0x50..+0x477
    i32 m_478;                   // +0x478
    char _pad47c[0x488 - 0x47c]; // +0x47c..+0x487
    i32 m_488;                   // +0x488
    i32 m_48c;                   // +0x48c
};

#endif // GRUNTZ_GRUNTZ_CFADERSUBTYPES_H
