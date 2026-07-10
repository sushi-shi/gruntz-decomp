// Cluster0c.cpp - drained. The two methods of the unidentified per-session net
// object (CCluster0c) have been spatially re-homed:
//   0xc0c20 CCluster0c::Init    -> src/Net/NetCmdSlot.cpp (its RVA neighborhood)
//   0xc5240 CCluster0c::Cleanup -> src/Gruntz/MultiStartDlgRoster.cpp
// (The former "Run" @0xc2a50 was a CONFLATION -> re-homed to src/Net/NetMgrMisc.cpp
// as CMultiStartDlg::Method_c2a50.) The real class identity of Init/Cleanup is still
// unrecovered (a net-session sub-object); class attribution is a later pass.
#include <rva.h>
