#ifndef MANAGERS_DDRAWMGR_SURFACE_FAMILY_H
#define MANAGERS_DDRAWMGR_SURFACE_FAMILY_H

#include <rva.h> // OVERRIDE macro (override under clang, no-op under MSVC 5.0)

/*
 * ========================== HYPOTHESIS — NOT CONFIRMED ==========================
 * The "DirectDraw surface / page-manager" class FAMILY.
 *
 * This is tomalla's `harry_potter.{h,cpp}` hierarchy (refs/tomalla-gruntz/gruntz/
 * harry_potter.h). tomalla's class NAMES are throwaway placeholders (Harry-Potter
 * characters) — NONE are real and NONE are in RTTI. We carry them VERBATIM for
 * traceability and tag the whole file as a HYPOTHESIS.
 *
 * HYPOTHESIS (strength: structure/offsets HIGH, identity MEDIUM, names LOW):
 *   This family IS the CDirectDrawMgr surface/page-manager group from
 *   C:\Proj\DDrawMgr\{DDRAWMGR,DIRPAL,DIRSURF}.CPP.
 *   Evidence:
 *     - The root manager (UnknownClassCGruntzMgrHarryPotter) is stored in
 *       CGruntzMgr @0x30 (m_..._maybeSurfaceRestoreHandler) and is constructed
 *       during CGruntzMgr init.
 *     - Its UnknownVirtualMethod18(hWnd, 0x280, 0x1e0, 0x10, flags)
 *       = (640, 480, 16bpp, flags) — a classic display-mode init.
 *     - UnknownSeverus owns a static UnknownDirectDrawStructure that is a
 *       DDSURFACEDESC-shaped struct (dwSize @0) it zeroes/sizes — classic DDraw.
 *     - UnknownRemus's ctor seeds a resolution/scaling ladder.
 *     - UnknownSalazar/UnknownVoldemort hold the 101-entry volume->attenuation
 *       lookup table (see enums.h GruntzVolumeAttenuation).
 *
 * @approx tomalla 1.0.1.77 — all OFFSETS / SIZES / vtable-SLOTS / INHERITANCE below
 * are version-independent (high confidence). The compilable declarations below
 * encode those offsets directly (clang -fdump-record-layouts reproduces them); the
 * ~120 method-body addresses tomalla recorded (mostly `throw NotImplemented`
 * stubs) are DEFERRED to the re-anchor and not modeled. Static-DATA @address values
 * (006c02xx/006c03xx) are 1.0.1.77 data anchors, carried as approximate.
 *
 * Inheritance spine (verbatim from tomalla):
 *   CObject
 *     -> UnknownCGruntzMgrHogwarts (0x8)
 *         -> UnknownCGruntzMgrLucius (0x10)
 *             |- UnknownDraco (0x1c) | UnknownHermiona (0x6c) | UnknownHagrid (0x2c)
 *             |- UnknownSeverus (0x2c, static DDSURFACEDESC) | UnknownSirius (0x2c)
 *             |- UnknownAlbus (0x68) | UnknownRemus (0x6d4, resolution ladder)
 *             '- UnknownMinerva (0x38) | UnknownPettigrew (0x2c)
 *   Standalone:
 *     UnknownFilch (0x948)
 *     UnknownSalazar (0x94) -> UnknownVoldemort (0x9c, attenuation table)
 *   The manager UnknownClassCGruntzMgrHarryPotter (CObject; 0x40) owns one of each
 *   subclass via 11 pointer fields @4..2c + hWnd @30 + flags @34.
 * ================================================================================
 */

typedef void* HWND; // Win32 handle (4-byte pointer); avoid pulling <afxwin.h>.

// ---------------------------------------------------------------------------
// Minimal placeholder MFC base/collection types. Only their SIZES are
// load-bearing here (they appear as embedded members whose size sets the next
// field's offset). Sizes are pinned by the offset gaps in this same family:
//   CObject 0x4 (vptr), CObList/CMapPtrToPtr/CMapStringToOb/CMapStringToPtr/
//   CPtrList 0x1c, CObArray/CPtrArray 0x14. (Match the real MSVC 5.0 MFC sizes.)
// ---------------------------------------------------------------------------
class CObject {
public:
    virtual ~CObject() {}
}; // 0x04 (vptr only)
struct CObList {
    void* _v;
    char _raw[0x1c - 4];
}; // 0x1c
struct CObArray {
    void* _v;
    char _raw[0x14 - 4];
}; // 0x14
struct CPtrList {
    void* _v;
    char _raw[0x1c - 4];
}; // 0x1c
struct CPtrArray {
    void* _v;
    char _raw[0x14 - 4];
}; // 0x14
struct CMapPtrToPtr {
    void* _v;
    char _raw[0x1c - 4];
}; // 0x1c
struct CMapStringToOb {
    void* _v;
    char _raw[0x1c - 4];
}; // 0x1c
struct CMapStringToPtr {
    void* _v;
    char _raw[0x1c - 4];
}; // 0x1c

class UnknownClassCGruntzMgrHarryPotter; // the family manager (declared below)

/* Common base (0x8): CObject vptr @0 + one int field @4. */
class UnknownCGruntzMgrHogwarts : public CObject {
protected:
    UnknownCGruntzMgrHogwarts() {}
    UnknownCGruntzMgrHogwarts(i32 unknown) {
        m_fieldBaseUnknown = unknown;
    }
    virtual ~UnknownCGruntzMgrHogwarts() OVERRIDE {}

    i32 m_fieldBaseUnknown; // +0x04
};

/* The shared polymorphic base for the 10 surface/page sub-managers (0x10). */
class UnknownCGruntzMgrLucius : public UnknownCGruntzMgrHogwarts {
public:
    UnknownCGruntzMgrLucius(
        UnknownClassCGruntzMgrHarryPotter* pHarryPotter,
        i32 unknown2,
        i32 unknown3
    );
    virtual ~UnknownCGruntzMgrLucius() OVERRIDE;
    virtual void VirtualMethodUnknown14();
    virtual bool VirtualMethodUnknown18();
    virtual void VirtualMethodUnknown1C();
    virtual void VirtualMethodUnknown20();

    i32 fieldUnknown8;                                        // +0x08
    UnknownClassCGruntzMgrHarryPotter* m_pUnknownHarryPotter; // +0x0c
};

class UnknownDraco : public UnknownCGruntzMgrLucius {
public:
    UnknownDraco(UnknownClassCGruntzMgrHarryPotter* p, i32 u2, i32 u3);
    virtual ~UnknownDraco() OVERRIDE;
    virtual void VirtualMethodUnknown24();
    i32 fieldUnknown10, fieldUnknown14, fieldUnknown18; // +0x10..+0x1b
}; // 0x1c

class UnknownHermiona : public UnknownCGruntzMgrLucius {
public:
    UnknownHermiona(UnknownClassCGruntzMgrHarryPotter* p, i32 u2, i32 u3);
    virtual ~UnknownHermiona() OVERRIDE;
    CObList m_unknownObList;            // +0x10
    CMapPtrToPtr m_unknownPtrMap1;      // +0x2c
    CMapPtrToPtr m_unknownPtrMap2;      // +0x48
    i32 fieldUnknown64, fieldUnknown68; // +0x64..+0x6b
}; // 0x6c

class UnknownHagrid : public UnknownCGruntzMgrLucius {
public:
    UnknownHagrid(UnknownClassCGruntzMgrHarryPotter* p, i32 u2, i32 u3);
    virtual ~UnknownHagrid() OVERRIDE;
    CObList m_unknownObList; // +0x10
}; // 0x2c

/* Holds the static DDSURFACEDESC-shaped struct — strongest DDraw evidence. */
struct UnknownDirectDrawStructure {
    u32 dwSize;               // +0x00 (DDSURFACEDESC is 0x6c — re-verify)
    char _pad04[0x64 - 0x04]; // +0x04
}; // 0x64

class UnknownSeverus : public UnknownCGruntzMgrLucius {
public:
    UnknownSeverus(UnknownClassCGruntzMgrHarryPotter* p, i32 u2, i32 u3);
    virtual ~UnknownSeverus() OVERRIDE;
    CMapStringToOb m_unknownMap; // +0x10  (widest sub-manager vtable, 18 slots)

    //@address: 006c0270  (static data anchor, @approx tomalla 1.0.1.77)
    static UnknownDirectDrawStructure staticUnknownDirectDrawStructure;
    //@address: 006c02d4
    static i32 staticUnknown1;
    //@address: 006c02d8
    static i32 staticUnknown2;
}; // 0x2c

class UnknownSirius : public UnknownCGruntzMgrLucius {
public:
    UnknownSirius(UnknownClassCGruntzMgrHarryPotter* p, i32 u2, i32 u3);
    virtual ~UnknownSirius() OVERRIDE;
    CMapStringToOb m_unknownMap; // +0x10
}; // 0x2c

class UnknownAlbus : public UnknownCGruntzMgrLucius {
public:
    UnknownAlbus(UnknownClassCGruntzMgrHarryPotter* p, i32 u2, i32 u3);
    virtual ~UnknownAlbus() OVERRIDE;
    CMapStringToOb m_unknownMap1; // +0x10
    CMapStringToOb m_unknownMap2; // +0x2c
    CMapStringToOb m_unknownMap3; // +0x48
    i32 fieldUnknown64;           // +0x64
}; // 0x68

/* Seeds the resolution/scaling ladder in its ctor. */
class UnknownRemus : public UnknownCGruntzMgrLucius {
public:
    UnknownRemus(UnknownClassCGruntzMgrHarryPotter* p, i32 u2, i32 u3);
    virtual ~UnknownRemus() OVERRIDE;
    i32 fieldUnknown10;                                                 // +0x10
    char _pad14[0x20 - 0x14];                                           // +0x14..+0x1f
    CObArray m_unknownObArray1;                                         // +0x20
    CObArray m_unknownObArray2;                                         // +0x34
    CObArray m_unknownObArray3;                                         // +0x48
    i32 fieldUnknown5C, fieldUnknown60, fieldUnknown64, fieldUnknown68; // +0x5c..+0x6b
    char _pad6C[0xac - 0x6c]; // +0x6c..+0xab (unknown int block / padding)
    i32 fieldUnknownAC, fieldUnknownB0, fieldUnknownB4, fieldUnknownB8; // +0xac..
    i32 fieldUnknownBC, fieldUnknownC0, fieldUnknownC4, fieldUnknownC8;
    i32 fieldUnknownCC, fieldUnknownD0, fieldUnknownD4, fieldUnknownD8, fieldUnknownDC;
    char _unknownBuffer[0x6d4 - 0xe0]; // +0xe0  (~1.5KB scratch/scaling buffer)
}; // 0x6d4

class UnknownMinerva : public UnknownCGruntzMgrLucius {
public:
    UnknownMinerva(UnknownClassCGruntzMgrHarryPotter* p, i32 u2, i32 u3);
    virtual ~UnknownMinerva() OVERRIDE;
    void ClearUnknownMap();
    CMapStringToPtr m_unknownMap; // +0x10
    i32 fieldUnknown2C;           // +0x2c
    char _pad30[0x34 - 0x30];     // +0x30
    i32 fieldUnknown34;           // +0x34
}; // 0x38

class UnknownPettigrew : public UnknownCGruntzMgrLucius {
public:
    UnknownPettigrew(UnknownClassCGruntzMgrHarryPotter* p, i32 u2, i32 u3);
    virtual ~UnknownPettigrew() OVERRIDE;
    CMapStringToPtr m_unknownMap; // +0x10
}; // 0x2c

/* Standalone (NOT a Lucius subclass; no vtable). */
class UnknownFilch {
public:
    UnknownFilch();
    i32 fieldUnknown000, fieldUnknown004;                  // +0x00, +0x04
    char _pad008[0x47c - 0x08];                            // +0x08
    CPtrList m_unknownPtrList1;                            // +0x47c
    CPtrList m_unknownPtrList2;                            // +0x498
    CPtrArray m_unknownPtrArray;                           // +0x4b4
    char _pad4C8[0x534 - 0x4c8];                           // +0x4c8
    i32 fieldUnknown534, fieldUnknown538;                  // +0x534, +0x538
    char _pad53C[0x93c - 0x53c];                           // +0x53c
    i32 fieldUnknown93C, fieldUnknown940, fieldUnknown944; // +0x93c..+0x947
}; // 0x948

/* Holds the 101-entry volume->attenuation lookup table (see ../enums.h). */
class UnknownSalazar {
public:
    UnknownSalazar();
    virtual ~UnknownSalazar();                                          // vtbl @0x00
    i32 fieldUnknown04, fieldUnknown08, fieldUnknown0C, fieldUnknown10; // +0x04..+0x13
    char _pad14[0x78 - 0x14];                                           // +0x14
    i32 fieldUnknown78;                                                 // +0x78
    char _pad7C[0x80 - 0x7c];                                           // +0x7c
    i32 fieldUnknown80, fieldUnknown84, fieldUnknown88, fieldUnknown8C,
        fieldUnknown90; // +0x80..+0x93

    // table[i] = -1000 * log2(100/i); table[0] = -10000, table[100] = 0.
    // See ../enums.h GruntzVolumeAttenuation for the full 101-value spec.
    static i32 unknownLookupTable[101];
    static void initializeUnknownLookupTable();
    static i32 getLookupTableValue(i32 index);
}; // 0x94

class UnknownVoldemort : public UnknownSalazar {
public:
    UnknownVoldemort();
    virtual ~UnknownVoldemort() OVERRIDE;
    i32 fieldUnknown94, fieldUnknown98; // +0x94, +0x98
}; // 0x9c

/*
 * The family manager — HYPOTHESIZED CDirectDrawMgr. Stored in CGruntzMgr @0x30.
 * Owns one of each sub-manager via 11 pointer slots @4..2c, plus hWnd + flags.
 */
class UnknownClassCGruntzMgrHarryPotter : public CObject {
public:
    UnknownClassCGruntzMgrHarryPotter();
    virtual ~UnknownClassCGruntzMgrHarryPotter() OVERRIDE;
    virtual void UnknownVirtualMethod14();
    // (hWnd, 640, 480, 16, flags) — the display-mode init call.
    virtual bool
    UnknownVirtualMethod18(HWND hWnd, i32 width, i32 height, i32 bpp, i32 flagsUnknown);
    virtual void UnknownVirtualMethod1C();

    // One owned sub-manager per slot (names = tomalla placeholders).
    UnknownCGruntzMgrLucius* fieldUnknownDraco;     // +0x04
    UnknownCGruntzMgrLucius* fieldUnknownHermiona;  // +0x08
    UnknownCGruntzMgrLucius* fieldUnknownHagrid;    // +0x0c
    UnknownCGruntzMgrLucius* fieldUnknownSeverus;   // +0x10
    UnknownCGruntzMgrLucius* fieldUnknownSirius;    // +0x14
    UnknownCGruntzMgrLucius* fieldUnknownAlbus;     // +0x18
    UnknownFilch* fieldUnknownFilch;                // +0x1c
    UnknownVoldemort* fieldUnknownVoldemort;        // +0x20
    UnknownCGruntzMgrLucius* fieldUnknownRemus;     // +0x24
    UnknownMinerva* fieldUnknownMinerva;            // +0x28
    UnknownCGruntzMgrLucius* fieldUnknownPettigrew; // +0x2c
    HWND m_hWnd;                                    // +0x30
    i32 m_flagsUnknown;                             // +0x34
    i32 fieldUnknown38, fieldUnknown3C;             // +0x38, +0x3c

    //@address: 006c0314  (static data anchors, @approx tomalla 1.0.1.77)
    static i32 staticUnknown1;
    //@address: 006c0318
    static i32 staticUnknown2;
}; // 0x40

#endif /* MANAGERS_DDRAWMGR_SURFACE_FAMILY_H */
