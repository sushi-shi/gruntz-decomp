// LightFxMgr.cpp - CLightFxMgr, the light-FX / translucency shade-table manager
// hung off the game registry at g_gameReg->m_78 (tracer placeholder
// tomalla-3). Init builds the engine's pre-computed RGB565 color tables out
// of the shade-table cache (reg->m_50): one identity "grey" remap, one additive
// glow table, and eight subtractive color tables (one per fixed effect color),
// then registers the grey table globally (key 9). Reset zeroes the slots. Push
// applies a chosen table to an image-set's frames.
//
// Methods in ascending retail-RVA order. Field names are placeholders; offsets +
// code bytes are load-bearing. The CShadeTableCache builders, the global-table
// registrar (0x14dcf0), and the CImageSet frame accessors are external/no-body so
// their calls reloc-mask. No destructible stack locals -> plain /O2 (base flags).
#include <Gruntz/LightFxMgr.h>

#include <DDrawMgr/ShadeTableCache.h>
#include <Image/ImageSet.h>
#include <rva.h>

#include <string.h> // memset -> inline rep stosd in Reset

// The registry fields Init reads: the sprite-factory holder (+0x30) and the
// shade-table cache (+0x50). Modeled as a tiny view so the loads reloc-mask
// cleanly without depending on the full CGameRegistry layout.
struct LfxReg {
    char m_pad00[0x30];
    void* m_30; // +0x30  sprite-factory holder
    char m_pad34[0x50 - 0x34];
    CShadeTableCache* m_50; // +0x50  shade-table cache
};

// The global table registrar (0x14dcf0, __cdecl): store `table` into the screen
// shade-blit global selected by `key`. External/no-body -> reloc-masked.
extern "C" void LfxRegisterTable(void* table, i32 key); // 0x14dcf0

// ===========================================================================
// Init: bind to the registry, fetch the shade cache, build the grey +
// additive + eight subtractive color tables, and register the grey table (key 9).
// ===========================================================================
RVA(0x0009dad0, 0x14a)
i32 CLightFxMgr::Init(CGameRegistry* reg, void* owner) {
    LfxReg* r = (LfxReg*)reg;
    if (!r) {
        return 0;
    }
    m_04 = reg;
    m_00 = owner;
    m_08 = r->m_30;
    m_0c = r->m_50;
    // Re-read m_0c per builder call (don't cache it in a local) so the compiler
    // reloads [this+0xc] before each call rather than pinning the cache in a
    // callee-saved reg + an extra push. See reread-member-view-pointer.md.
    if (!m_0c) {
        return 0;
    }
    m_10 = m_0c->GreyTable();
    if (!m_10) {
        return 0;
    }
    m_tables[0] = m_0c->AddTable(2.0f);
    if (!m_tables[0]) {
        return 0;
    }
    m_tables[1] = m_0c->SubTable(0xff);
    if (!m_tables[1]) {
        return 0;
    }
    m_tables[2] = m_0c->SubTable(0xff00);
    if (!m_tables[1]) {
        return 0;
    }
    m_tables[3] = m_0c->SubTable(0xff0000);
    if (!m_tables[3]) {
        return 0;
    }
    m_tables[4] = m_0c->SubTable(0xffff);
    if (!m_tables[4]) {
        return 0;
    }
    m_tables[5] = m_0c->SubTable(0x202020);
    if (!m_tables[5]) {
        return 0;
    }
    m_tables[6] = m_0c->SubTable(0xff8080);
    if (!m_tables[6]) {
        return 0;
    }
    m_tables[7] = m_0c->SubTable(0xc000c0);
    if (!m_tables[7]) {
        return 0;
    }
    m_tables[8] = m_0c->SubTable(0x60c0);
    if (!m_tables[8]) {
        return 0;
    }
    m_tables[9] = m_0c->SubTable(0xc0c0c0);
    if (!m_tables[9]) {
        return 0;
    }
    LfxRegisterTable(m_10, 9);
    return 1;
}

// ===========================================================================
// Reset: zero the bound pointers and all 10 table slots.
// ===========================================================================
RVA(0x0009dc80, 0x1d)
void CLightFxMgr::Reset() {
    m_04 = 0;
    m_08 = 0;
    m_0c = 0;
    m_10 = 0;
    memset(m_tables, 0, sizeof(m_tables));
}

// ===========================================================================
// Push: apply the shade table chosen by `anchor` (clamped to [0,10)) to
// every frame of `imgSet`: re-type the frames (slot), then write the table's
// resolved format word.
// ===========================================================================
RVA(0x0009dcb0, 0x41)
i32 CLightFxMgr::Push(CImageSet* imgSet, i32 anchor, i32 slot) {
    if (!imgSet) {
        return 0;
    }
    if (anchor < 0 || anchor >= 10) {
        anchor = 0;
    }
    void* table = m_tables[anchor];
    imgSet->SetAllTypes(slot);
    imgSet->SetAllFormats((i32)table);
    return 1;
}

SIZE_UNKNOWN(LfxReg);
