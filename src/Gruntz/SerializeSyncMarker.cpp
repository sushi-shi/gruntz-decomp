// SerializeSyncMarker.cpp - the WAP32 serialize round validator (0x13610, __cdecl),
// split out of GruntzMgr.cpp: a free function whose retail object sits far from the
// CGruntzMgr block (0x13610, among the CVoiceTrigger / CMotionState serialize .text).
// A shared serialize helper with no recovered caller; homed with the g_629a50 scratch
// buffer it owns and the EnterModalUI reporter it drives. Same "eh" flags; byte-neutral.
#include <Mfc.h>                  // wsprintfA (USER32, reloc-masked)
#include <Gruntz/GruntzMgr.h>     // CGruntzMgr (g_mgrSettings->EnterModalUI)
#include <Gruntz/SerialArchive.h> // CSerialArchive (arc->Read @+0x2c / Write @+0x30)
#include <rva.h>

// The serialize-round sync counter (VA 0x629ad0; DATA home in Io/GameSave.cpp), the
// game-manager singleton (*0x64556c), and the wsprintfA scratch buffer (VA 0x629a50)
// the desync path formats into (its sole DATA home is here now).
extern "C" i32 g_serialCounter; // 0x629ad0
DATA(0x0024556c)
extern "C" CGruntzMgr* g_mgrSettings; // 0x64556c
DATA(0x00229a50)
extern "C" char g_629a50[]; // 0x629a50  wsprintfA scratch buffer

// SerializeSyncMarker (0x13610, __cdecl): the WAP32 serialize round validator every
// object's Serialize brackets its record with. WRITE (mode 4): stamp the current
// round counter (+ the 0x1234666 magic) into the archive. READ (mode 7): read it
// back, and if it does not match, format "save/load out of sync at <name>, <line>"
// and pop it in a modal error box (CGruntzMgr::EnterModalUI). Returns 1 (ok) / 0
// (desync).
RVA(0x00013610, 0x8c)
i32 SerializeSyncMarker(CSerialArchive* arc, i32 mode, const char* name, i32 line) {
    if (mode == 4) {
        i32 marker = g_serialCounter + 0x1234666;
        arc->Write(&marker, 4);
        return 1;
    }
    if (mode == 7) {
        i32 readVal;
        arc->Read(&readVal, 4);
        if (readVal != g_serialCounter + 0x1234666) {
            wsprintfA(g_629a50, "save/load out of sync at %s, %d", name, line);
            g_mgrSettings->EnterModalUI((i32)g_629a50);
            return 0;
        }
    }
    return 1;
}
