// NetGuids.h - the five DirectPlay session/service GUID byte-tables owned by NetMgr.cpp
// (the game's OWN GUID data, passed by address to DirectPlayCreate/EnumConnections - not
// SDK-imported IID_/CLSID_ interface ids). A NARROW, owner-only decl header: included
// solely by NetMgr.cpp so the definitions can drop the `extern` keyword while keeping the
// exact external-linkage symbols. const u8[16] used only by address (no scalar constant-
// propagation) -> byte-neutral.
#ifndef NET_NETGUIDS_H
#define NET_NETGUIDS_H

#include <Ints.h>

extern const u8 g_guid1[16];
extern const u8 g_guid2[16];
extern const u8 g_guid3[16];
extern const u8 g_guid4[16];
extern const u8 g_guid5[16];

#endif // NET_NETGUIDS_H
