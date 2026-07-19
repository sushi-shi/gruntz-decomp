// StreamRecordLoaders.cpp - the CEventLoadRec main cluster (0x09c650) + the spatially
// adjacent 0x09cab0 orphan out-param wrapper. The former conflated peers - the trigger
// (0x009bb0), grunt-state (0x0ea990) and projectile (0x0e0d40) record loaders - were
// carved into their own TUs (TriggerLoadRec/GruntStateRec/ProjLoadRec.cpp; operation
// REHOME package D8), each an independent obj at its own far-away .text block.
//
// Both records here stream through the shared WAP32 CSerialArchive stream-reader (a
// virtual Read at vtable +0x2c), with string-valued fields read into a scratch buffer
// and interned against the game registry's name->object map (g_gameReg->m_world +0x10,
// CDDrawWorkerRegistry's CMapStringToOb::Lookup). A global sequence counter
// (g_serialCounter) ticks once per string read. Each Load is a __thiscall taking the
// reader (ret 4); names are placeholders, only the field offsets + code bytes are
// load-bearing. The ex-view registry chain (CRegSub30 -> CRegTypeTable) is the REAL
// g_gameReg->m_world (CDDrawSurfaceMgr) -> m_imageRegistry -> m_10map -> CSprite
// chain: the indexed "type table" is the same [m_firstFrame..m_lastFrame]-gated
// CSprite frame resolve CSBI_Image::SerializeFields runs.
#include <rva.h>
#include <Gruntz/GameRegPtr.h>
#include <Io/FileMem.h>           // the serialize stream (CSerialArchive == the real CFileMemBase)
#include <Gruntz/MgrSettings.h>   // CDDrawWorkerRegistry (the name map at g_gameReg->m_world +0x10)
#include <Gruntz/GameRegistry.h>  // CGameRegistry (g_gameReg->m_world = CDDrawSurfaceMgr*)
#include <Gruntz/SerialArchive.h> // CSerialArchive (reader; Read @ vtable +0x2c)
#include <Gruntz/StreamRecordLoaders.h> // CEventLoadRec (this TU owns the loader)
#include <Gruntz/Sprite.h>             // CSprite (the looked-up, index-gated frame table)
#include <DDrawMgr/DDrawWorkerCache.h> // the +0x14 worker cache - Find (0x9cab0) is its method
#include <string.h>                    // inline strlen (repne scasb) over the scratch buffer

// The game registry singleton (0x64556c). The delinker's canonical symbol is the
// extern "C" _g_mgrSettings (the cplay unit owns it); reloc-masked DIR32.

// The serialize sequence counter (0x629ad0, ?g_serialCounter@@3HA): bumped once
// per string field read.

// ===========================================================================
// CEventLoadRec::Load (0x09c650) - the HandleEvent-path serialized record. Same
// reader+registry idiom as CTriggerLoadRec but a different field layout: two raw
// dwords, one plain name ref (m_8), one raw dword, five bounds-checked type-table
// index refs (m_10..m_20), then two trailing raw dwords (m_48/m_4c). __thiscall,
// ret 4. Note: unlike CTriggerLoadRec it does NOT null-check g_gameReg, only
// the m_30 sub-registry.
// ===========================================================================

// @early-stop
// outparam-zeroinit-scheduling wall (same as CTriggerLoadRec): logic + offsets
// byte-exact, residual is only the 6 `out = 0` store positions (retail sinks, cl
// hoists). The idx-in-callee-saved-reg regalloc is steered by the `i32 i = idx;`
// copy. ~92%.
RVA(0x0009c650, 0x372)
i32 CEventLoadRec::Load(CSerialArchive* s) {
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

    s->Read(&m_0, 4);
    s->Read(&m_4, 4);

    g_serialCounter++;
    s->Read(buf, 0x80);
    if (strlen(buf) != 0) {
        out = 0;
        reg->m_imageRegistry->m_10map.Lookup(buf, out);
        m_8 = out;
    } else {
        m_8 = 0;
    }

    s->Read(&m_c, 4);

    g_serialCounter++;
    s->Read(buf, 0x80);
    s->Read(&idx, 4);
    if (strlen(buf) != 0) {
        i32 i = idx;
        out = 0;
        reg->m_imageRegistry->m_10map.Lookup(buf, out);
        CSprite* tt = static_cast<CSprite*>(out);
        void* r;
        if (tt != 0 && i >= tt->m_minIndex && i <= tt->m_maxIndex) {
            r = reinterpret_cast<CImage*>(tt->m_items.GetAt(i));
        } else {
            r = 0;
        }
        m_10 = r;
    } else {
        m_10 = 0;
    }

    g_serialCounter++;
    s->Read(buf, 0x80);
    s->Read(&idx, 4);
    if (strlen(buf) != 0) {
        i32 i = idx;
        out = 0;
        reg->m_imageRegistry->m_10map.Lookup(buf, out);
        CSprite* tt = static_cast<CSprite*>(out);
        void* r;
        if (tt != 0 && i >= tt->m_minIndex && i <= tt->m_maxIndex) {
            r = reinterpret_cast<CImage*>(tt->m_items.GetAt(i));
        } else {
            r = 0;
        }
        m_14 = r;
    } else {
        m_14 = 0;
    }

    g_serialCounter++;
    s->Read(buf, 0x80);
    s->Read(&idx, 4);
    if (strlen(buf) != 0) {
        i32 i = idx;
        out = 0;
        reg->m_imageRegistry->m_10map.Lookup(buf, out);
        CSprite* tt = static_cast<CSprite*>(out);
        void* r;
        if (tt != 0 && i >= tt->m_minIndex && i <= tt->m_maxIndex) {
            r = reinterpret_cast<CImage*>(tt->m_items.GetAt(i));
        } else {
            r = 0;
        }
        m_18 = r;
    } else {
        m_18 = 0;
    }

    g_serialCounter++;
    s->Read(buf, 0x80);
    s->Read(&idx, 4);
    if (strlen(buf) != 0) {
        i32 i = idx;
        out = 0;
        reg->m_imageRegistry->m_10map.Lookup(buf, out);
        CSprite* tt = static_cast<CSprite*>(out);
        void* r;
        if (tt != 0 && i >= tt->m_minIndex && i <= tt->m_maxIndex) {
            r = reinterpret_cast<CImage*>(tt->m_items.GetAt(i));
        } else {
            r = 0;
        }
        m_1c = r;
    } else {
        m_1c = 0;
    }

    g_serialCounter++;
    s->Read(buf, 0x80);
    s->Read(&idx, 4);
    if (strlen(buf) != 0) {
        i32 i = idx;
        out = 0;
        reg->m_imageRegistry->m_10map.Lookup(buf, out);
        CSprite* tt = static_cast<CSprite*>(out);
        void* r;
        if (tt != 0 && i >= tt->m_minIndex && i <= tt->m_maxIndex) {
            r = reinterpret_cast<CImage*>(tt->m_items.GetAt(i));
        } else {
            r = 0;
        }
        m_20 = r;
    } else {
        m_20 = 0;
    }

    s->Read(&m_48, 4);
    s->Read(&m_4c, 4);

    return 1;
}
SIZE_UNKNOWN(CEventLoadRec);

// ---------------------------------------------------------------------------
// CDDrawWorkerCache::Find (0x09cab0; spatially re-homed from src/Stub/
// BoundaryLowerMethods.cpp, adjacent to CEventLoadRec at 0x09c650). Out-param
// wrapper: call the +0x10 name->object map's Lookup (the real MFC
// CMapStringToOb::Lookup @0x1b8008) with a zeroed local and return the filled
// local. OWNER RECOVERED (Fable A2, 2026-07-14): the registry with the
// CMapStringToOb @+0x10 and the virtual registrar at slot 9 (+0x24) IS the
// canonical CDDrawWorkerCache (<DDrawMgr/DDrawWorkerCache.h>, vtbl 0x1efd00,
// CreateWorker [9] @0x1652c0) - the CDDrawSurfaceMgr/CDDrawSurfaceMgr +0x14
// child; the tile-logic leaf ctors probe it via thunk 0x1703. (Was the C9cab0
// placeholder.)
RVA(0x0009cab0, 0x23)
i32 CDDrawWorkerCache::Find(const char* key) {
    i32 local = 0;
    m_10.Lookup(key, reinterpret_cast<CObject*&>(local));
    return local;
}
