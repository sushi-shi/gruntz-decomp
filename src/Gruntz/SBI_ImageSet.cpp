#define SBI_DTOR_CHAIN        // enable the inline base-dtor bodies (see StatusBarItem.h)
#define SBI_OWN_IMAGESET_DTOR // this TU supplies the out-of-line ~CSBI_ImageSet (0x102000)
#include <rva.h>
#include <Gruntz/SerialCounter.h> // g_serialCounter
#include <Io/FileMem.h> // CFileMemBase - the CSerialArchive stream (Read/Write dispatch)
#include <Mfc.h>
#include <Ints.h>
#include <Gruntz/ResMgr.h> // canonical g_gameReg->m_world view (CResMgr + CImageRegistry + CSprite)
#include <Gruntz/SBI_ImageSet.h> // canonical CSBI_ImageSet + CImageSetStream (the frameless method view)
#include <Gruntz/GameRegistry.h> // canonical g_gameReg singleton (CSpriteFactoryHolder m_world)
#include <Gruntz/SbiConfig.h>    // canonical config-host family (SetupImage's map lookup)
#include <Image/CImage.h>        // the resolved frame record (TickRenderFrame's blit)
// SBI_ImageSet.cpp - Gruntz CSBI_ImageSet (C:\Proj\Gruntz), the frameless methods.
// RTTI .?AVCSBI_ImageSet@@; the most-derived of the SBI image chain
//   CSBI_ImageSet : CSBI_Image : CSBI_RectOnly : CStatusBarItem.
// Vtable @0x5eac4c. The /GX chain destructor (0x102000) is defined below - the
// former SBI_ImageSetEh.cpp companion split is collapsed (retail's one TU was
// /GX). Sibling/engine callees are ILT/vtable-reloc-masked.

// ---------------------------------------------------------------------------
// Shared engine views (modeled minimally; only touched members/methods are
// load-bearing; every call through them is reloc-masked). CSBI_ImageSet +
// CImageSetStream now live in the canonical <Gruntz/SBI_ImageSet.h>.

// The resolved config record (Lookup result) is the CSprite the image registry
// yields; its config name string lives at record+0x24. The config map is the image
// registry's embedded m_10map (CSpriteHashTable, Lookup 0x1b8008) - the same map
// shape SetupImage uses, reached as reg->m_10->m_10map.

extern "C" CGameRegistry* g_gameReg;

// The serialize-sequence counter bumped once per non-trivial pass.

// ---------------------------------------------------------------------------
// CSBI_ImageSet::SetupImage (0xe72f0, vtable slot 11 - the CSBI_Image::SetupImage
// override): validate + latch the config descriptor (subtype, command, the four
// rect coords, the rect-test command), then look the record key up in the host's
// map and, if found, resolve the start frame from the record's frame range/table.
// Re-attributed from the SBI_RectOnly host TU's "ConfigureRect" (dossier #16:
// vtbl 0x1eac4c slot [11] thunk 0x263a -> 0xe72f0; the fields are the thin chain's).
// @early-stop
// ~62%: the gate, every field latch (same statement order as retail), the map
// lookup and the frame-range resolution are byte-correct in instruction selection,
// but retail stages the four rect coords through a reused `edx = this+0x14`
// pointer where the recompile addresses them as direct `[this+0x14..]` offsets,
// and the surrounding store schedule + register naming diverge accordingly. An
// addressing-mode/scheduling wall, not steerable from C; deferred.
RVA(0x000e72f0, 0xc4)
i32 CSBI_ImageSet::SetupImage(
    CStatusBarMgr* owner,
    CSpriteFactoryHolder* host,
    i32 cmd,
    i32 obj,
    SbRect rect,
    const char* key,
    i32 frame,
    i32 extra
) {
    (void)extra;
    if (host == 0 || owner == 0) {
        return 0;
    }
    m_2c = (i32)owner;
    m_10 = obj;
    i32* rc = (i32*)&m_rect14;
    m_24 = (i32)host;
    m_28 = 0;
    m_4 = 1;
    rc[0] = rect.left;
    rc[1] = rect.top;
    rc[2] = rect.right;
    rc[3] = rect.bottom;
    m_c = cmd;
    if (key == 0) {
        return 0;
    }
    CSbiConfigRecord* rec = 0;
    ((CMapStringToPtr*)&host->m_10->m_10map)->Lookup(key, (void*&)rec);
    m_34 = (CSprite*)rec;
    if (rec == 0) {
        return 0;
    }
    i32 f = frame;
    if (f == -1) {
        f = rec->m_64;
    }
    m_38 = f;
    if (f >= rec->m_64 && f <= rec->m_68) {
        m_30 = rec->m_14[f];
    } else {
        m_30 = 0;
    }
    return 1;
}

// ---------------------------------------------------------------------------
// CSBI_ImageSet::ResetCounters (0xe7400, vtable slot 3): m_34 = m_30 = 0.
// Re-attributed from the SBI_RectOnly host TU (dossier #16: vtbl 0x1eac4c
// slot [3] thunk 0x2a09 -> 0xe7400).
RVA(0x000e7400, 0x9)
void CSBI_ImageSet::ResetCounters() {
    m_34 = 0;
    m_30 = 0;
}

// ---------------------------------------------------------------------------
// CSBI_ImageSet::TickRenderFrame (0xe7440, vtable slot 5): one play step that
// re-resolves the frame from the record table and renders it: while cycles remain,
// consume one, look up the frame (0 when out of range), record it, and - when
// present - blit it. Returns 1. Ex CAniPlayer view (dossier #16).
// @early-stop
// 86.7% - logic + externs byte-exact; residual is the same RenderFrame surface-context
// chain regalloc as TickRenderCurrent_0e6dd0 plus the record-table range-test register
// pairing. Final sweep.
RVA(0x000e7440, 0x5e)
i32 CSBI_ImageSet::TickRenderFrame_0e7440() {
    if (m_28 > 0) {
        m_28--;
        CSbiConfigRecord* tbl = (CSbiConfigRecord*)m_34;
        CImage* cel;
        if (m_38 >= tbl->m_64 && m_38 <= tbl->m_68) {
            cel = (CImage*)tbl->m_14[m_38];
        } else {
            cel = 0;
        }
        m_30 = (i32)cel;
        if (cel != 0) {
            cel->RenderFrame(
                (void*)(i32)((CResMgr*)g_gameReg->m_world)->m_drawTarget->m_14,
                (void*)(cel->m_anchorX + m_rect14.m_0),
                (void*)(cel->m_anchorY + m_rect14.m_4),
                (void*)0
            );
        }
    }
    return 1;
}

// ---------------------------------------------------------------------------
// vtable slot 1 (0xe74f0): serialize the config id + name. mode 7 = load (read id,
// read name, resolve record), mode 4 = save (write id, write name from the record);
// either way chain to the base serialize and return its normalized truth.
// @early-stop
// 99.2% (entropy tail): logic + control flow + inline strcpy/strlen + the typed
// vtable (Read/WriteBytes) + the switch dispatch are all byte-exact. The only
// residual is ONE extra `mov [esp+0x18],eax` (retail keeps the dead strlen result
// live before Lookup) + the consequent 1-byte branch-displacement shifts, plus the
// reloc-masked Lookup/Read/WriteBytes/BaseSerialize/g_* operands. Naming the strlen
// result to recover the dead store regresses it (98.4%) - a non-steerable /O2
// dead-store artifact (docs/patterns/reloc-typing-vptr-global.md). Effectively done.
RVA(0x000e74f0, 0x152)
i32 CSBI_ImageSet::Serialize(CSerialArchive* s, i32 mode, i32 a3, i32 a4) {
    if (s == 0) {
        return 0;
    }
    CSpriteFactoryHolder* reg = g_gameReg->m_world;
    if (reg == 0) {
        return 0;
    }
    char buf[0x80];
    switch (mode) {
        case 7:
            s->Read(&m_38, 4);
            g_serialCounter++;
            s->Read(buf, 0x80);
            if (strlen(buf)) {
                CSprite* out;
                reg->m_10->m_10map.Lookup(buf, (CObject*&)out);
                m_34 = out;
            } else {
                m_34 = 0;
            }
            break;
        case 4:
            s->Write(&m_38, 4);
            g_serialCounter++;
            memset(buf, 0, 0x80);
            if (m_34) {
                strcpy(buf, m_34->m_name);
            }
            s->Write(buf, 0x80);
            break;
    }
    return SerializeChain(s, mode, a3, a4) != 0; // the CSBI_Image base leg (0xe6e40)
}

// ---------------------------------------------------------------------------
// ~CSBI_ImageSet (0x102000): the /GX chain destructor - stamp ??_7CSBI_ImageSet,
// run ResetCounters (0xe7400, this class's own member teardown), then MSVC folds the
// three inline base dtors in (??_7CSBI_Image + ClearFrame, ??_7CSBI_RectOnly +
// DtorRect, ??_7CStatusBarItem + DtorStatus - the SBI_DTOR_CHAIN device; this TU owns
// ~CSBI_ImageSet itself via SBI_OWN_IMAGESET_DTOR) behind the /GX SEH frame with
// 0/1/2/-1 trylevels. Collapsed from SBI_ImageSetEh.cpp (4-level case of
// docs/patterns/eh-dtor-multilevel-polymorphic-chain.md).
RVA(0x00102000, 0x7f)
CSBI_ImageSet::~CSBI_ImageSet() {
    ResetCounters();
}
