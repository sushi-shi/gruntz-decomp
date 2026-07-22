#ifndef GRUNTZ_CGRUNTHEALTHSPRITE_H
#define GRUNTZ_CGRUNTHEALTHSPRITE_H

#include <rva.h>
#include <Gruntz/GruntIndicatorSprite.h> // CIndicatorActReg + g_healthActReg
#include <Gruntz/UserLogic.h>            // CUserLogic base (CGruntHealthSprite : CUserLogic)
#include <Gruntz/SerialArchive.h>        // shared CSerialArchive (Read +0x2c / Write +0x30)

class CGrunt;

class CGruntHealthSprite : public CUserLogic, public CWapX {
public:
public:
    virtual i32 SerializeMove(CGruntArchive*, i32, i32, i32) OVERRIDE; // slot 1
    RVA(0x00011f60, 0x6)
    virtual LogicTypeId GetTypeTag() OVERRIDE {
        return LOGIC_GRUNTHEALTHSPRITE;
    } // slot 2
    static void InitActReg(); // 0x07ecf0 (construct g_healthActReg over [2000,2010])
    virtual void FireActivation(i32 id)
        OVERRIDE;               // 0x07ed70 (resolve the id's registered handler + dispatch it)
    static void RegisterActs(); // 0x07eed0 (register the class's activation handlers)

    i32 HealthUpdate(); // 0x07f180 (the registered per-frame handler; reconstructed in the .cpp)
    i32 SetHealthGlyph(i32 x, i32 y, i32 health); // 0x07f0d0
    // slot 16 (new): the per-class stat-time getter (leaf overrides read the bound
    // grunt's stamina/wingz/toy timer); HealthUpdate dispatches it with the grunt entry.
    virtual i32 Vslot16(CGrunt* grunt);
    CGruntHealthSprite();                   // 0x011ef0 (no-arg ctor; body in GruntHealthSprite.cpp)
    CGruntHealthSprite(CGameObject* obj);   // 0x07eb00 (1-arg ctor; body in GruntHealthSprite.cpp)
    // NO user-declared dtor: retail's is COMPILER-GENERATED (implicit
    // elides the leaf-vptr restamp; RVA_COMPGEN pin in the home TU).
                                            // so its 0x11fb0 COMDAT labels cleanly - an inline dtor
                                            // can't hang RVA() without also tagging the synthesized
    // ??_G, tripping the duplicate-RVA guard. The derived leaf
    // dtors therefore tail-jump this base dtor rather than
    // inlining the teardown - a pre-existing modeling gap.)

    // CUserLogic is 0x40; the leaf adds its own fields. SetHealthGlyph stashes the
    // two coordinates at +0x54/+0x58 and the health at +0x5c.
    i32 m_cellX;  // +0x54  stashed grunt cell x
    i32 m_cellY;  // +0x58  stashed grunt cell y
    i32 m_health; // +0x5c  stashed health value
    i32 m_60;     // +0x60  screen-Y bias (HealthUpdate adds it to the grunt's screen y)
};
SIZE_UNKNOWN();

typedef i32 (CUserLogic::*HealthActHandler)();
struct CHealthActEntry {
    HealthActHandler m_fn;
};
SIZE_UNKNOWN();

#endif // GRUNTZ_CGRUNTHEALTHSPRITE_H
