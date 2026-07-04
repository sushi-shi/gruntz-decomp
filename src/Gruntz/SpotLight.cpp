// SpotLight.cpp - re-homed from src/Stub/Discovered.cpp (0x0b1ee0). The
// CSpotLight per-tick update: when the owner's mode (m_owner->m_mode) is 1 it rotates
// the (m_offsetX,m_offsetY) offset by the running angle m_angle (scaled by m_angleStep and the frame
// delta g_645584), folds in the tracked target (m_target->m_x/m_y), and advances
// m_angle; then it (re)resolves the light's bute key into m_lightCfg->m_buteNode when the
// manager's per-cell slot is empty. X/Y within each coordinate pair is inferred;
// offsets are load-bearing. Engine globals + CButeTree::Find are external
// (reloc-masked). flags=base (/O2 /Oi -> fsin/fcos).
#include <rva.h>
#include <Mfc.h>
#include <math.h>
#include <Bute/ButeMgr.h>        // CButeTree::Find
#include <Gruntz/GameRegistry.h> // canonical *0x24556c singleton (light-grid via m_68)

extern "C" unsigned g_645584; // 0x645584 frame delta
extern CButeTree g_buteTree;  // 0x6bf620
extern char s_actKeyA[];      // 0x60a454 "A"

struct SpotM10 {
    char pad[0x114];
    int m_mode; // 0x114
};
struct SpotM14 {
    char pad[0x1c];
    void* m_buteNode; // 0x1c
};
struct SpotM98 {
    char pad[0x5c];
    int m_x; // 0x5c
    int m_y; // 0x60
};
// The light-fx cell grid is the spotlight facet of the registry's reused +0x68
// slot ((MgrObj68*)g_gameReg->m_68; see CGameRegistry.h): a flat i32 grid at +0x1c
// indexed (col + row*15). Authentic per-mode downcast of the canonical singleton.
struct MgrObj68 {
    char pad[0x1c];
    int arr[1]; // 0x1c
};
DATA(0x0024556c)
extern CGameRegistry* g_gameReg;

class CSpotLight {
public:
    char pad00[0x10];
    SpotM10* m_owner;    // 0x10
    SpotM14* m_lightCfg; // 0x14
    char pad18[0x30 - 0x18];
    void* m_prevNode; // 0x30
    char pad34[0x58 - 0x34];
    double m_angleStep; // 0x58
    double m_worldX;    // 0x60
    double m_worldY;    // 0x68
    double m_anchorX;   // 0x70
    double m_anchorY;   // 0x78
    double m_offsetX;   // 0x80
    double m_offsetY;   // 0x88
    double m_angle;     // 0x90
    SpotM98* m_target;  // 0x98
    int m_gridRow;      // 0x9c
    int m_gridCol;      // 0xa0
    int Update_0b1ee0();
};

// @early-stop
// x87 fp-stack scheduling wall: the rotation's fld/fmul/fsub tree + the fxch
// interleave (and the frame-delta fild hoist) follow the x87-fp-stack-schedule
// pattern - prologue, control flow, the m_gridRow*15 cell lookup and the bute re-resolve
// match, but the FP block's fxch ordering is not source-steerable under MSVC5 /O2.
RVA(0x000b1ee0, 0x11d)
int CSpotLight::Update_0b1ee0() {
    if (m_owner->m_mode == 1) {
        double c = cos(m_angle);
        double s = sin(m_angle);
        m_worldX = -(m_offsetY * s + m_offsetX * c);
        m_worldY = m_offsetX * s - m_offsetY * c;
        if (m_target) {
            m_anchorX = (double)m_target->m_x;
            m_anchorY = (double)m_target->m_y;
        }
        m_worldX = m_anchorX + m_worldX;
        m_worldY = m_anchorY + m_worldY;
        m_angle = (double)g_645584 * m_angleStep + m_angle;
    }
    if (((MgrObj68*)g_gameReg->m_cmdGrid)->arr[m_gridCol + m_gridRow * 15] == 0) {
        m_prevNode = m_lightCfg->m_buteNode;
        m_lightCfg->m_buteNode = g_buteTree.Find(s_actKeyA);
    }
    return 0;
}

// class-metadata SIZE sweep (misc-Gruntz A-C): matching-neutral, hosted at
// .cpp EOF (see docs/class-metadata-sweep-log.md). SIZE_UNKNOWN = size not yet pinned.
SIZE_UNKNOWN(CSpotLight);
SIZE_UNKNOWN(MgrObj68);
SIZE_UNKNOWN(SpotM10);
SIZE_UNKNOWN(SpotM14);
SIZE_UNKNOWN(SpotM98);
