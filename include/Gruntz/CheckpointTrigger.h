#ifndef GRUNTZ_CCHECKPOINTTRIGGER_H
#define GRUNTZ_CCHECKPOINTTRIGGER_H

#include <rva.h>
#include <Gruntz/UserLogic.h>

class CCheckpointTrigger : public CUserLogic, public CWapX {
public:
    virtual i32 SerializeMove(CGruntArchive*, i32, i32, i32) OVERRIDE; // slot 1
    virtual LogicTypeId GetTypeTag() OVERRIDE;                         // slot 2
public:
    CCheckpointTrigger(CGameObject* obj);   // 0x10ee20 (1-arg leaf ctor)
    // NO user-declared dtor: retail's is COMPILER-GENERATED (implicit
    // elides the leaf-vptr restamp; @rva-symbol pin in the home TU).
    // Act cluster RE-ATTRIBUTED (the shift-by-one): this class owns the 0x10f160/
    // 0x10f1e0/0x10f340 cluster (RTTI: its slot 4 override -> ILT 0x001366 -> jmp
    // 0x10f1e0); the 0x10ea00.. cluster it used to claim is CBrickz's. Bodies in
    // TileLogicPump.cpp.
    static void InitActReg();                     // 0x10f160 (constructs g_checkpointActReg @0x64e7e8)
    virtual void FireActivation(i32 id) OVERRIDE; // 0x10f1e0 (vtable slot 4 body)
    static void RegisterActs();                   // 0x10f340 (binds the "A"/"B" activation handlers)
    i32 Act(); // 0x10f6a0 ("A" handler; @stub body pending leaf-first reconstruction)
    i32 Act_10f970(); // 0x10f970 ("B" handler; declared-only, used as a PMF)
    i32 m_state[15];           // +0x54  the captured checkpoint state (15 dwords)
    i32 m_firstEmpty;          // +0x90  first-empty index
};
VTBL(CCheckpointTrigger, 0x1e7ebc);
SIZE(CCheckpointTrigger, 0x94);

struct CCheckpointActEntry {
    i32 (CUserLogic::*m_fn)();
};

#endif // GRUNTZ_CCHECKPOINTTRIGGER_H
