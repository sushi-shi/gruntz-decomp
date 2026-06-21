#include <rva.h>
// CGruntStaminaSprite.cpp - CGruntStaminaSprite ctor (matched).
//
// Sprite-ctor archetype (/GX EH frame). Chains the OUT-OF-LINE CGruntSprite base
// ctor (0x7eb00, via thunk 0x3224; engine fn, not matched -> per-file shell
// whose call reloc-masks by address). The base constructs a throwing
// CUserBaseLink and the body calls can throw, so MSVC emits the /GX frame that
// unwinds the base subobject (its declared dtor) on throw.
//
// Body: stamp the leaf vftable, register the sprite's GAME bute via
// m_38->ApplyLookupSprite(str, 1), seed the "A" bute node, force the object's
// pose id, then set the two sprite anchor fields m_5c/m_60. The sibling
// CGruntToyTime/WingzTimeSprite ctors are the same shape (different vtbl/str and
// m_5c/m_60 constants).

// --- engine helper types (offsets load-bearing; per-file unique names) -----
struct CStaminaSpriteObj {
    void ApplyLookupSprite(const char* key, int flag); // 0x1504d0 (__thiscall)
    char m_pad00[0x08];
    int m_08; // +0x08
    char m_pad0c[0x74 - 0x0c];
    int m_74; // +0x74
};

struct CStaminaSpriteObjAux {
    char m_pad00[0x1c];
    void* m_1c; // +0x1c
};

// g_buteTree / CButeTree come from CButeTree.cpp (included earlier in All.cpp).

// --- out-of-line base shell (reloc-masks to CGruntSprite ctor 0x7eb00) -----
struct CStaminaSpriteBase {
    CStaminaSpriteBase(CStaminaSpriteObj* obj);
    ~CStaminaSpriteBase(); // out-of-line; unwound on throw
    void* m_vptr;          // +0x00
    char m_pad04[0x10 - 0x04];
    CStaminaSpriteObj* m_10;    // +0x10
    CStaminaSpriteObjAux* m_14; // +0x14
    char m_pad18[0x30 - 0x18];
    void* m_30;                 // +0x30
    char m_pad34[0x38 - 0x34];
    CStaminaSpriteObj* m_38;    // +0x38
};

class CGruntStaminaSprite : public CStaminaSpriteBase {
public:
    CGruntStaminaSprite(CStaminaSpriteObj* obj);
    char m_pad3c[0x5c - 0x3c];
    int m_5c; // +0x5c
    int m_60; // +0x60
};

// Leaf vftable, referenced as DIR32 data.
DATA(0x5e7a44)
extern void* g_gruntStaminaSpriteVtbl;

// @confidence: high
// @source: rtti-vptr
RVA(0x07fae0, 0xa0)
CGruntStaminaSprite::CGruntStaminaSprite(CStaminaSpriteObj* obj) : CStaminaSpriteBase(obj) {
    *(void**)this = &g_gruntStaminaSpriteVtbl;
    m_38->ApplyLookupSprite("GAME_GRUNTSTAMINASPRITE", 1);
    m_30 = m_14->m_1c;
    m_14->m_1c = g_buteTree.Find("A");
    if (m_10->m_74 != 0xdbba0) {
        m_10->m_74 = 0xdbba0;
        m_10->m_08 |= 0x20000;
    }
    m_5c = 0x64;
    m_60 = -0x20;
}
