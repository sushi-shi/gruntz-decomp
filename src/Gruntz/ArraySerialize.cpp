// ArraySerialize.cpp - the CArray<PLAYLISTINFOSTRUCT*, PLAYLISTINFOSTRUCT*>
// instantiation host: the movie/creditz playlist array (worker+0x868c,
// <Io/MoviePlayer.h> CMoviePlaylist) is the REAL MFC template - RTTI-proven by the
// COL at its vtable 0x1e971c (.?AV?$CArray@PAUPLAYLISTINFOSTRUCT@@PAU1@@@). The
// explicit instantiation below makes THIS obj emit the full member COMDAT family
// under the exact retail mangled names, and the @rva-symbol pins bind the
// retail-kept copies:
//   ??1  @0x39f20 (0x51)  - the /GX dtor (`if (m_pData) delete[]` + inline ~CObject;
//                           formerly the fake `CWorker39f20` dtor in CreditzAssets.cpp)
//   ??_G @0x3a1a0 (0x1e)  - the scalar-deleting dtor (vtable slot 1 via thunk 0x40f7)
//   ?Serialize @0x39fa0 (0x188) - vtable slot 2 (the former hand-rolled
//                           CMovieScratch::Serialize reconstruction here - MFC's real
//                           afxtempl source replaces it)
//   ??0  @0x94340         - the default ctor (called out-of-line by the state builder
//                           that constructs the playlist embed; stamp @0x94346 -> 0x5e971c)
// The vtable datum 0x1e971c auto-names from config/vtable_names.csv when this obj's
// ??_7 COMDAT lands. delete[] lowers to ??3@YAXPAX@Z @0x1b9b82 (== _RezFree; FID
// carries both names - the game's Rez free IS operator delete).
//
// @rva-symbol: ??1?$CArray@PAUPLAYLISTINFOSTRUCT@@PAU1@@@UAE@XZ 0x00039f20 0x51
// @rva-symbol: ??_G?$CArray@PAUPLAYLISTINFOSTRUCT@@PAU1@@@UAEPAXI@Z 0x0003a1a0 0x1e
// @rva-symbol: ?Serialize@?$CArray@PAUPLAYLISTINFOSTRUCT@@PAU1@@@UAEXAAVCArchive@@@Z 0x00039fa0 0x188
// @rva-symbol: ??0?$CArray@PAUPLAYLISTINFOSTRUCT@@PAU1@@@QAE@XZ 0x00094340
#include <Ints.h>
#include <rva.h>

#include <Io/MoviePlayer.h> // CMoviePlaylist typedef + the full PLAYLISTINFOSTRUCT

template class CArray<PLAYLISTINFOSTRUCT*, PLAYLISTINFOSTRUCT*>;
