// FaderSubtypes.h - the six concrete CFader screen-fader subtypes the
// CFaderMgr::Add factory allocates. Each is a real polymorphic CFader subclass
// (: public CFader): its ctor chains ??0CFader, stamps its own vftable, and zeroes
// its subtype fields; its two motion virtuals (v1/v2) override the CFader pure
// virtuals (slots 1/2). The ctor/dtor bodies live in CFader.cpp; this header is the
// single owner of the declarations so CFaderMgr.cpp can `new` the real subtype and
// call the inherited CFader::SetTimers/Set2c directly (no (CFaderImpl*) cross-cast,
// no (CFader*) upcast).
//
// nFaderType (0..5) -> subtype / pInit type-id / operator-new size:
//   0 -> CFaderShape (id 1, 0x494)   3 -> CFaderRadial (id 4, 0x5c)
//   1 -> CFaderLight (id 2, 0x206c)  4 -> CFaderFlat    (id 5, 0x50)
//   2 -> CFaderSine   (id 3, 0x7d5c)  5 -> CFaderMesh  (id 6, 0x6c)
//
// Field names are placeholders (m_<hexoffset>); only offsets + code bytes are
// load-bearing. The per-subtype `operator new` returns the exact retail allocation
// size so `new CFaderXxx` emits `push <size>; call operator new` without padding the
// modeled (partial) layout. The default-init builder (CFaderInit::BuildXxx) and the
// apply/copy method (CFaderXxx::ApplyInit) are external/reloc-masked engine methods.
#ifndef GRUNTZ_GRUNTZ_CFADERSUBTYPES_H
#define GRUNTZ_GRUNTZ_CFADERSUBTYPES_H

#include <Mfc.h> // afx-first: CFaderInit embeds a CString (breakable-MFC-wall; all 3
                 // includers already pull <Mfc.h>, so this is matching-neutral)
#include <Ints.h>
#include <rva.h>

#include <Gruntz/Fader.h> // the polymorphic base (SetTimers/Set2c/virtual dtor)

// The default-init descriptor built on the CFaderMgr::Add stack when pInit is null:
// an embedded CString (~CString on every exit forces the /GX frame) plus the
// subtype's default parameters. Each subtype's ApplyInit consumes it; when pInit is
// supplied instead, the same engine method is reached through CopyFrom(CFader*)
// (pInit is a CFxMode transition descriptor the caller passes through CFader*).
// Defined in CFaderMgr.cpp.
// The default-init descriptor CFaderMgr::Add builds on its stack when pInit is null:
// nine int parameter words + an embedded CString (the destructible member that forces
// the /GX frame), filled by one of the six reloc-masked default builders (one per
// fader type 0..5). Each subtype's ApplyInit reads a different subset of the int words
// (they overlay the same bytes): CFaderSine reads m_04(src)/m_08(alt)/m_0c(count)/
// m_10(intensity); CFaderMesh casts to its local FxTransDesc view of the same offsets.
// +0x00 is the type discriminator. Was a per-TU FxDesc_17fe00 view + a FaderMgr.cpp-
// local blob; folded to this one canonical shape.
SIZE_UNKNOWN(CFaderInit);
struct CFaderInit {
    i32 m_00;      // +0x00  type discriminator
    i32 m_04;      // +0x04  src override (FaderSrc*, else this->m_timerA)
    i32 m_08;      // +0x08  alt/dst override (else this->m_timerB)
    i32 m_0c;      // +0x0c  count/param -> m_40
    i32 m_10;      // +0x10  intensity (Sine) / gate (Mesh)
    i32 m_14;      // +0x14
    i32 m_18;      // +0x18
    i32 m_1c;      // +0x1c
    i32 m_20;      // +0x20
    CString m_str; // +0x24  destructible member (forces /GX)

    void BuildDefaultInit0(); // 0x17e7c0  fader type 0
    void BuildDefaultInit1(); // 0x17e840  fader type 1
    void BuildDefaultInit2(); // 0x17e880  fader type 2
    void BuildDefaultInit3(); // 0x17e8b0  fader type 3
    void BuildDefaultInit4(); // 0x17e8e0  fader type 4
    void BuildDefaultInit5(); // 0x17e910  fader type 5
};

// Animation frame source (the CFaderSine/Flat active src/dst box): +0x18 the frame
// count, +0x1c the per-frame element count. Was FxSrc_17fe00 / a Fader.cpp-local view.
SIZE_UNKNOWN(FaderSrc);
struct FaderSrc {
    char pad00[0x18];
    i32 m_frameCount; // +0x18  frame count (w)
    i32 m_1c;         // +0x1c  element count
};
class CDDSurface; // the DDraw surface CFaderLight's overlay-pool members point at (matcher-7)

// ===========================================================================
// CFaderMesh (ctor 0x17e940, size 0x6c): embeds a nested polymorphic sub-object
// at +0x58 (own vftable 0x5f07d8). See CFader.cpp for the ctor/member-order notes.
// ===========================================================================
SIZE_UNKNOWN(CFaderMeshSub);
struct CFaderMeshSub { // nested sub-object at +0x58 (own vftable 0x5f07d8)
    virtual void v0(); // one virtual -> its own vtable (reloc-masks 0x5f07d8)
    i32 m_04;          // +0x5c
    i32 m_08;          // +0x60
    i32 m_0c;          // +0x64
    i32 m_10;          // +0x68
    CFaderMeshSub() {
        m_04 = 0;
        m_10 = 0;
        m_0c = 0;
        m_08 = 0;
    }
};

SIZE(CFaderMesh, 0x6c);
VTBL(CFaderMesh, 0x001f07c0);
class CFaderMesh : public CFader {
public:
    virtual ~CFaderMesh() OVERRIDE;
    CFaderMesh();                    // 0x17e940
    virtual void v1(i32 f) OVERRIDE; // slot 1 -> 0x17ef00 (overrides CFader pure)
    virtual i32 v2() OVERRIDE;       // slot 2 -> 0x17f120 (overrides CFader pure)

    void* operator new(u32) {
        return ::operator new(0x6c);
    }
    i32 ApplyInit(
        CFaderInit* src
    ); // 0x17ea00 (apply the transition descriptor; body in CFaderMeshApply.cpp)
    i32 CopyFrom(CFader* src); // 0x17ea00 (same method; copy from the pInit descriptor)

    // ApplyInit latches the transition descriptor into these fields, then walks an
    // (m_50 x m_54) grid emitting projected mesh records into the m_58 buffer.
    // m_38/m_3c hold the active dst/src box pointers (stored as dwords via SetTimers).
    i32 m_38;           // +0x38  active dst box (ptr-as-dword)
    i32 m_3c;           // +0x3c  active src box (ptr-as-dword)
    i32 m_40;           // +0x40
    i32 m_44;           // +0x44
    i32 m_48;           // +0x48
    i32 m_4c;           // +0x4c  record-order flag
    i32 m_50;           // +0x50  columns
    i32 m_54;           // +0x54  rows
    CFaderMeshSub m_58; // +0x58..+0x6b  growable mesh buffer (member vptr + 4 fields)
};

// ===========================================================================
// CFaderSine (ctor 0x17fdb0, size 0x7d5c): motion virtuals 0x17ff30 / 0x180400.
// ===========================================================================
SIZE(CFaderSine, 0x7d5c);
VTBL(CFaderSine, 0x001f0848);
class CFaderSine : public CFader {
public:
    CFaderSine();                    // 0x17fdb0
    virtual ~CFaderSine() OVERRIDE;  // 0x17fdf0
    virtual void v1(i32 f) OVERRIDE; // slot 1 -> 0x17ff30 (overrides CFader pure)
    virtual i32 v2() OVERRIDE;       // slot 2 -> 0x180400 (overrides CFader pure)

    void* operator new(u32) {
        return ::operator new(0x7d5c);
    }
    i32 ApplyInit(CFaderInit* src); // 0x17fe00 (apply the built default init; body in Fader.cpp)
    i32 CopyFrom(CFader* src);      // 0x17fe00 (same method; copy from the pInit descriptor)

    // ApplyInit latches the source boxes + geometry, range-checks the 0..100 intensity,
    // computes the scaled magnitude (m_54) via the FP pipeline, then fills four parallel
    // 2000-int arrays (three zeroed, one seeded with rand()%count) and scatters the last.
    FaderSrc* m_38;           // +0x38  active source box (else CFader::m_timerA)
    FaderSrc* m_3c;           // +0x3c  active alt/dst source box (else CFader::m_timerB)
    i32 m_40;                 // +0x40  count/param (=1 when m_3c is null)
    char _pad44[0x4c - 0x44]; // +0x44..+0x4b
    i32 m_4c;                 // +0x4c  frame count (source +0x18)
    i32 m_50;                 // +0x50  element count (source +0x1c)
    i32 m_54;                 // +0x54  scaled magnitude (intensity * scale * frames)
    i32 m_58;                 // +0x58  intensity (0..100)
    i32 m_arr0[2000];         // +0x5c
    i32 m_arr1[2000];         // +0x1f9c  seeded with rand()%count
    i32 m_arr2[2000];         // +0x3edc
    i32 m_arr3[2000];         // +0x5e1c  scattered, handed to ScatterSamples
};

// ===========================================================================
// CFaderFlat (ctor 0x17f530, size 0x50): motion virtuals 0x17f660 / 0x17f950.
// ===========================================================================
SIZE(CFaderFlat, 0x50);
VTBL(CFaderFlat, 0x001f07f8);
class CFaderFlat : public CFader {
public:
    virtual ~CFaderFlat() OVERRIDE;
    CFaderFlat();                    // 0x17f530
    virtual void v1(i32 f) OVERRIDE; // slot 1 -> 0x17f660 (overrides CFader pure)
    virtual i32 v2() OVERRIDE;       // slot 2 -> 0x17f950 (overrides CFader pure)

    void* operator new(u32) {
        return ::operator new(0x50);
    }
    i32 ApplyInit(CFaderInit* src); // 0x17f5e0 (apply the built default init)
    i32 CopyFrom(CFader* src);      // 0x17f5e0 (same method; copy from the pInit descriptor)

    char _pad38[0x3c - 0x38]; // +0x38..+0x3b
    FaderSrc* m_src;          // +0x3c  animation source (frame count at +0x18)
    char _pad40[0x44 - 0x40]; // +0x40..+0x43
    i32 m_percent;            // +0x44  duration-scale percent (v2)
    char _pad48[0x4c - 0x48]; // +0x48..+0x4b
    i32 m_4c;                 // +0x4c
};

// ===========================================================================
// CFaderLight (ctor 0x180410, size 0x206c): motion virtuals 0x180640 / 0x1814f0.
// ===========================================================================
SIZE(CFaderLight, 0x206c);
VTBL(CFaderLight, 0x001f0870);
class CFaderLight : public CFader {
public:
    virtual void v3() OVERRIDE; // slot 3
    virtual void v4() OVERRIDE; // slot 4
    virtual ~CFaderLight() OVERRIDE;
    CFaderLight();                   // 0x180410
    virtual void v1(i32 f) OVERRIDE; // slot 1 -> 0x180640 (overrides CFader pure)
    virtual i32 v2() OVERRIDE;       // slot 2 -> 0x1814f0 (overrides CFader pure)

    void* operator new(u32) {
        return ::operator new(0x206c);
    }
    i32 ApplyInit(CFaderInit* src); // 0x1804a0 (apply the built default init)
    i32 CopyFrom(CFader* src);      // 0x1804a0 (same method; copy from the pInit descriptor)
    void SubFree180630();           // 0x180630 (dtor member teardown; reloc-masked)

    // Overlay-pool state read by v3/v4 (AddItem/DropItem). The surface POOL itself is
    // the CFader base's dual-role +0x2c slot (m_set2cArg, read as CDDrawPtrCollections*
    // here). (This is a partial layout for v3/v4; the fuller CFaderLightApply flat view
    // in LightEffectSetup.cpp is the pending complete-fold onto this class.)
    CDDSurface* m_38;           // +0x38  active surface (v3 Blt destination)
    char _pad3c[0x40 - 0x3c];   // +0x3c
    CDDSurface* m_40;           // +0x40  current pooled overlay surface (0 = none)
    char _pad44[0x48 - 0x44];   // +0x44
    i32 m_48;                   // +0x48  emit gate
    char _pad4c[0x2060 - 0x4c]; // +0x4c..+0x205f
    i32 m_2060;                 // +0x2060  active span count (gate)
    i32 m_2064;                 // +0x2064  surface width
    i32 m_2068;                 // +0x2068  surface height
};

// ===========================================================================
// CFaderRadial (ctor 0x17f9a0, size 0x5c): motion virtuals 0x17fc60 / 0x17fda0.
// ===========================================================================
SIZE(CFaderRadial, 0x5c);
VTBL(CFaderRadial, 0x001f0810);
class CFaderRadial : public CFader {
public:
    virtual ~CFaderRadial() OVERRIDE;
    CFaderRadial();                  // 0x17f9a0
    virtual void v1(i32 f) OVERRIDE; // slot 1 -> 0x17fc60 (overrides CFader pure)
    virtual i32 v2() OVERRIDE;       // slot 2 -> 0x17fda0 (overrides CFader pure)

    void* operator new(u32) {
        return ::operator new(0x5c);
    }
    i32 ApplyInit(CFaderInit* src); // 0x17fa40 (apply the built default init)
    i32 CopyFrom(CFader* src);      // 0x17fa40 (same method; copy from the pInit descriptor)
    void FreeBuffer17fc40();        // 0x17fc40 (dtor: `if(m_50) RezFree(m_50)`; reloc-masked)

    char _pad38[0x40 - 0x38]; // +0x38..+0x3f
    i32 m_40;                 // +0x40
    i32 m_44;                 // +0x44
    i32 m_48;                 // +0x48
    char _pad4c[0x50 - 0x4c]; // +0x4c
    i32 m_50;                 // +0x50
};

// ===========================================================================
// CFaderShape (ctor 0x1816c0, size 0x494): motion virtuals 0x181b00 / 0x182900.
// ===========================================================================
SIZE(CFaderShape, 0x494);
VTBL(CFaderShape, 0x001f0890);
class CFaderShape : public CFader {
public:
    CFaderShape();                   // 0x1816c0
    virtual ~CFaderShape() OVERRIDE; // slot 0 -> 0x181720 (body in Obj5f0890Dtor.cpp)
    virtual void v1(i32 f) OVERRIDE; // slot 1 -> 0x181b00 (overrides CFader pure)
    virtual i32 v2() OVERRIDE;       // slot 2 -> 0x182900 (overrides CFader pure)

    void* operator new(u32) {
        return ::operator new(0x494);
    }
    i32 ApplyInit(CFaderInit* src); // 0x1817e0 (apply the built default init)
    i32 CopyFrom(CFader* src);      // 0x1817e0 (same method; copy from the pInit descriptor)

    char _pad38[0x44 - 0x38];    // +0x38..+0x43
    i32 m_44;                    // +0x44
    i32 m_48;                    // +0x48
    i32 m_4c;                    // +0x4c
    char _pad50[0x478 - 0x50];   // +0x50..+0x477
    i32 m_478;                   // +0x478
    char _pad47c[0x488 - 0x47c]; // +0x47c..+0x487
    i32 m_488;                   // +0x488
    i32 m_48c;                   // +0x48c
};

// --- vtable catalog (reduced-view classes share their base vtable rva) ---

#endif // GRUNTZ_GRUNTZ_CFADERSUBTYPES_H
