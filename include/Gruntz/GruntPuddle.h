#ifndef GRUNTZ_GRUNTZ_CGRUNTPUDDLE_H
#define GRUNTZ_GRUNTZ_CGRUNTPUDDLE_H

#include <rva.h>

#include <Mfc.h> // CObject base + <windows.h>

#include <Gruntz/ActReg.h>    // CLogicActTable (the slot-4 activation-dispatch table)
#include <Gruntz/UserLogic.h> // CUserLogic : CUserBase, EngStr, CGameObject

class CFileMemBase;
typedef CFileMemBase CSerialArchive;

struct CObjListNode {
    CObjListNode* m_next; // +0x00
    CObjListNode* m_prev; // +0x04
    void* m_data;         // +0x08
};
SIZE_UNKNOWN();
struct CObjList {
    char m_pad00[0x4];
    CObjListNode* m_head;             // +0x04  list head
    void RemoveAt(CObjListNode* pos); // 0x1b4ac7 (__thiscall, unlink + free node)
};
SIZE_UNKNOWN();
SIZE_UNKNOWN(); // {vptr,head,tail}=0xc header; full engine size unproven

struct CGruntPuddleSink {};
SIZE_UNKNOWN();

extern "C" u32 g_engineFrameDelta;

extern char g_puddleSpriteKey[]; // s_..._0060c1c0

class CGruntPuddle : public CUserLogic, public CWapX {
public:
    virtual i32 SerializeMove(CGruntArchive*, i32, i32, i32) OVERRIDE; // slot 1
    RVA(0x00010cc0, 0x6)
    virtual LogicTypeId GetTypeTag() OVERRIDE {
        return LOGIC_GRUNTPUDDLE;
    } // slot 2
public:
    CGruntPuddle(CGameObject* obj);   // 0x040490
    // NO user-declared dtor: retail's is COMPILER-GENERATED (implicit
    // elides the leaf-vptr restamp; @rva-symbol pin in the home TU).

    i32 Place(i32 a0, i32 a1, i32 a2, i32 a3); // 0x040c30
    i32 Remove();                              // 0x040d20
    void SetBute(char* key);                   // 0x07d810
    // FireActivation (0x40750): slot-4 (UserLogicVfunc2) override - resolve `id` in
    // the class dispatch table g_logicDispatch_6445e8; if the resolved entry holds a
    // handler, re-resolve and dispatch it __thiscall on `this`. Same archetype as
    // CTeleporter::FireActivation.
    virtual void FireActivation(i32 id) OVERRIDE;
    // Serialize (0x40e50): two-chain (CUserLogic base + the +0x34 sub-object) then a
    // tag-dispatched field round-trip - tag 4 writes / tag 7 reads the 7 own i32
    // fields via the archive vtable, tag 8 re-resolves the placed sprite from the
    // game registry. Same archetype as CGruntHealthSprite::Serialize.

    // --- CGruntPuddle own fields (placeholders; offsets load-bearing) ---
    // (This class is ALSO the CTriggerMgr::m_baseList element the spawn/resurrect
    //  scans walk - the ex "CTmCandidate" view, folded 2026-07-16; identity proof
    //  in <Gruntz/TriggerMgr.h>.)
    i32 m_tileX;      // +0x54  owner tile X (m_object->m_screenX >> 5)
    i32 m_tileY;      // +0x58  owner tile Y (m_object->m_screenY >> 5)
    i32 m_pending;    // +0x5c  not-yet-placed gate (ctor 1; cleared once placed;
                      //         the spawn/resurrect scans skip a nonzero one)
    i32 m_placed;     // +0x60  "placed" flag
    i32 m_placeArg3;  // +0x64  Place() arg3 snapshot
    i32 m_gruntType;  // +0x68  the dead owner's grunt-type index (Place() a0 snapshot;
                      //         the trigger-mgr resurrect re-creates via PlaceObject(type,..))
    i32 m_placeIndex; // +0x6c  selector/icon index (Place() a1 snapshot; GetSel draws by
                      //         it, the resurrect passes it through as PlaceObject a6)
};
SIZE_UNKNOWN();

extern CLogicActTable g_logicDispatch_6445e8;

typedef i32 (CUserLogic::*PuddleActHandler)();
struct CPuddleActEntry {
    PuddleActHandler m_fn;
};
SIZE_UNKNOWN(); // only the first dword (the handler) is modeled

#endif // GRUNTZ_GRUNTZ_CGRUNTPUDDLE_H
