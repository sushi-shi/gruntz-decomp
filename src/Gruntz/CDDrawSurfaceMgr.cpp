#include <rva.h>
// HarryPotter.cpp - root object of the tomalla-named DDraw surface/page-manager
// family. CDDrawSurfaceMgr is the owner stored off CGruntzMgr
// +0x30; it holds one child manager pointer per slot and a pair of global draw
// clock mirrors reset by the ctor.
//
// Names are tomalla placeholders. Offsets, store order, vtable slots, and global
// addresses are load-bearing for matching.

typedef void *HWND;
typedef long intptr_t;

class UnknownCGruntzMgrLuciusChild {
public:
    int m_10;
    int m_14;
};

class CDDrawSubMgr {
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

class CDDrawSurfaceMgr {
public:
    CDDrawSurfaceMgr();
    virtual ~CDDrawSurfaceMgr();
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

    // Engine-label backlog stubs.
    void UnknownVirtualMethod18();

    CDDrawSubMgr *m_04;      // +0x04  Draco
    CDDrawSubMgr *m_08;      // +0x08  Hermiona
    CDDrawSubMgr *m_0c;      // +0x0c  Hagrid
    CDDrawSubMgr *m_10;      // +0x10  Severus
    CDDrawSubMgr *m_14;      // +0x14  Sirius
    CDDrawSubMgr *m_18;      // +0x18  Albus
    void                    *m_1c;      // +0x1c  Filch
    void                    *m_20;      // +0x20  Voldemort
    CDDrawSubMgr *m_24;      // +0x24  Remus
    void                    *m_28;      // +0x28  Minerva
    void                    *m_2c;      // +0x2c  Pettigrew
    HWND                     m_hWnd;    // +0x30
    int                      m_flags;   // +0x34
    int                      m_38;      // +0x38
    int                      m_3c;      // +0x3c
};

DATA(0x2bf3c0)
extern "C" unsigned int g_6bf3c0;       // draw-clock mirror
DATA(0x2bf3bc)
extern "C" unsigned int g_6bf3bc;       // draw-delta mirror

// ---------------------------------------------------------------------------
// CDDrawSurfaceMgr::CDDrawSurfaceMgr()
// Stamps the vftable, clears every owned-child pointer except hwnd (+0x30), clears
// flags/bookkeeping at +0x34/+0x38/+0x3c, then resets the two draw-clock globals.
RVA(0x155840, 0x41)
CDDrawSurfaceMgr::CDDrawSurfaceMgr()
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
// CDDrawSurfaceMgr::UnknownVirtualMethod14()
// Returns whether the core child managers are present and the first child accepts
// its +0x14 virtual readiness check.
RVA(0x155f00, 0x41)
int CDDrawSurfaceMgr::UnknownVirtualMethod14()
{
    CDDrawSubMgr *first = m_04;

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

// Helper structs for __thiscall external functions.
struct MinervaInner { void Free(); };
struct VoldemortObj { void Free(); };
struct MinervaMgr   { void ClearMap(); };
extern void __cdecl RelayHwnd(void *hWnd);
extern int __stdcall CreateChildSurface(int x, int y, int flags);
struct RemusCoordsHelper { int SetCoords(int x, int y); };
typedef int (__cdecl *HP_Callback)(void *, void *, int, int, int);

// ---------------------------------------------------------------------------
// CDDrawSurfaceMgr::UnknownVirtualMethod20()
// Frees context — cleans up the Voldemort surface and the Minerva map.
RVA(0x155fc0, 0x2e)
void CDDrawSurfaceMgr::UnknownVirtualMethod20()
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
// CDDrawSurfaceMgr::UnknownVirtualMethod24()
// Validates/sets surface dimensions.
RVA(0x155f60, 0x56)
int CDDrawSurfaceMgr::UnknownVirtualMethod24(
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
// CDDrawSurfaceMgr::UnknownVirtualMethod28()
// Relays the hWnd argument to an external manager function.
RVA(0x155f50, 0x10)
void CDDrawSurfaceMgr::UnknownVirtualMethod28(void *hWnd)
{
    RelayHwnd(hWnd);
}

// ---------------------------------------------------------------------------
// CDDrawSurfaceMgr::UnknownVirtualMethod38()
// Dispatches arguments through the m_3c callback function pointer,
// returning 1 on success / 0 on failure.
RVA(0x156a90, 0x3a)
int CDDrawSurfaceMgr::UnknownVirtualMethod38(
    void *arg1, int arg2, int arg3, int arg4)
{
    if (!arg1)
        return 0;
    if (!m_3c)
        return 0;
    return ((HP_Callback)(intptr_t)m_3c)(this, arg1, arg2, arg3, arg4) != 0;
}

// Out-of-line stubs so the vftable is emitted in this TU. They are not claimed
// as matched in symbol_names.csv.
CDDrawSurfaceMgr::~CDDrawSurfaceMgr() {}
int CDDrawSurfaceMgr::UnknownVirtualMethod18(
    HWND, int, int, int, int) { return 0; }
void CDDrawSurfaceMgr::UnknownVirtualMethod1C() {}
int CDDrawSurfaceMgr::UnknownVirtualMethod2C(int) { return 0; }
int CDDrawSurfaceMgr::UnknownVirtualMethod30(
    int, int, int, int, void *) { return 0; }
int CDDrawSurfaceMgr::UnknownVirtualMethod34(
    int, int, int, int, void *) { return 0; }

// Engine-label backlog stubs (moved from src/Stub/CDDrawSurfaceMgr.cpp).

// @confidence: high
// @source: tomalla
// @stub
RVA(0x155900, 0x519)
void CDDrawSurfaceMgr::UnknownVirtualMethod18() {}
