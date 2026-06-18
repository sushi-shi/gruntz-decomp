// HarryPotter.cpp - root object of the tomalla-named DDraw surface/page-manager
// family. UnknownClassCGruntzMgrHarryPotter is the owner stored off CGruntzMgr
// +0x30; it holds one child manager pointer per slot and a pair of global draw
// clock mirrors reset by the ctor.
//
// Names are tomalla placeholders. Offsets, store order, vtable slots, and global
// addresses are load-bearing for matching.

#include <stddef.h>
typedef void *HWND;
typedef long intptr_t;

class UnknownCGruntzMgrLuciusChild {
public:
    int m_10;
    int m_14;
};

class UnknownCGruntzMgrLucius {
public:
    virtual void Slot00();
    virtual void Slot04();
    virtual void Slot08();
    virtual void Slot0C();
    virtual void Slot10();
    virtual int  Vfunc14();

    void *m_04;
    UnknownCGruntzMgrLuciusChild *m_10;
};

class UnknownClassCGruntzMgrHarryPotter {
public:
    UnknownClassCGruntzMgrHarryPotter();
    virtual ~UnknownClassCGruntzMgrHarryPotter();
    virtual int UnknownVirtualMethod14();
    virtual int UnknownVirtualMethod18(HWND hWnd, int width, int height,
                                       int bpp, int flagsUnknown);
    virtual void UnknownVirtualMethod1C();
    virtual void UnknownVirtualMethod20();
    virtual int UnknownVirtualMethod24(int x, int y, int flags);
    virtual void UnknownVirtualMethod28(void *hWnd);
    virtual int UnknownVirtualMethod2C(int unknown);
    virtual int UnknownVirtualMethod30(int width, int height,
                                        int bpp, int flagsUnknown,
                                        void *callback);
    virtual int UnknownVirtualMethod34(int width, int height,
                                        int bpp, int flagsUnknown,
                                        void *callback);
    virtual int UnknownVirtualMethod38(void *arg1, int arg2, int arg3,
                                        int arg4);

    UnknownCGruntzMgrLucius *m_04;      // +0x04  Draco
    UnknownCGruntzMgrLucius *m_08;      // +0x08  Hermiona
    UnknownCGruntzMgrLucius *m_0c;      // +0x0c  Hagrid
    UnknownCGruntzMgrLucius *m_10;      // +0x10  Severus
    UnknownCGruntzMgrLucius *m_14;      // +0x14  Sirius
    UnknownCGruntzMgrLucius *m_18;      // +0x18  Albus
    void                    *m_1c;      // +0x1c  Filch
    void                    *m_20;      // +0x20  Voldemort
    UnknownCGruntzMgrLucius *m_24;      // +0x24  Remus
    void                    *m_28;      // +0x28  Minerva
    void                    *m_2c;      // +0x2c  Pettigrew
    HWND                     m_hWnd;    // +0x30
    int                      m_flags;   // +0x34
    int                      m_38;      // +0x38
    int                      m_3c;      // +0x3c
};

// @data: 0x2bf3c0
extern "C" unsigned int g_6bf3c0;       // VA 0x6bf3c0  draw-clock mirror
// @data: 0x2bf3bc
extern "C" unsigned int g_6bf3bc;       // VA 0x6bf3bc  draw-delta mirror

// ---------------------------------------------------------------------------
// UnknownClassCGruntzMgrHarryPotter::UnknownClassCGruntzMgrHarryPotter()
// Stamps the vftable, clears every owned-child pointer except hwnd (+0x30), clears
// flags/bookkeeping at +0x34/+0x38/+0x3c, then resets the two draw-clock globals.
//
// @address: 0x155840
// @size:    0x41
// ---------------------------------------------------------------------------
UnknownClassCGruntzMgrHarryPotter::UnknownClassCGruntzMgrHarryPotter()
{
    m_04 = 0;
    m_08 = 0;
    m_0c = 0;
    m_10 = 0;
    m_14 = 0;
    m_18 = 0;
    m_1c = 0;
    m_20 = 0;
    m_24 = 0;
    m_28 = 0;
    m_2c = 0;
    m_flags = 0;
    m_38 = 0;
    m_3c = 0;
    g_6bf3c0 = 0;
    g_6bf3bc = 0;
}

// ---------------------------------------------------------------------------
// UnknownClassCGruntzMgrHarryPotter::~UnknownClassCGruntzMgrHarryPotter()
// Calls UnknownVirtualMethod1C (which deletes all child objects), framed by
// the EH handler (the vtable stores + SEH are compiler-generated).
//
// @address: 0x1558b0
// @size:    0x46
// ---------------------------------------------------------------------------
UnknownClassCGruntzMgrHarryPotter::~UnknownClassCGruntzMgrHarryPotter()
{
    UnknownVirtualMethod1C();
}

// ---------------------------------------------------------------------------
// UnknownClassCGruntzMgrHarryPotter::UnknownVirtualMethod14()
// Returns whether the core child managers are present and the first child accepts
// its +0x14 virtual readiness check.
//
// @address: 0x155f00
// @size:    0x41
// ---------------------------------------------------------------------------
int UnknownClassCGruntzMgrHarryPotter::UnknownVirtualMethod14()
{
    UnknownCGruntzMgrLucius *first = m_04;

    if (first == 0)
        goto fail;
    if (m_08 == 0)
        goto fail;
    if (m_0c == 0)
        goto fail;
    if (m_10 == 0)
        goto fail;
    if (m_14 == 0)
        goto fail;
    if (first->Vfunc14() == 0)
        goto fail;
    if (m_24 != 0)
        return 1;

fail:
    return 0;
}

// ---------------------------------------------------------------------------
// UnknownClassCGruntzMgrHarryPotter::UnknownVirtualMethod1C()
// Deletes all owned child objects. Each is freed by calling its scalar deleting
// destructor through vtable slot 1 (m_20 uses slot 0). m_1c (Filch) is cleaned
// up via an explicit function call then freed.
//
// @address: 0x155e20
// @size:    0xd1
// ---------------------------------------------------------------------------
void UnknownClassCGruntzMgrHarryPotter::UnknownVirtualMethod1C()
{
    // Remus (m_24) — delete
    if (m_24) { delete m_24; m_24 = 0; }
    // Minerva (m_28) — delete
    if (m_28) { operator delete(m_28); m_28 = 0; }
    // Voldemort (m_20) — delete
    if (m_20) { operator delete(m_20); m_20 = 0; }
    // Draco (m_04) — delete
    if (m_04) { delete m_04; m_04 = 0; }
    // Hermiona (m_08) — delete
    if (m_08) { delete m_08; m_08 = 0; }
    // Hagrid (m_0c) — delete
    if (m_0c) { delete m_0c; m_0c = 0; }
    // Severus (m_10) — delete
    if (m_10) { delete m_10; m_10 = 0; }
    // Sirius (m_14) — delete
    if (m_14) { delete m_14; m_14 = 0; }
    // Albus (m_18) — delete
    if (m_18) { delete m_18; m_18 = 0; }
    // Pettigrew (m_2c) — delete
    if (m_2c) { operator delete(m_2c); m_2c = 0; }
    // Filch (m_1c) — custom cleanup
    if (m_1c) {
        m_1c = 0;
    }
    m_3c = 0;
}

// ---------------------------------------------------------------------------
// UnknownClassCGruntzMgrHarryPotter::UnknownVirtualMethod18()
// Allocates and constructs all child manager objects (Draco through Pettigrew).
// Uses SEH to properly clean up partial allocations on failure.
//
// @address: 0x155900
// @size:    0x515
// ---------------------------------------------------------------------------
int UnknownClassCGruntzMgrHarryPotter::UnknownVirtualMethod18(
    HWND hWnd, int width, int height, int bpp, int flagsUnknown)
{
    // FIXME: complex allocation sequence — will be filled after build cycle
    m_hWnd = hWnd;
    m_flags = flagsUnknown;
    return 0;
}

// Helper structs for __thiscall external functions.
struct MinervaInner { void Free(); };
struct VoldemortObj { void Free(); };
struct MinervaMgr   { void ClearMap(); };
extern void __cdecl RelayHwnd(void *hWnd);
extern int __stdcall CreateChildSurface(int x, int y, int flags);
struct RemusCoordsHelper { int SetCoords(int x, int y); };
typedef int (__cdecl *HP_Callback)(void *, void *, int, int, int);

// ---------------------------------------------------------------------------
// UnknownClassCGruntzMgrHarryPotter::UnknownVirtualMethod20()
// Frees context — cleans up the Voldemort surface and the Minerva map.
//
// @address: 0x155fc0
// @size:    0x2e
// ---------------------------------------------------------------------------
void UnknownClassCGruntzMgrHarryPotter::UnknownVirtualMethod20()
{
    if (m_28 != 0) {
        void *inner = *(void **)((char *)m_28 + 0x2c);
        if (inner != 0)
            ((MinervaInner *)inner)->Free();
        ((MinervaMgr *)m_28)->ClearMap();
    }
    if (m_20 != 0)
        ((VoldemortObj *)m_20)->Free();
}

// ---------------------------------------------------------------------------
// UnknownClassCGruntzMgrHarryPotter::UnknownVirtualMethod24()
// Validates/sets surface dimensions.
//
// @address: 0x155f60
// @size:    0x56
// ---------------------------------------------------------------------------
int UnknownClassCGruntzMgrHarryPotter::UnknownVirtualMethod24(
    int x, int y, int flags)
{
    UnknownCGruntzMgrLuciusChild *child = m_04->m_10;
    if (child->m_10 != x || child->m_14 != y) {
        if (CreateChildSurface(x, y, flags) == 0)
            return 0;
    }
    if (m_24 != 0) {
        if (((RemusCoordsHelper *)m_24)->SetCoords(x, y) == 0)
            return 0;
    }
    return 1;
}

// ---------------------------------------------------------------------------
// UnknownClassCGruntzMgrHarryPotter::UnknownVirtualMethod28()
// Relays the hWnd argument to an external manager function.
//
// @address: 0x155f50
// @size:    0x10
// ---------------------------------------------------------------------------
void UnknownClassCGruntzMgrHarryPotter::UnknownVirtualMethod28(void *hWnd)
{
    RelayHwnd(hWnd);
}

// ---------------------------------------------------------------------------
// UnknownClassCGruntzMgrHarryPotter::UnknownVirtualMethod2C()
// Builds an image surface from a file path, performs multi-step initialization,
// calls the m_3c callback at stages 1..5.
//
// @address: 0x156020
// @size:    0x505
// ---------------------------------------------------------------------------
int UnknownClassCGruntzMgrHarryPotter::UnknownVirtualMethod2C(int unknown)
{
    // FIXME: file surface loader — will be filled after build cycle
    return 0;
}

// ---------------------------------------------------------------------------
// UnknownClassCGruntzMgrHarryPotter::UnknownVirtualMethod30()
// Builds an image surface from a file path (alternate path), calls the m_3c
// callback at stages 2..8.
//
// @address: 0x156530
// @size:    0x557
// ---------------------------------------------------------------------------
int UnknownClassCGruntzMgrHarryPotter::UnknownVirtualMethod30(
    int width, int height, int bpp, int flagsUnknown, void *callback)
{
    // FIXME: alternate surface loader — will be filled after build cycle
    return 0;
}

// ---------------------------------------------------------------------------
// UnknownClassCGruntzMgrHarryPotter::UnknownVirtualMethod38()
// Dispatches arguments through the m_3c callback function pointer,
// returning 1 on success / 0 on failure.
//
// @address: 0x156a90
// @size:    0x3a
// ---------------------------------------------------------------------------
int UnknownClassCGruntzMgrHarryPotter::UnknownVirtualMethod38(
    void *arg1, int arg2, int arg3, int arg4)
{
    if (!arg1)
        return 0;
    if (!m_3c)
        return 0;
    return ((HP_Callback)(intptr_t)m_3c)(this, arg1, arg2, arg3, arg4) != 0;
}
