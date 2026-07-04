// ComHelperThunks.cpp - small leaf glue from the engine's COM/registry-helper
// module (the 0x1bf*-0x1d5* run): a vptr-stamping base dtor and two one-line
// forwarders onto file-scope engine singletons. All callees/globals are external
// symbols (reloc-masked rel32/DIR32); only the instruction selection is load-bearing.
//
// The detach getter (0x1c0a41, `and [ecx],0`) and the three static-destructor
// atexit-registration thunks (0x1ce973 / 0x1d4239 / 0x1d4c16) had no plain-C++
// spelling (they were __declspec(naked) bodies), so they are carved to
// config/library_labels.csv (asm-carveout: GAME-ASM / MSVC-THUNK) instead.
#include <rva.h>

// File-scope framework singletons the forwarders construct at static-init (both
// forwarders are reached from CRT static-init / EH-funclet callers, i.e. they run
// a ctor on a framework global).

// unidentified: g_com652f00 (VA 0x652f00) is an MFC-framework global constructed at
// static-init; its ctor 0x1baf15 FID/FLIRT-matches a CWnd-family method (Ghidra
// labels it CWnd) but the exact global identity and the int arg (Activate(1)) are
// unresolved - kept as an opaque forward view. (MFC-library territory: not a game
// class; per matcher policy, use the lib rather than hand-reconstruct it.)
struct ComSingleton2f00 {
    void Activate(int on); // 0x1baf15  (really a CWnd-family ctor; "Activate" is a guess)
};
DATA(0x00252f00)
extern ComSingleton2f00 g_com652f00; // VA 0x652f00

// MFC's global afxData (the single AUX_DATA instance): 0x1d4c7e is AUX_DATA::AUX_DATA
// (GetVersion + LoadCursorA stock-object cache - the textbook AUX_DATA ctor), run
// here at static-init. Opaque forward view (real AUX_DATA is a ~155-byte MFC struct).
struct AUX_DATA {
    void Construct(); // 0x1d4c7e  AUX_DATA::AUX_DATA (ctor)
};
DATA(0x00253210)
extern AUX_DATA g_afxData; // VA 0x653210  (MFC ?afxData)

// A base subobject whose vptr-restore stamps the vtable pointer. Real polymorphic:
// the plain (non-deleting) dtor of a class with a virtual dtor restores the vptr at
// entry, emitting `mov [ecx], offset ??_7CNoTrackObject@@6B@; ret` - byte-identical
// to the retail 7-byte stamp (a ctor would add a return-this `mov eax,ecx`). ??_7 at
// 0x1ec26c (config/vtable_names.csv); the scalar-dtor slot reloc-masks.
struct CNoTrackObject {
    virtual ~CNoTrackObject(); // 0x1d4722
};

// ---------------------------------------------------------------------------
// 0x1d4722 - the vptr-restore: cl stamps the ??_7CNoTrackObject vptr, then ret.
// ---------------------------------------------------------------------------
RVA(0x001d4722, 0x7)
CNoTrackObject::~CNoTrackObject() {}

// ---------------------------------------------------------------------------
// 0x1d4c0c - static-init: construct MFC's global afxData (AUX_DATA::AUX_DATA).
// ---------------------------------------------------------------------------
RVA(0x001d4c0c, 0xa)
void ComPump3210() {
    g_afxData.Construct();
}

// ---------------------------------------------------------------------------
// 0x1bae1f - static-init: construct the CWnd-family framework global @0x652f00.
// ---------------------------------------------------------------------------
RVA(0x001bae1f, 0xd)
void ComActivate2f00() {
    g_com652f00.Activate(1);
}

// Class metadata (hosted at .cpp EOF).
SIZE_UNKNOWN(ComSingleton2f00); // opaque MFC-framework global (unidentified, see above)
SIZE_UNKNOWN(AUX_DATA);         // MFC afxData / AUX_DATA (opaque forward view)
SIZE(CNoTrackObject, 0x4);      // real MFC CNoTrackObject: vptr only (??_7 at 0x1ec26c)
