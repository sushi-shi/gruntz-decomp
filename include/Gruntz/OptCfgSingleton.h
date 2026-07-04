#ifndef GRUNTZ_OPTCFGSINGLETON_H
#define GRUNTZ_OPTCFGSINGLETON_H
#include <Ints.h>
#include <rva.h>

// The multiplayer game-state singleton pointer at 0x64bd5c, shared by the net-game
// configuration dialogs.  Its REAL identity is CMulti (xref-proven -- see
// include/Gruntz/Multi.h + NetCmdMgr.cpp / NetMgrMisc.cpp, which reach it as
// `CMulti* g_64bd5c`).  This OptCfg_c4b30 struct is the minimal placeholder VIEW the
// three dialog TUs use: ReconBatch2 reads m_5c0 (== CMulti::m_hostIndex); NetGameDlg
// and NetGameDlgWatch cast the pointer to their own session lenses (CNetSession /
// WatchSess).  Unified here to end the former 3-way independent redefinition.
//
// The canonical DATA(0x0024bd5c) binding + `extern` (mangled
// ?g_optCfg_64bd5c@@3PAUOptCfg_c4b30@@A) stays owned by ReconBatch2.cpp so the
// winning symbol name is unchanged; the other TUs re-declare that extern so their
// loads reloc-mask against it.  Keep the struct name/tag exactly (the `U` mangling
// is load-bearing for the symbol).
SIZE_UNKNOWN(OptCfg_c4b30);
struct OptCfg_c4b30 {
    char m_pad0[0x5c0];
    i32 m_5c0; // +0x5c0  (= CMulti::m_hostIndex)
};

#endif // GRUNTZ_OPTCFGSINGLETON_H
