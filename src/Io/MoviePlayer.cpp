#include <rva.h>

#include <Io/MoviePlayer.h>

#include <Mfc.h>

#include <Crypto/FecCrypt.h>

RVA(0x00038fc0, 0xa5)
CMoviePlayer::~CMoviePlayer() {
    Teardown();
}

RVA_COMPGEN(0x000390a0, 0x5d, ??1CFecFile@@QAE@XZ)
