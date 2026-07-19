// InGameText.h - the in-game text/message display object, a CUserLogic-derived
// game-object leaf (vftables 0x5e705c / 0x5e70b4, the CUserLogic / CUserBase
// pair - the same shape every UserLogic leaf carries). Recovered from the three
// trace-discovered methods:
//   0x011dc0  ~CInGameText  (the bare CUserLogic teardown, /GX frame)
//   0x099460  Dispatch      (member-fn-ptr dispatch through a global zDArray)
//   0x099a30  Serialize     (chain base + the +0x34 sub-object, then +0x54/+0x58)
//
// Field names are placeholders (m_<hexoffset>); only OFFSETS + code bytes are
// load-bearing (campaign doctrine). The own state begins at +0x54 (CUserLogic
// ends at +0x40); Serialize round-trips two dwords at +0x54/+0x58.
#ifndef GRUNTZ_GRUNTZ_CINGAMETEXT_H
#define GRUNTZ_GRUNTZ_CINGAMETEXT_H

#include <rva.h>

#include <Mfc.h> // CObject base + <windows.h>

#include <Gruntz/UserLogic.h>    // CUserLogic : CUserBase, CGameObject
#include <Wap32/ZVec.h>          // zDArray<member-fn-ptr> (the dispatch table)
#include <Gruntz/SerialArchive.h> // CSerialArchive (the inherited CWapX::Chain arg; ex SerialObjRef.h)

// The serialize stream: the REAL CFileMemBase (<Gruntz/SerialArchive.h> typedefs
// CSerialArchive onto it). Pointer-only here, so the fwd decl + typedef suffice;
// an elaborated `struct CSerialArchive*` would re-declare a DISTINCT class and
// silently out-rank the typedef (MSVC5).
class CFileMemBase;
typedef CFileMemBase CSerialArchive;

// The engine serialization stream the leaf Serialize round-trips through is the shared
// WAP32 CSerialArchive (Read @ vtable +0x2c / Write @ +0x30), pulled in via
// <Gruntz/SerialArchive.h> above - the former local `CTextArchive` view is folded away.

// The embedded serializable sub-object overlaid at CInGameText+0x34 is the shared
// CSerialObjRef (Chain @0x8c00 via the 0x1aff thunk); reached as
// `lea ecx,[this+0x34]; call`. Modeled by <Gruntz/SerialArchive.h> above.

// ---------------------------------------------------------------------------
// CInGameText : CUserLogic. Its own state begins at +0x54 (within the inherited
// layout the dtor folds the bare CUserLogic teardown).
// ---------------------------------------------------------------------------
class CInGameText : public CUserLogic, public CWapX {
public:
    virtual i32 SerializeMove(CGruntArchive*, i32, i32, i32) OVERRIDE; // slot 1
    RVA(0x00011d70, 0x6)
    virtual LogicTypeId GetTypeTag() OVERRIDE {
        return LOGIC_INGAMETEXT;
    } // slot 2
public:
    CInGameText(CGameObject* obj);   // 0x099110 (folds CUserLogic(obj) + on-screen tail)
    // NO user-declared dtor: retail's is COMPILER-GENERATED (implicit
    // elides the leaf-vptr restamp; @rva-symbol pin in the home TU).

    static void InitActReg();                     // 0x0993e0
    virtual void FireActivation(i32 id) OVERRIDE; // 0x099460
    i32 Update();                                 // 0x0997c0

    // --- CInGameText own fields (offsets load-bearing) ---
    i32 m_cachedAreaId;        // +0x54  Update: cached hit-test area id; serialized scalar
    i32 m_cachedSubId;         // +0x58  Update: cached hit-test sub id; serialized scalar
};
VTBL(CInGameText, 0x1e7cac);

// ---------------------------------------------------------------------------
// The global member-fn-ptr dispatch table (a zDArray<int (CUserLogic::*)(void)>
// at VA 0x645950 / RVA 0x245950). Dispatch indexes it and invokes the resolved
// member function on `this`. The accessor inlines the bounds-check + grow.
// ---------------------------------------------------------------------------
// The DATA binding lives in InGameIcon.cpp (a header DATA() is not scanned).
struct CActReg;
extern CActReg g_textDispatch; // 0x245950 (registry archetype; zDArray<T> instantiation)

#endif // GRUNTZ_GRUNTZ_CINGAMETEXT_H
