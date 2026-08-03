#ifndef SRC_BUTE_BUTESTORERESETCOPY_H
#define SRC_BUTE_BUTESTORERESETCOPY_H

#include <rva.h>

#include <Bute/ButeStore.h>

struct CButeStoreResetCopyClear : public zPTree {
    void ResetCopy();
};
SIZE(0x2c);

#endif // SRC_BUTE_BUTESTORERESETCOPY_H
