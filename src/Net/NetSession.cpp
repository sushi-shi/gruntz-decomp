// NetSession.cpp - CNetMgr DirectPlay session helpers (the 0x0f9xxx
// "player NAME" cluster). These are __cdecl free helpers the CNetMgr session
// code calls (reached via incremental-link thunks); none take a `this`.
//
// Matched here:
//   0x0f93b0  AppendInt - sprintf an int with "%i" into a scratch buffer, then
//             forward (dst, sep, buf) to the engine string-builder (0xf9280).
// (FillPlayerList @0xb89e0 lives in its home TU per the interval dossier #4b:
// src/Gruntz/Multi.cpp, the lobby list-box helper.)
#include <Net/NetMgr.h> // <Mfc.h> (reloc-masked externs)
#include <stdio.h>      // engine sprintf (reloc-masked)
#include <rva.h>

// The engine 3-arg string builder at 0x0f9280 (__cdecl, returns 1): concatenates
// a fixed prefix + `section` + `key` into `dst`. This IS MakeButeSectionKey (its home
// is src/Gruntz/FxModeDesc.cpp); referenced by its real name so the call relocs.
i32 MakeButeSectionKey(char* dst, const char* section, const char* key);

// ---------------------------------------------------------------------------
// AppendInt() - 0x0f93b0. Format `n` as decimal into a 256-byte scratch buffer
// ("%i"), then hand (dst, sep, buf) to the string builder. __cdecl, 3 args.
// ---------------------------------------------------------------------------
RVA(0x000f93b0, 0x41)
void AppendInt(char* dst, const char* sep, i32 n) {
    char buf[256];
    sprintf(buf, "%i", n);
    MakeButeSectionKey(dst, sep, buf);
}
