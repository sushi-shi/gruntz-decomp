// KitchenSlime.h - the kitchen-slime hazard game-object (C:\Proj\Gruntz), a
// CUserLogic leaf. The bound CGameObject is held at the inherited m_10/m_38
// (CUserLogic sets m_10==m_38==obj); the slime views it as CSlimeLevel /
// CSlimeAnimPlayer (a typed reinterpret of the same object - the bodies cast
// m_10/m_38 at each use, codegen-neutral). The leaf adds the movement-integrator
// state at +0x58. Only offsets / code bytes are load-bearing.
#ifndef GRUNTZ_CKITCHENSLIME_H
#define GRUNTZ_CKITCHENSLIME_H

#include <rva.h>

#include <Gruntz/LogicTypeId.h> // LogicTypeId (GetTypeTag return type)
#include <Gruntz/UserLogic.h>   // CUserLogic base (+ CGameObject / CGruntArchive)

// The slime's typed views of the SAME bound object (full defs live in the .cpp).
struct CSlimeLevel;
struct CSlimeAnimPlayer;

class CKitchenSlime : public CUserLogic {
public:
    virtual i32 SerializeMove(CGruntArchive*, i32, i32, i32) OVERRIDE; // slot 1
    virtual LogicTypeId GetTypeTag() OVERRIDE;                         // slot 2
    virtual i32 UserLogicVfunc2() OVERRIDE;                            // slot 4
    TILE_LOGIC_TAIL
public:
    static void RegisterRange(); // 0x0b28c0 (seed the activation table's fast range)
    static void RegisterType();  // 0x0b2aa0 (level-load class registrar)
    void FireActivation(i32 coord);
    i32 Tick();
    i32 Serialize(void* stream, i32 tag, i32 c, i32 d);      // 0x0b2ff0
    i32 SerializeChain(void* stream, i32 tag, i32 c, i32 d); // 0x16e7f0 (inherited base chain)
    i32 LoadSprites();
    CKitchenSlime(CGameObject* obj);   // 0x0b23a0 (folds CUserLogic(obj) + the slime setup)
    virtual ~CKitchenSlime() OVERRIDE; // 0x013100 (folds the CUserLogic teardown)

    // The bound CGameObject (inherited m_object==m_38) viewed as the slime's typed
    // level / anim-player data (same object, non-overlapping field windows). The one
    // reinterpret lives here so the bodies read Level()->/Anim()-> with no cast;
    // codegen-neutral (each call is the same `mov reg,[this+0x10]` / `+0x38`).
    CSlimeLevel* Level() {
        return (CSlimeLevel*)m_object;
    }
    CSlimeAnimPlayer* Anim() {
        return (CSlimeAnimPlayer*)m_38;
    }

    i32 m_savedGeoId; // +0x40  saved m_38->m_1b4 geometry id (before GAME_CYCLE100)
    char m_pad44[0x58 - 0x44];
    double m_speed;  // +0x58  per-frame speed (g_slimeSpeedNum / timePerTile)
    double m_posX;   // +0x60  sub-pixel X position accumulator
    double m_posY;   // +0x68  sub-pixel Y position accumulator
    double m_dirX;   // +0x70  unit X travel direction (-1.0 / 0.0 / +1.0)
    double m_dirY;   // +0x78  unit Y travel direction (-1.0 / 0.0 / +1.0)
    i32 m_tileX;     // +0x80  current target tile X (pixels)
    i32 m_tileY;     // +0x84  current target tile Y (pixels)
    i32 m_stepMag;   // +0x88  per-step magnitude double {lo,hi}, overlaid as int pair
    i32 m_stepMagHi; // +0x8c  per-step magnitude, hi dword
};
VTBL(CKitchenSlime, 0x1e750c);
SIZE(CKitchenSlime, 0x90);

#endif // GRUNTZ_CKITCHENSLIME_H
