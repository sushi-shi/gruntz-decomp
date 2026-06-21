#include <rva.h>
// CGruntWingzTimeSprite.cpp - CGruntWingzTimeSprite ctor (matched modulo the /GX
// frame; see CGruntStaminaSprite.cpp for the archetype). Same body shape as the
// stamina/toytime siblings: out-of-line CGruntSprite base (0x7eb00 via thunk
// 0x3224), leaf vptr, ApplyLookupSprite(GAME_GRUNTWINGZTIMESPRITE, 1), the "A"
// bute seed, the pose-id guard, then the anchor constants m_5c=0, m_60=-0x26.
//
// WALL: same /GX unit-flags wall as the siblings (engine_label_stubs is
// flags="base"); body bytes match retail, the EH frame caps it ~60%.

struct CWingzTimeSpriteObj {
    void ApplyLookupSprite(const char* key, int flag); // 0x1504d0 (__thiscall)
    char m_pad00[0x08];
    int m_08; // +0x08
    char m_pad0c[0x74 - 0x0c];
    int m_74; // +0x74
};

struct CWingzTimeSpriteObjAux {
    char m_pad00[0x1c];
    void* m_1c; // +0x1c
};

// g_buteTree / CButeTree come from CButeTree.cpp (included earlier in All.cpp).

struct CWingzTimeSpriteBase {
    CWingzTimeSpriteBase(CWingzTimeSpriteObj* obj);
    ~CWingzTimeSpriteBase(); // out-of-line; unwound on throw
    void* m_vptr;            // +0x00
    char m_pad04[0x10 - 0x04];
    CWingzTimeSpriteObj* m_10;    // +0x10
    CWingzTimeSpriteObjAux* m_14; // +0x14
    char m_pad18[0x30 - 0x18];
    void* m_30;                   // +0x30
    char m_pad34[0x38 - 0x34];
    CWingzTimeSpriteObj* m_38;    // +0x38
};

class CGruntWingzTimeSprite : public CWingzTimeSpriteBase {
public:
    CGruntWingzTimeSprite(CWingzTimeSpriteObj* obj);
    char m_pad3c[0x5c - 0x3c];
    int m_5c; // +0x5c
    int m_60; // +0x60
};

DATA(0x5e77cc)
extern void* g_gruntWingzTimeSpriteVtbl;

// @confidence: high
// @source: rtti-vptr
RVA(0x07fcc0, 0xa0)
CGruntWingzTimeSprite::CGruntWingzTimeSprite(CWingzTimeSpriteObj* obj) : CWingzTimeSpriteBase(obj) {
    *(void**)this = &g_gruntWingzTimeSpriteVtbl;
    m_38->ApplyLookupSprite("GAME_GRUNTWINGZTIMESPRITE", 1);
    m_30 = m_14->m_1c;
    m_14->m_1c = g_buteTree.Find("A");
    if (m_10->m_74 != 0xdbba0) {
        m_10->m_74 = 0xdbba0;
        m_10->m_08 |= 0x20000;
    }
    m_5c = 0;
    m_60 = -0x26;
}
