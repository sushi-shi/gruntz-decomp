// CInGameText.cpp - the in-game text/message display object (a CUserLogic-derived
// game-object leaf). Three __thiscall methods reconstructed in ascending-RVA
// order:
//   0x011dc0  ~CInGameText  (the bare CUserLogic teardown, /GX frame)
//   0x099460  Dispatch      (member-fn-ptr dispatch through a global zDArray)
//   0x099a30  Serialize     (chain base + the +0x34 sub-object, then +0x54/+0x58)
//
// CUserLogic / CUserBase / CGameObject come from <Gruntz/UserLogic.h>; the
// zDArray dispatch table + the archive type from the class header. The base
// SerializeChain (0x16e7f0), the zvec accessor helpers and the +0x34 sub-object
// serializer are reloc-masked engine callees (no body).
#include <Gruntz/CInGameText.h>

#include <rva.h>

#include <Bute/ButeMgr.h> // CButeTree (the registration key store)
#include <Mfc.h>          // CString (the scratch name-vec element)
#include <Gruntz/CGameRegistry.h>
#include <Globals.h>

// The zvec error globals + the capture helper the inlined accessor touches on a
// bounds miss (the same set ZVec.cpp models).
extern void* zErr_CaptureRetB(); // 0x16d990

// The shared key store + scratch name-vec the registration path populates. The
// bute tree (g_buteTree.Find/Insert) keys each registered name to a running
// index; the scratch zDArray<CString> at 0x6bf650 caches each registered name.
extern CButeTree g_buteTree;

DATA(0x0021aea8)
extern i32 g_textRegCounter; // 0x61aea8  (running registration index)

// The scratch name-vec (zDArray<CString> @ 0x6bf650): the registration path
// IndexToPtr's it (growing + CString-constructing fresh slots) to stash the key.
struct NameVec : public zDArray {};
DATA(0x002bf650)
extern NameVec g_buteNameVec; // 0x6bf650

// The "InGameText" registration key (the .data string constant at 0x60a454).
DATA(0x0020a454)
extern const char s_textLogicKey[]; // 0x60a454

// The member function the dispatch slot is loaded with (FUN_00402013, a thunk to
// a CInGameText handler). Referenced by address so its DIR32 operand reloc-masks.
extern i32 TextLogic_402013();

// The member-function-pointer the dispatch table resolves and invokes on `this`.
typedef i32 (CUserLogic::*LogicFn)();

// The zDArray<CString> accessor inlined: like ResolveSlot but with the per-slot
// CString-ctor fixup over the freshly-grown region (the zDArray::IndexToPtr body).
static inline i32 ResolveNameSlot(NameVec* v, i32 idx) {
    i32 r;
    v->m_grown = 0;
    if (idx >= v->m_lo && idx <= v->m_hi) {
        r = v->m_base + (idx - v->m_lo) * v->m_stride;
    } else if (v->GrowTo(idx, 0)) {
        r = v->m_base + (idx - v->m_lo) * v->m_stride;
    } else {
        i32 sentinel = g_zvecErrSentinel;
        g_zvecErrToken = zErr_CaptureRetB();
        v->m_err->Error(v, sentinel, 0xc);
        r = v->m_spare;
    }
    CString* slot = (CString*)v->m_alloc;
    i32 n = v->m_grown;
    while (n-- != 0) {
        if (slot) {
            slot->CString::CString();
        }
        slot++;
    }
    return r;
}

// The inline table accessor (the _zvec::IndexToPtr body, no per-slot fixup): the
// bounds-check + grow folds into Dispatch (computed per index use).
static inline i32 ResolveSlot(_zvec* v, i32 idx) {
    i32 lo = v->m_lo;
    v->m_grown = 0;
    if (idx >= lo && idx <= v->m_hi) {
        return v->m_base + (idx - lo) * v->m_stride;
    }
    if (v->GrowTo(idx, 0)) {
        return v->m_base + (idx - v->m_lo) * v->m_stride;
    }
    i32 sentinel = g_zvecErrSentinel;
    g_zvecErrToken = zErr_CaptureRetB();
    v->m_err->Error(v, sentinel, 0xc);
    return v->m_spare;
}

// ===========================================================================
// CInGameText::~CInGameText  (0x011dc0)
// ===========================================================================
// The leaf adds no destructible members, so its dtor folds the bare CUserLogic
// teardown: store the CUserLogic vptr (0x5e705c), inline-destruct the +0x18 link
// (the embedded ~EngStr call), store the CUserBase vptr (0x5e70b4). The
// destructible link forces the /GX EH frame. The empty body is enough.
RVA(0x00011dc0, 0x44)
CInGameText::~CInGameText() {}

// The global game registry (WwdGameReg, RVA 0x24556c). m_134 is the place mode
// (==2 hides at construction); m_118 the has-window gate the on-screen check
// polls. Only the touched fields are modeled; wwdfile owns the DATA label.
DATA(0x0024556c)
extern CGameRegistry* g_gameReg;

// CInGameText::CInGameText @0x099110 - fold the shared CUserLogic(obj) init, then
// (unless the registry is in the no-place mode m_134==2) bind the "A" bute node,
// the cycle geometry, the "GAME_HELPBOX" sprite name; flag the sub-object; run the
// on-screen visibility gate keyed by the bound object's place mode (m_128); and on
// the visible path snap the screen position to the tile grid + seed the +0x74
// layer key and the +0x54/+0x58 scalars to -1.
//
// @early-stop
// register-pinning/eh-ctor-vptr-restamp wall (docs/patterns/zero-register-pinning.md,
// eh-ctor-vptr-restamp-position.md): body byte-faithful (every op/offset/imm/string
// + the m_128 visibility branch tangle match retail; constant 2 pins in ebx like
// retail). Residual is the /GX leaf-vptr re-stamp position + the visibility-gate
// branch-polarity (retail emits `je visible` where structured C emits `jne hide`).
RVA(0x00099110, 0x215)
CInGameText::CInGameText(CGameObject* obj) : CUserLogic(obj) {
    if (g_gameReg->m_134 == 2) {
        m_38->m_flags |= 0x10000;
        return;
    }
    m_prevAnimSetNode = m_objAux->m_1c;
    m_objAux->m_1c = g_buteTree.Find("A");
    m_savedGeoId = m_38->m_geoId;
    m_38->ApplyLookupGeometry("GAME_CYCLE100", 0);
    m_38->ApplyName("GAME_HELPBOX");
    m_38->m_flags |= 2;

    i32 vis = m_object->m_placeMode;
    if (vis == 1) {
        if (g_gameReg->m_isEasyMode == 0) {
            m_38->m_flags |= 0x10000;
            return;
        }
        if (g_gameReg->m_134 != 1) {
            m_38->m_flags |= 0x10000;
            return;
        }
    } else if (vis == 2) {
        if (g_gameReg->m_isEasyMode != 0 && g_gameReg->m_134 == 1) {
            m_38->m_flags |= 0x10000;
            return;
        }
    }

    m_object->m_screenX = (m_object->m_screenX & ~0x1f) + 0x10;
    m_object->m_screenY = (m_object->m_screenY & ~0x1f) + 0x10;
    if (m_object->m_latchedAnimId != 0x17318) {
        m_object->m_latchedAnimId = 0x17318;
        m_object->m_flags |= 0x20000;
    }
    m_cachedAreaId = -1;
    m_cachedSubId = -1;
}

// The activation-coordinate registry view of the dispatch table (g_textDispatch
// @0x645950): InitActReg builds it over the fixed [2000, 2010] range via the
// shared registry ctor (0x408710, __thiscall ret 8).
struct CTextActReg {
    void Construct(i32 lo, i32 hi); // 0x408710
};

// CInGameText::InitActReg @0x0993e0 - construct the class's activation-coordinate
// registry (g_textDispatch @0x645950) over [2000, 2010]. Free init thunk.
RVA(0x000993e0, 0x15)
void CInGameText::InitActReg() {
    ((CTextActReg*)&g_textDispatch)->Construct(2000, 2010);
}

// ===========================================================================
// CInGameText::Dispatch  (0x099460)
// ===========================================================================
// Index the global member-fn-ptr table by `idx`; if the resolved slot holds a
// non-null member function, invoke it on `this`. The bounds-check + grow of the
// table accessor is inlined (the _zvec::IndexToPtr body, no out-of-line call),
// computed once for the null-test and once for the call.
RVA(0x00099460, 0x102)
void CInGameText::Dispatch(i32 idx) {
    if (*(void**)ResolveSlot(&g_textDispatch, idx) != 0) {
        LogicFn fn = *(LogicFn*)ResolveSlot(&g_textDispatch, idx);
        (this->*fn)();
    }
}

// ===========================================================================
// RegisterTextLogic  (0x0995c0)
// ===========================================================================
// The file-scope static registration thunk for the text-logic handler: look the
// key up in the bute tree; if absent, Insert it under the running counter and
// cache the key name into the scratch zDArray<CString> slot (growing it), then
// bump the counter. Either way, resolve the dispatch-table slot for the key index
// and load it with the handler member-fn-ptr (FUN_00402013).
// ---------------------------------------------------------------------------
// @early-stop
// inlined zDArray/zvec IndexToPtr regalloc wall (the documented ZVec family - see
// ZVec.cpp's IndexToPtr/GrowTo @early-stops, ~80%): the two inlined accessors +
// the CString-ctor fixup loop are reconstructed faithfully, but cl pins the
// index/this/base across the grow branches differently than retail and permutes
// the two-block offset tails. Logic + the bute find/insert + the fn-ptr store are
// correct; the register assignment is not source-steerable.
RVA(0x000995c0, 0x18d)
void RegisterTextLogic() {
    i32 idx = (i32)g_buteTree.Find(s_textLogicKey);
    if (idx == 0) {
        g_buteTree.Insert(s_textLogicKey, (void*)g_textRegCounter);
        i32 slot = ResolveNameSlot(&g_buteNameVec, g_textRegCounter);
        *(CString*)slot = s_textLogicKey;
        g_textRegCounter++;
    }
    i32 dslot = ResolveSlot(&g_textDispatch, idx);
    *(void**)dslot = (void*)&TextLogic_402013;
}

// ===========================================================================
// CInGameText::Serialize  (0x099a30)
// ===========================================================================
// Guard on the archive, chain the shared CUserLogic::SerializeChain, then the
// +0x34 sub-object's own serializer, then round-trip the two own dwords at
// +0x54/+0x58: tag 4 stores (archive Write), tag 7 loads (archive Read).
RVA(0x00099a30, 0xaa)
i32 CInGameText::Serialize(CTextArchive* ar, i32 tag, i32 a, i32 b) {
    if (ar == 0) {
        return 0;
    }
    if (SerializeChain((i32)ar, tag, a, b) == 0) {
        return 0;
    }
    if (((CSerialObjRef*)&m_34)->Chain((CSerialArchive*)ar, tag, a, (CSerialObj*)b) == 0) {
        return 0;
    }
    switch (tag) {
        case 4:
            ar->Write(&m_cachedAreaId, 4);
            ar->Write(&m_cachedSubId, 4);
            break;
        case 7:
            ar->Read(&m_cachedAreaId, 4);
            ar->Read(&m_cachedSubId, 4);
            break;
    }
    return 1;
}

// class-metadata SIZE sweep (misc-Gruntz A-C): matching-neutral, hosted at
// .cpp EOF (see docs/class-metadata-sweep-log.md). SIZE_UNKNOWN = size not yet pinned.
SIZE_UNKNOWN(CInGameText);
SIZE_UNKNOWN(CTextActReg);
SIZE_UNKNOWN(CTextArchive);
SIZE_UNKNOWN(CGameRegistry);
