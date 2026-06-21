#include <rva.h>
// CGruntToyTimeSprite.cpp - CGruntToyTimeSprite ctor (matched modulo the /GX
// frame; see CGruntStaminaSprite.cpp for the archetype). Same body shape as the
// stamina/wingz siblings: out-of-line CGruntSprite base (0x7eb00 via thunk
// 0x3224), leaf vptr, ApplyLookupSprite(GAME_GRUNTTOYTIMESPRITE, 1), the "A"
// bute seed, the pose-id guard, then the two anchor constants m_5c=0, m_60=-0x20.
//
// WALL: the body bytes match retail, but this ctor needs the /GX EH frame
// (throwing body calls + a destructible base subobject). The engine_label_stubs
// unit is built flags="base" (no /GX), so the prologue/epilogue EH frame cannot
// be emitted here -> caps ~60%. Matches at flags="eh" (migration to a /GX unit).

struct CToyTimeSpriteObj {
    void ApplyLookupSprite(const char* key, int flag); // 0x1504d0 (__thiscall)
    char m_pad00[0x08];
    int m_08; // +0x08
    char m_pad0c[0x74 - 0x0c];
    int m_74; // +0x74
};

struct CToyTimeSpriteObjAux {
    char m_pad00[0x1c];
    void* m_1c; // +0x1c
};

// g_buteTree / CButeTree come from CButeTree.cpp (included earlier in All.cpp).

struct CToyTimeSpriteBase {
    CToyTimeSpriteBase(CToyTimeSpriteObj* obj);
    ~CToyTimeSpriteBase(); // out-of-line; unwound on throw
    void* m_vptr;          // +0x00
    char m_pad04[0x10 - 0x04];
    CToyTimeSpriteObj* m_10;    // +0x10
    CToyTimeSpriteObjAux* m_14; // +0x14
    char m_pad18[0x30 - 0x18];
    void* m_30;                 // +0x30
    char m_pad34[0x38 - 0x34];
    CToyTimeSpriteObj* m_38;    // +0x38
};

class CGruntToyTimeSprite : public CToyTimeSpriteBase {
public:
    CGruntToyTimeSprite(CToyTimeSpriteObj* obj);
    char m_pad3c[0x5c - 0x3c];
    int m_5c; // +0x5c
    int m_60; // +0x60
};

DATA(0x5e79ec)
extern void* g_gruntToyTimeSpriteVtbl;

// @confidence: high
// @source: rtti-vptr
RVA(0x07fbd0, 0xa0)
CGruntToyTimeSprite::CGruntToyTimeSprite(CToyTimeSpriteObj* obj) : CToyTimeSpriteBase(obj) {
    *(void**)this = &g_gruntToyTimeSpriteVtbl;
    m_38->ApplyLookupSprite("GAME_GRUNTTOYTIMESPRITE", 1);
    m_30 = m_14->m_1c;
    m_14->m_1c = g_buteTree.Find("A");
    if (m_10->m_74 != 0xdbba0) {
        m_10->m_74 = 0xdbba0;
        m_10->m_08 |= 0x20000;
    }
    m_5c = 0;
    m_60 = -0x20;
}
