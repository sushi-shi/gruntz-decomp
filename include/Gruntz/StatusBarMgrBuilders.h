#ifndef GRUNTZ_CSTATUSBARMGR_BUILDERS_H
#define GRUNTZ_CSTATUSBARMGR_BUILDERS_H

#include <Ints.h>
#include <rva.h>
#include <Mfc.h>
#include <Gruntz/SbGeom.h>
#include <Gruntz/StatusBarMgr.h>
#include <Gruntz/SbiConfig.h>

#include <Gruntz/SBI_Image.h>
#include <Gruntz/SBI_ImageSet.h>
#include <Gruntz/SBI_ImageSetAni.h>
#include <Gruntz/SBI_WellGoo.h>
#include <Gruntz/SBI_WarlordHead.h>
#include <Gruntz/SBI_GruntMachine.h>
#include <Gruntz/SBI_StatzTabGruntBar.h>

class CSbFactory {
public:
    void* GetByIndex(i32 idx, i32 z);
};
SIZE_UNKNOWN();

class CSbIconSet {};
SIZE_UNKNOWN();

#endif // GRUNTZ_CSTATUSBARMGR_BUILDERS_H
