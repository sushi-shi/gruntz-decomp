#include <rva.h>
// CGruntzCmdList.cpp - the command-list recycle list.
//
// 0x1b4a27 ("RemoveTail") is MFC `CObList::RemoveTail` (statically linked NAFXCW):
// it is already anchored in config/library_labels.csv as
// ?RemoveTail@CObList@@QAEPAVCObject@@XZ, and its whole sibling cluster
// (0x1b48a6..0x1b4afe: RemoveAll / NewNode / FreeNode / AddHead / AddTail /
// RemoveHead / InsertBefore / InsertAfter / RemoveAt / FindIndex) is CObList too.
// The MFC-library codegen tell is the `and [mem],0` pointer-zeroing idiom + the
// shared FreeNode tail (retail's static MFC was built with different flags than the
// game's /O2). NOT game code -> removed from src so status.py carves it as library
// (FID); the game callers (CGruntzCommand allocators via g_singleCmdList /
// g_multiCmdList, and CGruntzCmdMgr's GzObList) reach it as a reloc-masked external
// thiscall callee, exactly as GruntzCmdMgr.h already documents.
