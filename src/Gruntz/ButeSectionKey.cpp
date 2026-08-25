// @identity-TODO
// Xrefs place this helper in the NetMgr/NetSession domain, but no original-object
// boundary proves its exact owner TU.

#include <rva.h>

#include <Ints.h>

#include <string.h>

RVA(0x000f9280, 0xe4)
i32 MakeButeSectionKey(char* dst, const char* section, const char* key) {
    if (!key) {
        return 0;
    }
    strcat(dst, "[");
    strcat(dst, section);
    strcat(dst, ":");
    strcat(dst, key);
    strcat(dst, "]");
    return 1;
}
