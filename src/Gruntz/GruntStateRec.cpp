#include <rva.h>
#include <Gruntz/GameRegMfcPtr.h> // g_gameReg at its REAL type (CGruntzMgr)
#include <Gruntz/GruntzMgr.h>
#include <Io/FileMem.h>          // the serialize stream (CFileMemBase == the real CFileMemBase)
#include <Gruntz/MgrSettings.h>  // CDDrawWorkerRegistry (name map + AnyValueMatches)
#include <Gruntz/GameRegistry.h> // CGameRegistry (g_gameReg->m_world)
#include <Gruntz/SBI_StatzTabGruntBar.h> // the REAL class
#include <Gruntz/Sprite.h> // CDDrawWorker - the glyph maps ARE frame-data sprites (ex CStatzGlyphMap)
#include <string.h>                      // inline strlen / strcpy / memset over the scratch buffer

// ===========================================================================
// CSBI_StatzTabGruntBar::SerializeFields (0x0ea990) - the stat tab's dual-mode slot-1
// serialize. A __thiscall taking (stream, mode, a2, a3), ret 0x10. Bails (0) when the
// stream or the registry sub-object (g_gameReg->m_world) is absent.
//
// Mode 7 (read) resolves each tracked glyph from the stream as a registry ref: a name +
// an index, Lookup'd to a glyph map, then gated to (CImage*)map->m_items.GetAt(index) (the same
// name -> Lookup -> [m_minIndex..m_maxIndex] -> frame idiom CSBI_Image::SerializeFields
// runs against the real CDDrawWorker). Mode 4 (write) reverse-looks-up each glyph's name +
// index through the registry (AnyValueMatches) and writes them back. The two
// glyph MAPS themselves (m_glyphMap/m_timerGlyphMap) round-trip by name only.
//
// The stream is the shared WAP32 CFileMemBase: Read at vtable +0x2c (mode 7) and
// Write at +0x30 (mode 4) - both slots off the one type.
// ===========================================================================
// @early-stop
// scratch-slot scheduling tail (same family as CTriggerLoadRec/CEventLoadRec/
// CArchiveLoadRec): the dual-mode switch, every Read field/size, the indexed-ref
// bounds checks, the name-ref Lookups, the FillDefault sub-records, the inline
// strlen/strcpy, the g_serialCounter bumps and the tail-chain + 0/1 normalise are
// byte-faithful; residual is the MSVC5 scratch-buffer slot assignment + the
// outparam zero-init store positions. Not source-steerable.
RVA(0x000ea990, 0xa72)
i32 CSBI_StatzTabGruntBar::SerializeFields(CFileMemBase* s, i32 mode, i32 a2, i32 a3) {
    if (s == 0) {
        return 0;
    }
    CDDrawSurfaceMgr* reg = g_gameReg->m_world;
    if (reg == 0) {
        return 0;
    }

    char buf[0x80];
    CObject* out;
    i32 idx;
    i32 v;

    switch (mode) {
        case 4:
            // --- mode 4 (store): reverse-look-up each glyph's name + index ---
#define GS_SUBREC(field)                                                                           \
    g_serialCounter++;                                                                             \
    memset(buf, 0, sizeof(buf));                                                                   \
    v = 0;                                                                                         \
    if (field != 0) {                                                                              \
        reg->m_imageRegistry->AnyValueMatches(field, buf, &v);                                       \
    }                                                                                              \
    s->Write(buf, 0x80);                                                                           \
    s->Write(&v, 4)

            GS_SUBREC(m_statusGlyph);
            GS_SUBREC(m_statusGlyphLatched);
            s->Write(&m_statusValue, 4);
            // (m_abilityGlyph is written here but NOT read back in mode 7 below - the
            // asymmetry is retail's, preserved verbatim.)
            GS_SUBREC(m_abilityGlyph);
            GS_SUBREC(m_abilityGlyphLatched);
            s->Write(&m_abilityValue, 4);
            GS_SUBREC(m_overrideGlyph);
            GS_SUBREC(m_overrideGlyphLatched);
            s->Write(&m_overrideValue, 4);
            GS_SUBREC(m_selectKey);
            GS_SUBREC(m_selectGlyph);
            s->Write(&m_selectValue, 4);
            GS_SUBREC(m_timerGlyph);
            s->Write(&m_timerValue, 4);
            s->Write(&m_unitRow, 4);
            s->Write(&m_unitCol, 4);
#undef GS_SUBREC

            g_serialCounter++;
            memset(buf, 0, sizeof(buf));
            if (m_glyphMap != 0) {
                strcpy(buf, m_glyphMap->m_name);
            }
            s->Write(buf, 0x80);

            g_serialCounter++;
            memset(buf, 0, sizeof(buf));
            if (m_timerGlyphMap != 0) {
                strcpy(buf, m_timerGlyphMap->m_name);
            }
            s->Write(buf, 0x80);
            break;

        case 7:
            // --- mode 7 (load): each glyph = a name-Lookup'd map, gated by index ---
#define GS_IDXREF(field)                                                                           \
    g_serialCounter++;                                                                             \
    s->Read(buf, 0x80);                                                                            \
    s->Read(&idx, 4);                                                                              \
    if (strlen(buf) != 0) {                                                                        \
        i32 i = idx;                                                                               \
        out = 0;                                                                                   \
        reg->m_imageRegistry->m_10map.Lookup(buf, out);                                                       \
        CDDrawWorker* gm = static_cast<CDDrawWorker*>(out);                                                 \
        CImage* r;                                                                                 \
        if (gm != 0 && i >= gm->m_minIndex && i <= gm->m_maxIndex) {                                  \
            r = static_cast<CImage*>(gm->m_items.GetAt(i));                                                                    \
        } else {                                                                                   \
            r = 0;                                                                                 \
        }                                                                                          \
        field = r;                                                                                 \
    } else {                                                                                       \
        field = 0;                                                                                 \
    }
#define GS_NAMEREF(field)                                                                          \
    g_serialCounter++;                                                                             \
    s->Read(buf, 0x80);                                                                            \
    if (strlen(buf) != 0) {                                                                        \
        out = 0;                                                                                   \
        reg->m_imageRegistry->m_10map.Lookup(buf, out);                                                       \
        field = reinterpret_cast<CDDrawWorker*>(out);                                                              \
    } else {                                                                                       \
        field = 0;                                                                                 \
    }

            GS_IDXREF(m_statusGlyph);
            GS_IDXREF(m_statusGlyphLatched);
            s->Read(&m_statusValue, 4);
            GS_IDXREF(m_abilityGlyphLatched);
            s->Read(&m_abilityValue, 4);
            GS_IDXREF(m_overrideGlyph);
            GS_IDXREF(m_overrideGlyphLatched);
            s->Read(&m_overrideValue, 4);
            GS_IDXREF(m_selectKey);
            GS_IDXREF(m_selectGlyph);
            s->Read(&m_selectValue, 4);
            GS_IDXREF(m_timerGlyph);
            s->Read(&m_timerValue, 4);
            s->Read(&m_unitRow, 4);
            s->Read(&m_unitCol, 4);
            GS_NAMEREF(m_glyphMap);
            GS_NAMEREF(m_timerGlyphMap);
#undef GS_IDXREF
#undef GS_NAMEREF
            break;
    }

    // QUALIFIED = the direct base leg (retail `call 0x1848`); this was the view's
    // fabricated `ChainLoad`. Unqualified would now be recursion on this override.
    return CStatusBarItem::SerializeFields(s, mode, a2, a3) != 0 ? 1 : 0;
}
