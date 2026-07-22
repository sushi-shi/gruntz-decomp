#ifndef GRUNTZ_GRUNTZ_CINGAMETEXT_H
#define GRUNTZ_GRUNTZ_CINGAMETEXT_H

#include <rva.h>

#include <Mfc.h> // CObject base + <windows.h>

#include <Gruntz/UserLogic.h>    // CUserLogic : CUserBase, CGameObject
#include <Wap32/ZVec.h>          // zDArray<member-fn-ptr> (the dispatch table)
#include <Gruntz/SerialArchive.h> // CSerialArchive (the inherited CWapX::Chain arg; ex SerialObjRef.h)

class CFileMemBase;
typedef CFileMemBase CSerialArchive;

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
SIZE_UNKNOWN();

struct CActReg;
extern CActReg g_textDispatch; // 0x245950 (registry archetype; zDArray<T> instantiation)

#endif // GRUNTZ_GRUNTZ_CINGAMETEXT_H
