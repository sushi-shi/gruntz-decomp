// CInGameText.h - the in-game text/message display object, a CUserLogic-derived
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

#include <Gruntz/UserLogic.h>     // CUserLogic : CUserBase, CGameObject
#include <Wap32/ZVec.h>           // zDArray<member-fn-ptr> (the dispatch table)
#include <Gruntz/CSerialObjRef.h> // the shared +0x34 serialized-object-reference (Chain @0x8c00)

// ---------------------------------------------------------------------------
// The engine serialization stream the leaf Serialize round-trips through. A
// vtable-dispatched archive: slot +0x2c = Read(buf,n), slot +0x30 = Write(buf,n)
// (the same convention as the SBI/MenuItem archives). External; no body.
// ---------------------------------------------------------------------------
class CTextArchive {
public:
    virtual void Slot00();
    virtual void Slot04();
    virtual void Slot08();
    virtual void Slot0C();
    virtual void Slot10();
    virtual void Slot14();
    virtual void Slot18();
    virtual void Slot1C();
    virtual void Slot20();
    virtual void Slot24();
    virtual void Slot28();
    virtual void Read(void* buf, i32 n);  // +0x2c
    virtual void Write(void* buf, i32 n); // +0x30
};

// The embedded serializable sub-object overlaid at CInGameText+0x34 is the shared
// CSerialObjRef (Chain @0x8c00 via the 0x1aff thunk); reached as
// `lea ecx,[this+0x34]; call`. Modeled by <Gruntz/CSerialObjRef.h> above.

// ---------------------------------------------------------------------------
// CInGameText : CUserLogic. Its own state begins at +0x54 (within the inherited
// layout the dtor folds the bare CUserLogic teardown).
// ---------------------------------------------------------------------------
class CInGameText : public CUserLogic {
public:
    CInGameText(CGameObject* obj);   // 0x099110 (folds CUserLogic(obj) + on-screen tail)
    virtual ~CInGameText() OVERRIDE; // 0x011dc0

    static void InitActReg();                               // 0x0993e0
    void Dispatch(i32 idx);                                 // 0x099460
    i32 Update();                                           // 0x0997c0
    i32 Serialize(CTextArchive* ar, i32 tag, i32 a, i32 b); // 0x099a30

    // --- CInGameText own fields (offsets load-bearing) ---
    i32 m_savedGeoId;          // +0x40  saved m_38->m_1b4 geometry id (before GAME_CYCLE100)
    char m_pad44[0x54 - 0x44]; // +0x44..+0x53 (inherited tail / own scratch)
    i32 m_cachedAreaId;        // +0x54  Update: cached hit-test area id; serialized scalar
    i32 m_cachedSubId;         // +0x58  Update: cached hit-test sub id; serialized scalar
};

// ---------------------------------------------------------------------------
// The global member-fn-ptr dispatch table (a zDArray<int (CUserLogic::*)(void)>
// at VA 0x645950 / RVA 0x245950). Dispatch indexes it and invokes the resolved
// member function on `this`. The accessor inlines the bounds-check + grow.
// ---------------------------------------------------------------------------
DATA(0x00245950)
extern zDArray g_textDispatch;

#endif // GRUNTZ_GRUNTZ_CINGAMETEXT_H
