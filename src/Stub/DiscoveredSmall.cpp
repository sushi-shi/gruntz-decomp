// DiscoveredSmall.cpp - trace-discovered small leaf methods re-homed from
// src/Stub/Discovered.cpp (matcher-1). Each class here is a trace-discovered
// placeholder whose true identity is unrecovered; only the OFFSETS + code bytes
// are load-bearing. These are the self-contained (no/one-reloc) leaves: simple
// ctors / inits / free-list ops / a key-compare helper.
#include <Mfc.h> // real MFC CString (embedded name member; ~CString @0x1b9cde)
#include <Ints.h>
#include <Wap32/Object.h> // CObject grand-base (real virtual dtor)
#include <rva.h>

// The engine __cdecl deallocator (operator delete; reloc-masked rel32). 0x1b9b82.
extern "C" void RezFree(void* p);

// The wap-object teardown grand-base vtable (0x5e8cb4); stamped by address.
// (FreeNodePool::Push @0x0311b0 re-homed to src/Gruntz/FreeNodePool.cpp.)

// ---------------------------------------------------------------------------
// ListNodeAdvance @0x029a30 - a list iterator advance: read the current node
// (*it), step the cursor to its link (*cur), return a pointer into the current
// node's payload (cur+8). __stdcall, 1 stack arg.
// ---------------------------------------------------------------------------
RVA(0x00029a30, 0x10)
void* __stdcall ListNodeAdvance(void** it) {
    char* cur = (char*)*it;
    *it = *(void**)cur;
    return cur + 8;
}

// ---------------------------------------------------------------------------
// QuadIntRecord @0x029ac0 - 4-field initializer (ctor returning this).
// __thiscall, 4 stack args, ret 0x10.
// @orphan: identity unrecovered - a generic 4-int record ctor called broadly across
// CBattlezMapConfig / CGrunt / CArriveMgr / FontRenderer (xref); no single owning TU.
// ---------------------------------------------------------------------------
SIZE_UNKNOWN(QuadIntRecord);
class QuadIntRecord {
public:
    QuadIntRecord(i32 a, i32 b, i32 c, i32 d);
    i32 m_0;
    i32 m_4;
    i32 m_8;
    i32 m_c;
};
RVA(0x00029ac0, 0x20)
QuadIntRecord::QuadIntRecord(i32 a, i32 b, i32 c, i32 d) {
    m_0 = a;
    m_4 = b;
    m_8 = c;
    m_c = d;
}

// ---------------------------------------------------------------------------
// Obj15b2b0 @0x15b2b0 - zero three fields (ctor returning this).
// @orphan: a CWwdGameObject subclass new'd by CWwdObjMgr::CreateObject_159600 (xref),
// but the concrete game-object class carries no recoverable RTTI name (hex identity).
// ---------------------------------------------------------------------------
SIZE_UNKNOWN(Obj15b2b0);
class Obj15b2b0 {
public:
    Obj15b2b0();
    char m_pad0[0x8];
    i32 m_8;
    i32 m_c;
    char m_pad10[0x18 - 0x10];
    i32 m_18;
};
RVA(0x0015b2b0, 0xe)
Obj15b2b0::Obj15b2b0() {
    m_c = 0;
    m_8 = 0;
    m_18 = 0;
}

// ---------------------------------------------------------------------------
// Obj15b270 @0x15b270 - seed two fields (ctor returning this).
// @orphan: a CWwdGameObject subclass new'd by CWwdObjMgr::CreateObject_159250/440/600
// (xref); concrete game-object class has no recoverable RTTI name (hex identity).
// ---------------------------------------------------------------------------
SIZE_UNKNOWN(Obj15b270);
class Obj15b270 {
public:
    Obj15b270();
    char m_pad0[0x8];
    i32 m_8;
    char m_pad0c[0x20 - 0xc];
    i32 m_20;
};
RVA(0x0015b270, 0x11)
Obj15b270::Obj15b270() {
    m_8 = (i32)0x80000000;
    m_20 = -1;
}

// (DualBufferOwner::FreeBuffers @0x148d10 re-homed to src/Image/ImageOwned.cpp as
// CDDrawShadeBlit::Teardown - the +0x30 owned shaded sprite of CImage; xref-proven
// via CImage::FreeAll's owned->Teardown() call, fields m_rleData/m_palette.)

// ---------------------------------------------------------------------------
// WapObjBase @0x1591b0 - wap-object base init: seed m_4=-1, zero m_8/m_c/m_10,
// stamp the grand-base dtor vptr. A void METHOD (keeps this in ecx, eax=0; no
// mov eax,ecx) - see vptr-stamp-void-init-not-ctor.
// TERMINAL manual stamp (not convertible to `: public CObject`): this is a
// standalone void re-init method, not a ctor, so the store IS retail's own body -
// cl's auto-stamp only lands in a ctor. Identity is a placeholder besides.
// @orphan: only caller is an UNMATCHED scalar-deleting-destructor @0x159190; no RTTI
// name - owning wap-object class identity unrecoverable.
// ---------------------------------------------------------------------------
SIZE_UNKNOWN(WapObjBase);
class WapObjBase : public CObject {
public:
    void BaseInit();
    i32 m_4;
    i32 m_8;
    i32 m_c;
    i32 m_10;
};
RVA(0x001591b0, 0x19)
void WapObjBase::BaseInit() {
    m_4 = -1;
    m_10 = 0;
    m_8 = 0;
    m_c = 0;
    // base vptr auto-stamped via CObject (retail's manual stamp dropped, % ok)
}

// ---------------------------------------------------------------------------
// Obj1397a0 @0x1397a0 - teardown: free m_0; then free m_38 unless the
// m_10 target is live (m_10 && m_10->m_48 != 0); then clear nine fields.
// __thiscall, void. (The two `if (m_38) free` arms tail-merge to one call.)
// @orphan: a Bute-symbol payload torn down from ~CSymRec via CSymListNode::m_14, but it
// is NOT CSymRec (accesses +0x38, past CSymRec's size 0x30); identity unrecovered and
// the Bute CSymRec/CSymList models still diverge - do not fold here.
// ---------------------------------------------------------------------------
struct Obj49Target {
    char m_pad[0x48];
    i32 m_48;
};
SIZE_UNKNOWN(Obj1397a0);
class Obj1397a0 {
public:
    void Teardown();
    void* m_0;
    i32 m_4;
    i32 m_8;
    i32 m_c;
    Obj49Target* m_10;
    i32 m_14;
    i32 m_18;
    char m_pad1c[0x30 - 0x1c];
    i32 m_30;
    char m_pad34[0x38 - 0x34];
    void* m_38;
};
RVA(0x001397a0, 0x57)
void Obj1397a0::Teardown() {
    if (m_0) {
        RezFree(m_0);
    }
    if (m_10 != 0) {
        if (m_10->m_48 == 0) {
            if (m_38) {
                RezFree(m_38);
            }
        }
    } else {
        if (m_38) {
            RezFree(m_38);
        }
    }
    m_0 = 0;
    m_4 = 0;
    m_8 = 0;
    m_c = 0;
    m_38 = 0;
    m_10 = 0;
    m_14 = 0;
    m_18 = 0;
    m_30 = 0;
}

// (FirstDiffBit @0x16e480 re-homed to src/Bute/ButeTree.cpp - the crit-bit index
// helper CButeTree::Insert + CProjActMap::Insert both call; ButeTree.cpp already
// declared it reloc-masked, so the definition lands in the canonical trie TU.)

// (CU35Host::DestroyStr @0x021c40 re-homed to src/Gruntz/FontConfig.cpp as
// FontItem::~FontItem - the out-of-line dtor of CFontConfig's {type,data,CString name}
// list record; xref-proven via CFontConfig::FreeNodes/Scroll deleting FontItem.)

SIZE_UNKNOWN(Obj49Target);
