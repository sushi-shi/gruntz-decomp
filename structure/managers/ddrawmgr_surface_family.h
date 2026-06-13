#ifndef MANAGERS_DDRAWMGR_SURFACE_FAMILY_H
#define MANAGERS_DDRAWMGR_SURFACE_FAMILY_H

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
 *   C:\Proj\DDrawMgr\{DDRAWMGR,DIRPAL,DIRSURF}.CPP (see ../managers/cdirectdrawmgr.h).
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
 *       lookup table (see ../enums.h GruntzVolumeAttenuation).
 *
 * @approx tomalla 1.0.1.77 — all OFFSETS / SIZES / vtable-SLOTS / INHERITANCE below
 * are version-independent (high confidence). The ~120 method-body addresses tomalla
 * recorded (mostly `throw NotImplemented` stubs) are 1.0.1.77 and are DEFERRED to
 * the re-anchor — NOT recorded here. Static-DATA @address values (006c02xx/006c03xx)
 * are 1.0.1.77 data anchors, carried as approximate.
 *
 * Inheritance spine (verbatim from tomalla):
 *   CObject
 *     -> UnknownCGruntzMgrHogwarts (size 8)
 *         -> UnknownCGruntzMgrLucius (size 0x10)
 *             |- UnknownDraco     (0x1c)
 *             |- UnknownHermiona  (0x6c)
 *             |- UnknownHagrid    (0x2c)
 *             |- UnknownSeverus   (0x2c)   <- holds the static DDSURFACEDESC struct
 *             |- UnknownSirius    (0x2c)
 *             |- UnknownAlbus     (0x68)
 *             |- UnknownRemus     (0x6d4)  <- resolution ladder
 *             |- UnknownMinerva   (0x38)
 *             '- UnknownPettigrew (0x2c)
 *   Standalone:
 *     UnknownFilch    (0x948)
 *     UnknownSalazar  (0x94)  -> UnknownVoldemort (0x9c)   <- attenuation table
 *   The manager UnknownClassCGruntzMgrHarryPotter (CObject; 0x40) owns one of each
 *   subclass via 11 pointer fields @4..2c + hWnd @30 + flags @34.
 * ================================================================================
 */

#undef UNICODE
#undef _UNICODE
#include <afxwin.h>

class UnknownClassCGruntzMgrHarryPotter;  // the family manager (declared below)

/* Common base @8 (one int field). */
class UnknownCGruntzMgrHogwarts : public CObject
{
protected:
    //@size: 8   @approx tomalla 1.0.1.77
    UnknownCGruntzMgrHogwarts() {}
    UnknownCGruntzMgrHogwarts(int unknown) { m_fieldBaseUnknown = unknown; }
    virtual ~UnknownCGruntzMgrHogwarts() {}

    //@offset: 4
    int m_fieldBaseUnknown;
};

/* The shared polymorphic base for the 10 surface/page sub-managers. */
class UnknownCGruntzMgrLucius : public UnknownCGruntzMgrHogwarts
{
public:
    //@size: 0x10   @approx tomalla 1.0.1.77
    UnknownCGruntzMgrLucius(UnknownClassCGruntzMgrHarryPotter* pHarryPotter,
                            int unknown2, int unknown3);

    //@vftable: 4
    virtual ~UnknownCGruntzMgrLucius();
    //@vftable: 14
    virtual void VirtualMethodUnknown14();
    //@vftable: 18
    virtual bool VirtualMethodUnknown18();
    //@vftable: 1c
    virtual void VirtualMethodUnknown1C();
    //@vftable: 20
    virtual void VirtualMethodUnknown20();

    //@offset: 8
    int fieldUnknown8;
    //@offset: c
    UnknownClassCGruntzMgrHarryPotter* m_pUnknownHarryPotter;
};

class UnknownDraco : public UnknownCGruntzMgrLucius
{
public:
    //@size: 0x1c   @approx tomalla 1.0.1.77
    UnknownDraco(UnknownClassCGruntzMgrHarryPotter* p, int u2, int u3);
    virtual ~UnknownDraco();
    //@vftable: 24
    virtual void VirtualMethodUnknown24();
    //@offset: 10
    int fieldUnknown10, fieldUnknown14, fieldUnknown18;
};

class UnknownHermiona : public UnknownCGruntzMgrLucius
{
public:
    //@size: 0x6c   @approx tomalla 1.0.1.77
    UnknownHermiona(UnknownClassCGruntzMgrHarryPotter* p, int u2, int u3);
    virtual ~UnknownHermiona();
    //@vftable: 24..40  (24,28,2c,30,34,38,3c,40)  @todo bodies
    //@offset: 10
    CObList m_unknownObList;
    //@offset: 2c
    CMapPtrToPtr m_unknownPtrMap1;
    //@offset: 48
    CMapPtrToPtr m_unknownPtrMap2;
    //@offset: 64
    int fieldUnknown64, fieldUnknown68;
};

class UnknownHagrid : public UnknownCGruntzMgrLucius
{
public:
    //@size: 0x2c   @approx tomalla 1.0.1.77
    UnknownHagrid(UnknownClassCGruntzMgrHarryPotter* p, int u2, int u3);
    virtual ~UnknownHagrid();
    //@vftable: 24..34  @todo bodies
    //@offset: 10
    CObList m_unknownObList;
};

/* Holds the static DDSURFACEDESC-shaped struct — strongest DDraw evidence. */
//@todo: replace with the real DDSURFACEDESC from <ddraw.h> once confirmed
struct UnknownDirectDrawStructure
{
    //@size: 0x64   @approx tomalla 1.0.1.77 (DDSURFACEDESC is 0x6c — re-verify)
    //@offset: 0
    DWORD dwSize;
    //@offset: 4
    char _padding04[96];
};

class UnknownSeverus : public UnknownCGruntzMgrLucius
{
public:
    //@size: 0x2c   @approx tomalla 1.0.1.77
    UnknownSeverus(UnknownClassCGruntzMgrHarryPotter* p, int u2, int u3);
    virtual ~UnknownSeverus();
    //@vftable: 24..58  (the widest sub-manager vtable — 18 slots)  @todo bodies
    //@offset: 10
    CMapStringToOb m_unknownMap;

    //@address: 006c0270   @approx tomalla 1.0.1.77 (static data anchor)
    static UnknownDirectDrawStructure staticUnknownDirectDrawStructure;
    //@address: 006c02d4
    static int staticUnknown1;
    //@address: 006c02d8
    static int staticUnknown2;
};

class UnknownSirius : public UnknownCGruntzMgrLucius
{
public:
    //@size: 0x2c   @approx tomalla 1.0.1.77
    UnknownSirius(UnknownClassCGruntzMgrHarryPotter* p, int u2, int u3);
    virtual ~UnknownSirius();
    //@vftable: 24  @todo body
    //@offset: 10
    CMapStringToOb m_unknownMap;
};

class UnknownAlbus : public UnknownCGruntzMgrLucius
{
public:
    //@size: 0x68   @approx tomalla 1.0.1.77
    UnknownAlbus(UnknownClassCGruntzMgrHarryPotter* p, int u2, int u3);
    virtual ~UnknownAlbus();
    //@vftable: 24,28,2c,30  @todo bodies
    //@offset: 10
    CMapStringToOb m_unknownMap1;
    //@offset: 2c
    CMapStringToOb m_unknownMap2;
    //@offset: 48
    CMapStringToOb m_unknownMap3;
    //@offset: 64
    int fieldUnknown64;
};

/* Seeds the resolution/scaling ladder in its ctor (500/250, 1000, 1600x1200,
 * 2560x1920, 768x576, base 64x64 — see ../enums.h GruntzResolutionLadder). */
class UnknownRemus : public UnknownCGruntzMgrLucius
{
public:
    //@size: 0x6d4   @approx tomalla 1.0.1.77
    UnknownRemus(UnknownClassCGruntzMgrHarryPotter* p, int u2, int u3);
    virtual ~UnknownRemus();
    //@vftable: 14,1c,20,24,28,2c,30,34,38,3c,40,44  @todo bodies
    //@offset: 10
    int fieldUnknown10;
    //@offset: 14   (0x14..0x1c padding/unknown ints)
    char _padding14[0x0C];
    //@offset: 20
    CObArray m_unknownObArray1;
    //@offset: 34
    CObArray m_unknownObArray2;
    //@offset: 48
    CObArray m_unknownObArray3;
    //@offset: 5c
    int fieldUnknown5C, fieldUnknown60, fieldUnknown64, fieldUnknown68;
    //@offset: 6c  (0x6c..0xac: block of unknown ints/padding)
    char _padding6C[0x40];
    //@offset: ac  (0xac..0xdc: int block — resolution-ladder constants land here)
    int fieldUnknownAC, fieldUnknownB0, fieldUnknownB4, fieldUnknownB8;
    int fieldUnknownBC, fieldUnknownC0, fieldUnknownC4, fieldUnknownC8;
    int fieldUnknownCC, fieldUnknownD0, fieldUnknownD4, fieldUnknownD8, fieldUnknownDC;
    //@offset: e0
    char _unknownBuffer[1524];   // ~1.5KB scratch/scaling buffer
    // (total 0x6d4)
};

class UnknownMinerva : public UnknownCGruntzMgrLucius
{
public:
    //@size: 0x38   @approx tomalla 1.0.1.77
    UnknownMinerva(UnknownClassCGruntzMgrHarryPotter* p, int u2, int u3);
    virtual ~UnknownMinerva();
    //@vftable: 14,1c  @todo bodies
    void ClearUnknownMap();
    //@offset: 10
    CMapStringToPtr m_unknownMap;
    //@offset: 2c
    int fieldUnknown2C;
    //@offset: 30
    char _padding30[4];
    //@offset: 34
    int fieldUnknown34;
};

class UnknownPettigrew : public UnknownCGruntzMgrLucius
{
public:
    //@size: 0x2c   @approx tomalla 1.0.1.77
    UnknownPettigrew(UnknownClassCGruntzMgrHarryPotter* p, int u2, int u3);
    virtual ~UnknownPettigrew();
    //@vftable: 14,18,1c  @todo bodies
    //@offset: 10
    CMapStringToPtr m_unknownMap;
};

/* Standalone (NOT a Lucius subclass). */
class UnknownFilch
{
public:
    //@size: 0x948   @approx tomalla 1.0.1.77
    UnknownFilch();
    //@offset: 0
    int fieldUnknown000, fieldUnknown004;
    //@offset: 8
    char _padding008[1140];
    //@offset: 47c
    CPtrList m_unknownPtrList1;
    //@offset: 498
    CPtrList m_unknownPtrList2;
    //@offset: 4b4
    CPtrArray m_unknownPtrArray;
    //@offset: 4c8
    char _padding4C8[108];
    //@offset: 534
    int fieldUnknown534, fieldUnknown538;
    //@offset: 53c
    char _padding53C[1024];
    //@offset: 93c
    int fieldUnknown93C, fieldUnknown940, fieldUnknown944;
};

/* Holds the 101-entry volume->attenuation lookup table (see ../enums.h). */
class UnknownSalazar
{
public:
    //@size: 0x94   @approx tomalla 1.0.1.77
    UnknownSalazar();
    //@vftable: 0
    virtual ~UnknownSalazar();
    //@offset: 4
    int fieldUnknown04, fieldUnknown08, fieldUnknown0C, fieldUnknown10;
    //@offset: 14
    char _padding14[100];
    //@offset: 78
    int fieldUnknown78;
    //@offset: 7c
    char _padding7C[4];
    //@offset: 80
    int fieldUnknown80, fieldUnknown84, fieldUnknown88, fieldUnknown8C, fieldUnknown90;

    // table[i] = -1000 * log2(100/i); table[0] = -10000, table[100] = 0.
    // See ../enums.h GruntzVolumeAttenuation for the full 101-value spec.
    static int unknownLookupTable[101];
    static void initializeUnknownLookupTable();
    static int getLookupTableValue(int index);
};

class UnknownVoldemort : public UnknownSalazar
{
public:
    //@size: 0x9c   @approx tomalla 1.0.1.77
    UnknownVoldemort();
    virtual ~UnknownVoldemort();
    //@offset: 94
    int fieldUnknown94, fieldUnknown98;
};

/*
 * The family manager — HYPOTHESIZED CDirectDrawMgr. Stored in CGruntzMgr @0x30.
 * Owns one of each sub-manager via 11 pointer slots @4..2c, plus hWnd + flags.
 */
class UnknownClassCGruntzMgrHarryPotter : public CObject
{
public:
    //@size: 0x40   @approx tomalla 1.0.1.77
    UnknownClassCGruntzMgrHarryPotter();
    virtual ~UnknownClassCGruntzMgrHarryPotter();

    //@vftable: 14
    virtual void UnknownVirtualMethod14();
    //@vftable: 18
    // (hWnd, 640, 480, 16, flags) — the display-mode init call.
    virtual bool UnknownVirtualMethod18(HWND hWnd, int width, int height,
                                        int bpp, int flagsUnknown);
    //@vftable: 1c
    virtual void UnknownVirtualMethod1C();

    //@offset: 4   one owned sub-manager per slot (names = tomalla placeholders)
    UnknownCGruntzMgrLucius* fieldUnknownDraco;
    //@offset: 8
    UnknownCGruntzMgrLucius* fieldUnknownHermiona;
    //@offset: c
    UnknownCGruntzMgrLucius* fieldUnknownHagrid;
    //@offset: 10
    UnknownCGruntzMgrLucius* fieldUnknownSeverus;
    //@offset: 14
    UnknownCGruntzMgrLucius* fieldUnknownSirius;
    //@offset: 18
    UnknownCGruntzMgrLucius* fieldUnknownAlbus;
    //@offset: 1c
    UnknownFilch* fieldUnknownFilch;
    //@offset: 20
    UnknownVoldemort* fieldUnknownVoldemort;
    //@offset: 24
    UnknownCGruntzMgrLucius* fieldUnknownRemus;
    //@offset: 28
    UnknownMinerva* fieldUnknownMinerva;
    //@offset: 2c
    UnknownCGruntzMgrLucius* fieldUnknownPettigrew;
    //@offset: 30
    HWND m_hWnd;
    //@offset: 34
    int m_flagsUnknown;
    //@offset: 38
    int fieldUnknown38, fieldUnknown3C;

    //@address: 006c0314   @approx tomalla 1.0.1.77 (static data anchors)
    static int staticUnknown1;
    //@address: 006c0318
    static int staticUnknown2;
};

#endif /* MANAGERS_DDRAWMGR_SURFACE_FAMILY_H */
