#include <Ints.h>
#include <Gruntz/Dialogs.h> // CString (the E25c forwarder's cast)
#include <rva.h>
#include <string.h>

DATA(0x0024e25c)
extern CString g_assetRoot; // VA 0x64e25c

RVA(0x000f9710, 0xa)
void NetPollE25c() {
    g_assetRoot.~CString();
}
