#include "../rva.h"
// HarryPotter.cpp - root object of the tomalla-named DDraw surface/page-manager
// family. UnknownClassCGruntzMgrHarryPotter is the owner stored off CGruntzMgr
// +0x30; it holds one child manager pointer per slot and a pair of global draw
// clock mirrors reset by the ctor.
//
// Names are tomalla placeholders. Offsets, store order, vtable slots, and global
// addresses are load-bearing for matching.

typedef void *HWND;

class UnknownCGruntzMgrLucius {
public:
    virtual void Slot00();
    virtual void Slot04();
    virtual void Slot08();
    virtual void Slot0C();
    virtual void Slot10();
    virtual int  Vfunc14();
};

class UnknownClassCGruntzMgrHarryPotter {
public:
    UnknownClassCGruntzMgrHarryPotter();
    virtual ~UnknownClassCGruntzMgrHarryPotter();
    virtual int UnknownVirtualMethod14();
    virtual int UnknownVirtualMethod18(HWND hWnd, int width, int height,
                                       int bpp, int flagsUnknown);
    virtual void UnknownVirtualMethod1C();

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

DATA(0x2bf3c0)
extern "C" unsigned int g_6bf3c0;       // VA 0x6bf3c0  draw-clock mirror
DATA(0x2bf3bc)
extern "C" unsigned int g_6bf3bc;       // VA 0x6bf3bc  draw-delta mirror

// ---------------------------------------------------------------------------
// UnknownClassCGruntzMgrHarryPotter::UnknownClassCGruntzMgrHarryPotter()
// Stamps the vftable, clears every owned-child pointer except hwnd (+0x30), clears
// flags/bookkeeping at +0x34/+0x38/+0x3c, then resets the two draw-clock globals.
RVA(0x155840, 0x41)
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
// UnknownClassCGruntzMgrHarryPotter::UnknownVirtualMethod14()
// Returns whether the core child managers are present and the first child accepts
// its +0x14 virtual readiness check.
RVA(0x155f00, 0x41)
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

// Out-of-line stubs so the vftable is emitted in this TU. They are not claimed
// as matched in symbol_names.csv.
UnknownClassCGruntzMgrHarryPotter::~UnknownClassCGruntzMgrHarryPotter() {}
int UnknownClassCGruntzMgrHarryPotter::UnknownVirtualMethod18(
    HWND, int, int, int, int) { return 0; }
void UnknownClassCGruntzMgrHarryPotter::UnknownVirtualMethod1C() {}
