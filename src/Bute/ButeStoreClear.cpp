#include <Bute/ButeStoreDtorCopies.h> // the shared COMDAT-copy anchors (CButeStore == zPTree)
#include <rva.h>

RVA(0x000212a0, 0x21)
void CButeStoreResetCopyClear::ResetCopy() {
    Reset();
}
