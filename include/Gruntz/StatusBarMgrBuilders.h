#ifndef GRUNTZ_CSTATUSBARMGR_BUILDERS_H
#define GRUNTZ_CSTATUSBARMGR_BUILDERS_H

#include <Ints.h>
#include <rva.h>
#include <Mfc.h>                 // CPtrList (embedded tab lists in CStatusBarMgr)
#include <Gruntz/SbRect.h>       // the by-value geometry rect the setup virtuals take
#include <Gruntz/StatusBarMgr.h> // canonical CStatusBarMgr
#include <Gruntz/SbiConfig.h>    // canonical CDDrawSurfaceMgr (the setup virtuals' arg2)

#include <Gruntz/SBI_Image.h>            // CSBI_RectOnly + CSBI_Image             (tag 3)
#include <Gruntz/SBI_ImageSet.h>         // CSBI_ImageSet                          (tag 4)
#include <Gruntz/SBI_ImageSetAni.h>      // CSBI_ImageSetAni + CSBI_StatzTabArrow  (8 / 5)
#include <Gruntz/SBI_WellGoo.h>          // CSBI_WellGoo                           (tag 7)
#include <Gruntz/SBI_WarlordHead.h>      // CSBI_WarlordHead                       (tag 0xb)
#include <Gruntz/SBI_GruntMachine.h>     // CSBI_GruntMachine                      (tag 9)
#include <Gruntz/SBI_StatzTabGruntBar.h> // CSBI_StatzTabGruntBar                  (tag 6)

class CSbFactory {
public:
    void* GetByIndex(i32 idx, i32 z); // thunk 0x4165 -> FUN_004e23c0
};
SIZE_UNKNOWN(CSbFactory);

class CSbIconSet {};
SIZE_UNKNOWN(CSbIconSet);

#endif // GRUNTZ_CSTATUSBARMGR_BUILDERS_H
