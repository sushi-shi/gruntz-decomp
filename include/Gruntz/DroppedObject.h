// DroppedObject.h - a dropped game-object (C:\Proj\Gruntz).
//
// CDroppedObject : CUserLogic - a tile-logic leaf in the same game-object
// hierarchy as CTimeBomb (proven by its dtor @0x0125b0 stamping the CUserLogic
// vftable 0x5e705c then the CUserBase vftable 0x5e70b4, tearing down the +0x18
// link via the embedded ~EngStr at 0x16d2a0 - byte-identical in shape to
// ~CTimeBomb @0x012a70). The leaf adds no destructible members beyond
// CUserLogic, so its dtor folds the bare CUserLogic teardown (the /GX leaf-dtor
// archetype).
//
// FireActivation (0x0c6bd0) is the SAME per-coordinate activation-registry
// dispatcher archetype as CTimeBomb::FireActivation (0x0e1830), but a DIFFERENT
// registry instance (CDroppedObject's, at 0x64bed8 vs the timebomb's at
// 0x64c780).
//
// Field names are placeholders; only OFFSETS + the inheritance chain are
// load-bearing.
#ifndef GRUNTZ_CDROPPEDOBJECT_H
#define GRUNTZ_CDROPPEDOBJECT_H

#include <rva.h>
#include <Gruntz/UserLogic.h> // CUserLogic base (CDroppedObject : CUserLogic)

class CDroppedObject : public CTileLogic {
public:
    CDroppedObject(CGameObject* obj);   // 0x0c68b0 (1-arg leaf ctor)
    static void RegisterRange();        // 0x0c6b50 (seed the activation table's fast range)
    static void RegisterActs();         // 0x0c6d30
    void FireActivation(i32 coord);     // 0x0c6bd0
    i32 ActA();                         // 0x0c7090 (per-frame "A" activation handler)
    virtual ~CDroppedObject() OVERRIDE; // 0x0125b0 (folds the CUserLogic teardown)

    i32 m_savedGeoId; // +0x40  m_38->m_geoId snapshot
    char m_pad44[0x58 - 0x44];
    double m_timePerTile; // +0x58  per-tile time (32.0 / TimePerTile)
    double m_fallY;       // +0x60  fall accumulator (adjusted screen Y)
    i32 m_landY;          // +0x68  landing row (pre-offset screen Y)
};

#endif // GRUNTZ_CDROPPEDOBJECT_H
