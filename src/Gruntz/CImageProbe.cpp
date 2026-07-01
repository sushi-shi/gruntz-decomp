// CImageProbe.cpp - the image-source probe/resolve init (RVA 0x17cbe0).
//
// Fills a 0x6c-byte surface-request descriptor (embedded at +0x9c) from the
// source-info object (+0x10), asks the source provider (+0x14, COM slot 6) to
// build the descriptor into an image source (+0x28), probes that source for the
// payload (+0x24, the engine's shared CImageSource::Probe slot 0 + g_imageProbeTag
// tag, the SAME path Image.cpp::CImageFactory uses), and - when the mode field
// (+0x520) selects format 8 - hands the payload to the bound surface (+0x2c)
// through payload slot 0x7c. Returns 0 on any sub-step failure, else 1.
//
// The provider/source/payload are the engine's COM-style interfaces (vtbl ptr @0,
// __stdcall methods taking the object as the explicit first arg - the same idiom
// as IDirectDrawSurfaceZ in CDirectDrawMgr.cpp); modeled NO-body so every dispatch
// reloc-masks. Field names are placeholders; only offsets + code bytes are
// load-bearing.
#include <rva.h>

#include <string.h> // inline memset (rep stos) for the descriptor zero

// The probe tag passed to the source's slot-0 Probe (reloc-masked .rdata datum,
// the SAME global Image.cpp names).
DATA(0x001ef888)
extern void* g_imageProbeTag; // 0x5ef888

// The resolved image source (CImageSource): slot-0 Probe(magic, &out).
struct CImageSource;
struct CImageSourceVtbl {
    i32(__stdcall* Probe)(CImageSource*, void* magic, void** out); // +0x00
};
struct CImageSource {
    CImageSourceVtbl* vtbl;
};

// The probed payload object: slot 0x7c hands it the bound surface.
struct CImagePayload;
struct CImagePayloadVtbl {
    char s0[0x7c];
    void(__stdcall* Apply)(CImagePayload*, void* surface); // +0x7c
};
struct CImagePayload {
    CImagePayloadVtbl* vtbl;
};

// The source provider that builds the descriptor into a CImageSource: slot 6.
struct CImageProvider;
struct CImageProviderVtbl {
    char s0[0x18];
    i32(__stdcall* BuildSource)(CImageProvider*, void* desc, CImageSource** out, i32 flag); // +0x18
};
struct CImageProvider {
    CImageProviderVtbl* vtbl;
};

// The source-info object the descriptor is seeded from (+0x10).
struct CImageInfo {
    char m_pad0[4];
    i32 m_04; // +0x04 -> descriptor +0x0c
    i32 m_08; // +0x08 -> descriptor +0x08
};

// The 0x6c-byte surface-request descriptor embedded at +0x9c.
struct CImageDesc {
    u32 size;  // +0x00 (= 0x6c)
    u32 flags; // +0x04 (= 7)
    i32 f08;   // +0x08 (= info->m_08)
    i32 f0c;   // +0x0c (= info->m_04)
    char m_pad10[0x68 - 0x10];
    u32 f68; // +0x68 (= 0x840)
};

class CImageProbe {
public:
    i32 Init();

    char m_pad00[0x10];
    CImageInfo* m_10;     // +0x10
    CImageProvider* m_14; // +0x14
    char m_pad18[0x24 - 0x18];
    CImagePayload* m_24; // +0x24
    CImageSource* m_28;  // +0x28
    void* m_2c;          // +0x2c
    char m_pad30[0x9c - 0x30];
    CImageDesc m_desc; // +0x9c .. +0x108
    char m_pad108[0x520 - 0x108];
    i32 m_520; // +0x520
};

// 0x17cbe0: build the descriptor, resolve + probe the image source, optionally
// apply it to the bound surface.
RVA(0x0017cbe0, 0x97)
i32 CImageProbe::Init() {
    memset(&m_desc, 0, 0x6c);
    m_desc.size = 0x6c;
    m_desc.flags = 7;
    m_desc.f68 = 0x840;
    m_desc.f08 = m_10->m_08;
    m_desc.f0c = m_10->m_04;
    if (m_14->vtbl->BuildSource(m_14, &m_desc, &m_28, 0) != 0) {
        return 0;
    }
    if (m_28->vtbl->Probe(m_28, &g_imageProbeTag, (void**)&m_24) != 0) {
        return 0;
    }
    if (m_520 == 8) {
        m_24->vtbl->Apply(m_24, m_2c);
    }
    return 1;
}

// class-metadata SIZE sweep (misc-Gruntz A-C): matching-neutral, hosted at
// .cpp EOF (see docs/class-metadata-sweep-log.md). SIZE_UNKNOWN = size not yet pinned.
SIZE_UNKNOWN(CImageDesc);
SIZE_UNKNOWN(CImageInfo);
SIZE_UNKNOWN(CImagePayload);
SIZE_UNKNOWN(CImagePayloadVtbl);
SIZE_UNKNOWN(CImageProbe);
SIZE_UNKNOWN(CImageProvider);
SIZE_UNKNOWN(CImageProviderVtbl);
SIZE_UNKNOWN(CImageSourceVtbl);
