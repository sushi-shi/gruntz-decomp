#include <Ints.h>
#include <Gruntz/Dialogs.h> // CString (the E25c forwarder's cast)
#include <rva.h>
#include <string.h>
#include <Net/NetMgrMisc.h> // g_assetRoot decl

// g_assetRoot (0x0024e25c): CString - no provable static init (the type has no
// default ctor / is runtime-Init'd), so the datum is named by symbol.
DATA_SYMBOL(0x0024e25c, 0x0, ?g_assetRoot@@3VCString@@A)


RVA(0x000f9710, 0xa)
void NetPollE25c() {
    g_assetRoot.~CString();
}
