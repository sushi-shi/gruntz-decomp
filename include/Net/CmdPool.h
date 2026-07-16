// CmdPool.h - the recycled network-command node free pool (0x64aca8; DATA home
// src/Net/NetCmdSlot.cpp). A real MFC CPtrList constructed explicitly (block size
// 0xa) by the pool initializer, drained back on reset. Declared here so consumers
// reference it from this owner header instead of per-TU externs. CPtrList's full
// definition comes from <Mfc.h> at the use sites.
#ifndef NET_CMDPOOL_H
#define NET_CMDPOOL_H

class CPtrList;

extern CPtrList g_pool; // 0x64aca8

#endif // NET_CMDPOOL_H
