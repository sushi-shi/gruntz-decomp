#ifndef INCLUDE_IO_SAVEGAMEDTORINLINE_H
#define INCLUDE_IO_SAVEGAMEDTORINLINE_H

#include <rva.h>

#include <Io/SaveGame.h>

// The INLINED shape of ~CSaveGame. Retail keeps the standalone COMDAT at
// 0x85b50 (SaveGame.cpp) for CGruntzMgr::Close and ALSO expands the same body
// at CGruntzMgr::Run's `delete m_saveSink` - `call Reset`, then the two CString
// members in reverse declaration order, with the EH state transitions in
// between (0x83450 @ 0xe4e). Only the TU that wants the expansion includes
// this; SaveGame.cpp keeps the out-of-line copy.
inline CSaveGame::~CSaveGame() {
    Reset();
}

#endif // INCLUDE_IO_SAVEGAMEDTORINLINE_H
