#ifndef GRUNTZ_SERIALARCHIVE_H
#define GRUNTZ_SERIALARCHIVE_H

class CFileMemBase;

// --- the TU's extern surface (moved out of the .cpp; addresses/thunk
// VAs are reloc-masked at use) ---
    // UNDEFINED DATA: a char[] datum here is a STRING (or a run of them); its
    // extent is not boundable from the named-symbol gaps (the unnamed $SG literals
    // in between get swallowed). Inline the literal at its use site instead.
extern "C" char g_syncErrMsgBuf[];

#endif // GRUNTZ_SERIALARCHIVE_H
