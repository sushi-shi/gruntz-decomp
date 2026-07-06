// ImageProbe.cpp - the image-source probe/resolve init (RVA 0x17cbe0).
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
// as IDirectDrawSurface in CDirectDrawMgr.cpp); modeled NO-body so every dispatch
// reloc-masks. Field names are placeholders; only offsets + code bytes are
// load-bearing.
#include <rva.h>

#include <string.h> // inline memset (rep stos) for the descriptor zero

// The probe tag passed to the source's slot-0 Probe (reloc-masked .rdata datum,
// the SAME global Image.cpp names).
DATA(0x001ef888)
extern void* g_imageProbeTag; // 0x5ef888

// The engine's COM-style interfaces (vptr @ +0x00, __stdcall slots that take the
// object as the explicit first arg). Modeled as real declare-only polymorphic
// classes: cl emits no vtable (no ctor/key-fn defined, never instantiated), and
// `obj->Slot(args)` lowers to the same `mov eax,[obj]; push obj; call [eax+slot]`
// the old hand-rolled *Vtbl structs did (verified byte-exact at 0x17cbe0). The
// intervening unnamed slots are placeholder virtuals holding the slot index.

// The resolved image source: Probe at slot 0 (+0x00), Probe(magic, &out).
struct CImageSource {
    i32 __stdcall Probe(void* magic, void** out); // slot 0 == +0x00
};

// The probed payload object: Apply at slot 31 (+0x7c) hands it the bound surface.
struct CImagePayload {
    void v0();
    void v1();
    void v2();
    void v3();
    void v4();
    void v5();
    void v6();
    void v7();
    void v8();
    void v9();
    void v10();
    void v11();
    void v12();
    void v13();
    void v14();
    void v15();
    void v16();
    void v17();
    void v18();
    void v19();
    void v20();
    void v21();
    void v22();
    void v23();
    void v24();
    void v25();
    void v26();
    void v27();
    void v28();
    void v29();
    void v30();
    void __stdcall Apply(void* surface); // slot 31 == +0x7c
};

// The source provider that builds the descriptor into a CImageSource: BuildSource
// at slot 6 (+0x18).
struct CImageProvider {
    void v0();
    void v1();
    void v2();
    void v3();
    void v4();
    void v5();
    i32 __stdcall BuildSource(void* desc, CImageSource** out, i32 flag); // slot 6 == +0x18
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
    if (m_14->BuildSource(&m_desc, &m_28, 0) != 0) {
        return 0;
    }
    if (m_28->Probe(&g_imageProbeTag, (void**)&m_24) != 0) {
        return 0;
    }
    if (m_520 == 8) {
        m_24->Apply(m_2c);
    }
    return 1;
}

// class-metadata SIZE sweep (misc-Gruntz A-C): matching-neutral, hosted at
// .cpp EOF (see docs/class-metadata-sweep-log.md). SIZE_UNKNOWN = size not yet pinned.
SIZE_UNKNOWN(CImageDesc);
SIZE_UNKNOWN(CImageInfo);
SIZE_UNKNOWN(CImagePayload);
SIZE_UNKNOWN(CImageProbe);
SIZE_UNKNOWN(CImageProvider);
SIZE_UNKNOWN(CImageSource);

// --- vtable catalog (reduced-view classes share their base vtable rva) ---

// --- vtable catalog (view/base classes bound to their unit vtable rva) ---
