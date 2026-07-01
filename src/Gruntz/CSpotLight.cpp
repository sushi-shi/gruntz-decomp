// CSpotLight.cpp - re-homed from src/Stub/Discovered.cpp (0x0b1ee0). The
// CSpotLight per-tick update: when the owner's mode (m_10->m_114) is 1 it rotates
// the (m_80,m_88) offset by the running angle m_90 (scaled by m_58 and the frame
// delta g_645584), folds in the tracked target (m_98->m_5c/m_60), and advances
// m_90; then it (re)resolves the light's bute key into m_14->m_1c when the
// manager's per-cell slot is empty. Field names placeholders; engine globals +
// CButeTree::Find are external (reloc-masked). flags=base (/O2 /Oi -> fsin/fcos).
#include <rva.h>
#include <Mfc.h>
#include <math.h>
#include <Bute/ButeMgr.h> // CButeTree::Find

extern "C" unsigned g_645584; // 0x645584 frame delta
extern CButeTree g_buteTree;  // 0x6bf620
extern char s_actKeyA[];      // 0x60a454 "A"

struct SpotM10 {
    char pad[0x114];
    int m_114; // 0x114
};
struct SpotM14 {
    char pad[0x1c];
    void* m_1c; // 0x1c
};
struct SpotM98 {
    char pad[0x5c];
    int m_5c; // 0x5c
    int m_60; // 0x60
};
struct MgrObj68 {
    char pad[0x1c];
    int arr[1]; // 0x1c
};
struct MgrReg2 {
    char pad[0x68];
    MgrObj68* m_68; // 0x68
};
extern "C" MgrReg2* g_mgrSettings; // 0x64556c

class CSpotLight {
public:
    char pad00[0x10];
    SpotM10* m_10; // 0x10
    SpotM14* m_14; // 0x14
    char pad18[0x30 - 0x18];
    void* m_30; // 0x30
    char pad34[0x58 - 0x34];
    double m_58;   // 0x58
    double m_60;   // 0x60
    double m_68;   // 0x68
    double m_70;   // 0x70
    double m_78;   // 0x78
    double m_80;   // 0x80
    double m_88;   // 0x88
    double m_90;   // 0x90
    SpotM98* m_98; // 0x98
    int m_9c;      // 0x9c
    int m_a0;      // 0xa0
    int Update_0b1ee0();
};

// @early-stop
// x87 fp-stack scheduling wall: the rotation's fld/fmul/fsub tree + the fxch
// interleave (and the frame-delta fild hoist) follow the x87-fp-stack-schedule
// pattern - prologue, control flow, the m_9c*15 cell lookup and the bute re-resolve
// match, but the FP block's fxch ordering is not source-steerable under MSVC5 /O2.
RVA(0x000b1ee0, 0x11d)
int CSpotLight::Update_0b1ee0() {
    if (m_10->m_114 == 1) {
        double c = cos(m_90);
        double s = sin(m_90);
        m_60 = -(m_88 * s + m_80 * c);
        m_68 = m_80 * s - m_88 * c;
        if (m_98) {
            m_70 = (double)m_98->m_5c;
            m_78 = (double)m_98->m_60;
        }
        m_60 = m_70 + m_60;
        m_68 = m_78 + m_68;
        m_90 = (double)g_645584 * m_58 + m_90;
    }
    if (g_mgrSettings->m_68->arr[m_a0 + m_9c * 15] == 0) {
        m_30 = m_14->m_1c;
        m_14->m_1c = g_buteTree.Find(s_actKeyA);
    }
    return 0;
}

// class-metadata SIZE sweep (misc-Gruntz A-C): matching-neutral, hosted at
// .cpp EOF (see docs/class-metadata-sweep-log.md). SIZE_UNKNOWN = size not yet pinned.
SIZE_UNKNOWN(MgrObj68);
SIZE_UNKNOWN(MgrReg2);
SIZE_UNKNOWN(SpotM10);
SIZE_UNKNOWN(SpotM14);
SIZE_UNKNOWN(SpotM98);
