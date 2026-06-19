#include <rva.h>
// Wormhole.cpp - CWormhole::LoadColors - the
// wormhole's one-time color-attribute resolver. A CWormhole is a world teleport
// node (RTTI CWormhole, structure/game/world_objects.h); it owns a state
// sub-object at this+0x10 that carries the wormhole kind discriminator (+0x124)
// and a lazily-resolved color id (+0x128).
//
// The method maps the wormhole kind (m_124 == 2 SECRET / == 1 SINGLE-USE /
// else NORMAL) to a color id read once from the global CButeMgr "Wormhole"
// config group via the matched GetIntDef getter (butemgr unit):
//     2  -> GetIntDef("Wormhole", "SecretColor",    1)
//     1  -> GetIntDef("Wormhole", "SingleUseColor", 2)
//   else -> GetIntDef("Wormhole", "NormalColor",    4)
// The lookup is cached in m_128 (done only while m_128 == 0). It then indexes the
// game registry's color table (g_gameReg -> [+0x78] -> [m_128*4 + 0x14])
// and stamps three draw fields on the state object: m_4c = colorEntry, m_50 = 7,
// m_58 = 1.
//
// Field names are placeholders (m_<hexoffset>); only OFFSETS + code bytes are
// load-bearing (campaign doctrine). Plain /O2 /MT (no /GX): a scalar leaf, no
// stack C++ object / EH frame. The "SecretColor" branch is the kind==2 path that
// has NO config tag/key push of its own (the SECRET color is a fixed id 1 fed
// straight into the shared cache/index tail) - the disasm pushes (1, "SecretColor")
// then jumps INTO the NORMAL branch's GetIntDef("Wormhole", key, def) call site,
// so all three default-color branches converge on one GetIntDef call.
// ---------------------------------------------------------------------------

// The global CButeMgr text-config tree (the singleton). Modeled as a
// minimal class so the `ecx=&g_buteMgr; call GetIntDef` shape reloc-masks against
// the already-matched CButeMgr::GetIntDef (butemgr unit).
class CButeMgr {
public:
    int GetIntDef(char *tag, char *key, int def);
};
DATA(0x2453d8)
extern CButeMgr g_buteMgr;

// The global game-registry pointer (an int*). Its +0x78 slot is a
// pointer to the color table; the wormhole color id (m_128) indexes it at
// [m_128*4 + 0x14]. Declared int* to match g_gameReg (the target's reloc).
DATA(0x24556c)
extern int *g_gameReg;

// The "Wormhole" config group + the three color keys (the original source string
// literals; objdiff matches these .data relocations by value against the target).
#define s_Wormhole       "Wormhole"
#define s_SecretColor    "SecretColor"
#define s_SingleUseColor "SingleUseColor"
#define s_NormalColor    "NormalColor"

// ---------------------------------------------------------------------------
// The wormhole state sub-object at CWormhole+0x10. Only the load-bearing member
// offsets the method touches are reconstructed.
// ---------------------------------------------------------------------------
struct CWormholeState {
    char  m_pad00[0x4c];
    int   m_4c;          // +0x4c  draw color entry (= colorTable[m_128*4+0x14])
    int   m_50;          // +0x50  (= 7)
    char  m_pad54[4];
    int   m_58;          // +0x58  (= 1)
    char  m_pad5c[0x124 - 0x5c];
    int   m_124;         // +0x124 wormhole kind discriminator (2/1/other)
    int   m_128;         // +0x128 resolved color id (cached; indexes the reg table)
};

// CWormhole - the world teleport node. The state/draw sub-object pointer lives at
// this+0x10. Only the load-bearing member is reconstructed.
class CWormhole {
public:
    void LoadColors();

    char            m_pad00[0x10];
    CWormholeState *m_10;        // +0x10  the wormhole state/draw sub-object

    // Engine-label backlog stubs.
    void Stub_03fc70();
    void Stub_03fed0();
    void Stub_0412c0();
};

// ---------------------------------------------------------------------------
// CWormhole::LoadColors
RVA(0x411f0, 0xa0)
void CWormhole::LoadColors()
{
    // NB: m_10 is re-dereferenced through `this` (held in esi) on every access -
    // do NOT cache it in a local, or MSVC pins it in a 2nd callee-saved reg (edi)
    // and the schedule diverges (the target keeps only esi = this).
    if (m_10->m_124 == 2) {
        // SECRET: fixed color id 1; falls through to the shared cache/index tail.
        if (m_10->m_128 == 0)
            m_10->m_128 = g_buteMgr.GetIntDef(s_Wormhole, s_SecretColor, 1);
    } else if (m_10->m_124 == 1) {
        // SINGLE-USE.
        if (m_10->m_128 == 0)
            m_10->m_128 = g_buteMgr.GetIntDef(s_Wormhole, s_SingleUseColor, 2);
    } else {
        // NORMAL (default).
        if (m_10->m_128 == 0)
            m_10->m_128 = g_buteMgr.GetIntDef(s_Wormhole, s_NormalColor, 4);
    }

    // Resolve the color-table entry for the cached id + stamp the draw fields.
    // The TAIL caches m_10 once (eax) and reuses it for the id read + all three
    // stores; g_gameReg[+0x78] is the color table, indexed at [m_128*4 + 0x14]
    // (== table[m_128 + 5]). Store order m_58 / m_50 / m_4c.
    CWormholeState *s = m_10;
    int *colorTable = ((int **)g_gameReg)[0x78 / 4];
    int colorEntry = colorTable[s->m_128 + 0x14 / 4];
    s->m_58 = 1;
    s->m_50 = 7;
    s->m_4c = colorEntry;
}

// -------------------------------------------------------------------------
// Engine-label backlog stubs.
// -------------------------------------------------------------------------
// @confidence: med
// @source: rtti-vptr
// @stub
RVA(0x03fc70, 0x1db)
void CWormhole::Stub_03fc70() {}

// @confidence: med
// @source: decomp-xref
// @stub
RVA(0x03fed0, 0xa9)
void CWormhole::Stub_03fed0() {}

// @confidence: med
// @source: decomp-xref
// @stub
RVA(0x0412c0, 0x63)
void CWormhole::Stub_0412c0() {}
