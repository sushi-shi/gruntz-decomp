#ifndef GRUNTZ_CDROPPEDOBJECT_H
#define GRUNTZ_CDROPPEDOBJECT_H

#include <rva.h>
#include <Gruntz/UserLogic.h> // CUserLogic base (CDroppedObject : CUserLogic)

class CFileMemBase;
typedef CFileMemBase CSerialArchive;

class CDroppedObject : public CUserLogic, public CWapX {
public:
    virtual i32 SerializeMove(CGruntArchive*, i32, i32, i32) OVERRIDE; // slot 1
    RVA(0x00012560, 0x6)
    virtual LogicTypeId GetTypeTag() OVERRIDE {
        return LOGIC_DROPPEDOBJECT;
    } // slot 2
    virtual i32 UserLogicVfunc5() OVERRIDE; // slot 7
public:
    CDroppedObject(CGameObject* obj); // 0x0c68b0 (1-arg leaf ctor)
    static void RegisterRange();      // 0x0c6b50 (seed the activation table's fast range)
    static void RegisterActs();       // 0x0c6d30
    virtual void FireActivation(i32 id) OVERRIDE; // 0x0c6bd0
    i32 ActA();                                   // 0x0c7090 (per-frame "A" activation handler)
    // NO user-declared dtor: retail's is COMPILER-GENERATED (implicit
    // elides the leaf-vptr restamp; RVA_COMPGEN pin in the home TU).
    // The slot-1 serialize impl (plain method: ?Serialize name + RVA pin, vtable
    // slot reloc-masked, like CRollingBall::Serialize).
    char m_pad54[0x58 - 0x54];
    double m_timePerTile; // +0x58  per-tile time (32.0 / TimePerTile)
    double m_fallY;       // +0x60  fall accumulator (adjusted screen Y)
    i32 m_landY;          // +0x68  landing row (pre-offset screen Y)
};
SIZE_UNKNOWN();

typedef void (CUserLogic::*DropHandler)();
struct CDropEntry {
    DropHandler m_fn; // [entry]
};
SIZE_UNKNOWN();

#include <Gruntz/ActReg.h> // CSiblingActReg (extern below)
extern CSiblingActReg g_dropColl; // 0x0024bed8

extern CSiblingActReg g_dropperActReg; // 0x0024be90

extern CSiblingActReg g_shadowActReg; // 0x0024bf00


// TU-local thunk/table names this TU registers (moved from the .cpp; the
// addresses are ILT thunk VAs, reloc-masked at every use).
extern i32 DropActB_c7be0();
extern i32 DropActA_c7090();

#endif // GRUNTZ_CDROPPEDOBJECT_H
