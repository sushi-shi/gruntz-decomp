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

// (ListNodeAdvance @0x029a30 re-homed to src/Gruntz/BattlezMapConfig.cpp - a list
// iterator advance, RVA-contiguous with that TU's 0x24dc0-0x358a0 .text block.)

// (QuadIntRecord @0x029ac0 re-homed to src/Wap32/Rect.cpp as CRect::CRect(i32,i32,
// i32,i32) - Ghidra/FID attests ??0CRect@@QAE@HHHH@Z for this direct-store 4-int
// ctor; the "generic 4-int record" was the engine CRect. The m_0..m_c fields are
// tagRECT's left/top/right/bottom.)

// (0x15b2b0 Obj15b2b0 + 0x15b270 Obj15b270 - the CWwdGameObject embedded sub-object
// ctors - re-homed to src/DDrawMgr/DDrawSubMgr.cpp, next to their sibling CWwdSlot9c
// (0x15b2a0) built by the same CWwdObjMgr::CreateObject factory cluster.)

// (DualBufferOwner::FreeBuffers @0x148d10 re-homed to src/Image/ImageOwned.cpp as
// CDDrawShadeBlit::Teardown - the +0x30 owned shaded sprite of CImage; xref-proven
// via CImage::FreeAll's owned->Teardown() call, fields m_rleData/m_palette.)

// (0x1591b0 WapObjBase::BaseInit - the wap-object base re-init - re-homed to
// src/DDrawMgr/DDrawSubMgrPages.cpp (its RVA neighborhood).)

// (0x1397a0 Obj1397a0::Teardown - the Bute-symbol payload teardown - re-homed to
// src/Bute/SymTab.cpp (its RVA neighborhood, next to CSymLeafBuilder::Build); the
// Obj49Target/Obj1397a0 placeholder views moved with it.)

// (FirstDiffBit @0x16e480 re-homed to src/Bute/ButeTree.cpp - the crit-bit index
// helper CButeTree::Insert + CProjActMap::Insert both call; ButeTree.cpp already
// declared it reloc-masked, so the definition lands in the canonical trie TU.)

// (CU35Host::DestroyStr @0x021c40 re-homed to src/Gruntz/FontConfig.cpp as
// FontItem::~FontItem - the out-of-line dtor of CFontConfig's {type,data,CString name}
// list record; xref-proven via CFontConfig::FreeNodes/Scroll deleting FontItem.)
