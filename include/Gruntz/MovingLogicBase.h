// MovingLogicBase.h - light, decl-only view of CMovingLogicBase::Serialize (0x16e7f0),
// the shared serialize-chain leaf classes drive on `this`. The FULL CMovingLogicBase
// (with its persisted fields) lives in <Gruntz/MovingLogicSerial.h>, which transitively
// pulls Mfc.h and cannot go into lean leaf TUs; this header carries ONLY the method decl
// (no Mfc.h) so a leaf's Serialize override can bind its chain call to the real 0x16e7f0.
//
// NEVER include this together with <Gruntz/MovingLogicSerial.h> in one TU (the two
// CMovingLogicBase definitions collide) - the grunt-motion TUs that need the full class
// use MovingLogicSerial.h and call Serialize directly, so they must not pull this.
#ifndef GRUNTZ_MOVINGLOGICBASE_H
#define GRUNTZ_MOVINGLOGICBASE_H

#include <Ints.h>

struct CSerialArchive;

class CMovingLogicBase {
public:
    i32 Serialize(CSerialArchive* arc, i32 mode, i32 a3, i32 a4); // 0x16e7f0
};

#endif // GRUNTZ_MOVINGLOGICBASE_H
