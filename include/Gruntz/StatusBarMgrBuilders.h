#ifndef GRUNTZ_CSTATUSBARMGR_BUILDERS_H
#define GRUNTZ_CSTATUSBARMGR_BUILDERS_H

#include <rva.h>

#include <Mfc.h>

#include <Gruntz/SBI_GruntMachine.h>
#include <Gruntz/SBI_Image.h>
#include <Gruntz/SBI_ImageSet.h>
#include <Gruntz/SBI_ImageSetAni.h>
#include <Gruntz/SBI_StatzTabGruntBar.h>
#include <Gruntz/SBI_WarlordHead.h>
#include <Gruntz/SBI_WellGoo.h>
#include <Gruntz/SbiConfig.h>
#include <Gruntz/StatusBarMgr.h>
#include <Ints.h>
#include <MakeRect.h>

#define NEW_STATUS_BAR_ITEM(item, type, host, cmd, tab, rect, key, frame, flags)                   \
    item = new type;                                                                               \
    if (!item->SetupImage(this, host, cmd, tab, rect, key, frame, flags)) {                        \
        delete item;                                                                               \
        return 0;                                                                                  \
    }

#endif // GRUNTZ_CSTATUSBARMGR_BUILDERS_H
