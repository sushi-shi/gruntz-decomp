#include <rva.h>
// CButeNodeBase.cpp - CButeNodeBase, a ButeMgr config-tree node base.
//
// CButeNodeBase derives from CContainerErr (the container-library exception
// base, ctor @0x16d9c0, modeled in GameText) and embeds a small node subobject
// at +0x8 (the keyed-store entry: a ptr + a 16-bit kind + a vtbl). The ctor
// runs the CContainerErr base ctor (passing the node's error-message global),
// constructs the +0x8 subobject from (n, desc), then re-stamps the two derived
// vtables (the subobject at +0x8 and the whole object at +0x0) and zeroes the
// two child-link fields at +0x18 / +0x28.
//
// This ctor carries a C++ /GX EH frame (push -1 / push handler / fs:0 save) for
// the subobject construction. The engine_label_stubs unit is flags="base" (no
// /GX), so the EH frame is elided here -> the body+offsets match but the frame
// is missing; documented base-flags EH wall (docs/patterns/gx-frame-destructible
// -local.md). The logic below is the complete, correct reconstruction.
//
// Field names are placeholders; only OFFSETS + code bytes are load-bearing.

// CContainerErr - the container-library exception base (vtbl@0, msg@4). Its
// ctor (RVA 0x16d9c0, real body in GameText) is __thiscall(this, msg); declared
// no-body here so the `push msg; call` shape is reloc-masked.
extern void* g_buteNodeErrMsg; // DAT_006bf480 - the node's error-message global

class CContainerErr {
public:
    CContainerErr(void* msg);

    void* m_vtbl; // +0x00
    void* m_msg;  // +0x04
};

// The two derived vtables re-stamped by the ctor. Reloc-masked file-scope
// addresses (the most-derived vtbl for the +0x8 subobject, and for the whole
// object). External, no-body.
extern void* g_buteNodeSubVtbl; // 0x5e949c -> stamped at this+0x8
extern void* g_buteNodeVtbl;    // 0x5e94ac -> stamped at this+0x0

// The embedded node subobject at CButeNodeBase+0x8: a small keyed-store entry.
// Its ctor (RVA 0x16df70) is __thiscall(this, int n, void* desc): stores desc at
// +0x4, (WORD)n at +0x8, 0 at +0xc, and its own base vtbl at +0x0. Declared
// no-body so the `mov ecx; call` __thiscall shape is reloc-masked.
class CButeNodeEntry {
public:
    CButeNodeEntry(int n, void* desc);

    void* m_vtbl; // +0x00
    void* m_4;    // +0x04  desc
    short m_8;    // +0x08  (WORD)n
    char m_pada[2];
    int m_c; // +0x0c
};

// CButeNodeBase layout:
//   +0x00  CContainerErr base (vtbl, msg)
//   +0x08  CButeNodeEntry subobject (vtbl, desc, kind, 0) - spans +0x08..+0x18
//   +0x18  m_18 : child link, zeroed
//   +0x28  m_28 : child link, zeroed
class CButeNodeBase : public CContainerErr {
public:
    CButeNodeBase(void* desc, int n);

    CButeNodeEntry m_entry; // +0x08 (0x10 bytes -> ends at +0x18)
    int m_18;               // +0x18
    char m_pad1c[0x28 - 0x1c];
    int m_28; // +0x28
};

// @early-stop
// base-flags EH wall: this ctor needs a /GX EH frame (subobject construction
// under unwind) but engine_label_stubs is flags="base" -> the frame is elided.
// Body+offsets are byte-identical; only the EH frame is missing. See
// docs/patterns/gx-frame-destructible-local.md. Defer to the final sweep / an
// eh unit.
RVA(0x0016dff0, 0x73)
CButeNodeBase::CButeNodeBase(void* desc, int n)
    : CContainerErr(&g_buteNodeErrMsg), m_entry(n, desc) {
    m_entry.m_vtbl = &g_buteNodeSubVtbl;
    m_18 = 0;
    m_28 = 0;
    m_vtbl = &g_buteNodeVtbl;
}
