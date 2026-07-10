// SaveGame.cpp - CSaveGame, the WAP32 save-game / saved-slot roster manager.
// See SaveGame.h for the layout and the provenance note (these were mislabeled
// "CFileIO" by the this/ecx tracer; they are the OWNER class that USES CFileIO).
//
// Each file-touching method (Load/Save) builds a stack-local CFileIO and drives
// Open/Read/Write/Close/~CFileIO on it; the CString member + the CFileIO temp's
// dtor put a C++ EH frame on those methods -> /GX (the `mfc` /O1 profile, matching
// FileStream.cpp). The leaf accessors are frameless register-frame functions.
//
// All cross-class callees (CFileIO, CString, CGameRegistry, the CRT _strncpy, the
// MFC wait-cursor helpers) are modeled as external/no-body so their reloc
// operands are masked in objdiff.
#include <Io/SaveGame.h>
#include <Gruntz/FontConfig.h>
#include <rva.h>

#include <stdlib.h> // _itoa
#include <string.h> // memset -> inline rep stos

// The two per-slot dialog labellers (0x0e3e80 for the save dialog, 0x9e2d0 for the
// GAME_INFO dialog): each labels one CSaveGame::GetSlot() record into four dialog
// controls (same signature, different control-state side effects). Their real owner
// TU (they walk this file's save slots); forward-declared here (the FillSaveDialog /
// FillGameInfoDialog callers precede them in retail-RVA order), DEFINED at the end.
// The slot pointer flows in as the real Io/SaveGame.h SaveSlot* GetSlot() returns.
// IsSlotOccupied (0x2694 -> 0xe5700) is the slot-occupancy probe. All reloc-masked.
i32 IsSlotOccupied(SaveSlot* item); // 0x2694 (jmp-thunk -> 0xe5700)
void LabelSaveSlot(HWND hWnd, SaveSlot* item, i32 id3, i32 id4, i32 id5, i32 id6);     // 0x0e3e80
void LabelGameInfoSlot(HWND hWnd, SaveSlot* item, i32 id3, i32 id4, i32 id5, i32 id6); // 0x09e2d0

// ---------------------------------------------------------------------------
// 0x0e3be0 (spatially re-homed from src/Stub/ApiCallers.cpp). __stdcall dialog
// proc: WM_COMMAND OK/Cancel end the dialog; self-contained (no info line).
RVA(0x000e3be0, 0x52)
i32 CALLBACK OkCancelDialogProc(HWND hDlg, UINT msg, WPARAM wParam, LPARAM lParam) {
    switch (msg) {
        case 0x110:
            return 1;
        case 0x111:
            if (wParam == 2) {
                EndDialog(hDlg, 0);
                return 1;
            }
            if (wParam == 1) {
                EndDialog(hDlg, 1);
                return 1;
            }
            break;
    }
    return 0;
}

// ---------------------------------------------------------------------------
// FillSaveDialog  (0x000e3c60): walk the ten save slots of `sg`, labelling each
// into its row of four dialog controls (base IDs 0x435 / 0x490 / 0x49a / 0x4a4 +
// slot index). __cdecl(HWND, CSaveGame*); both pointers null-checked up front.
RVA(0x000e3c60, 0x1a3)
void FillSaveDialog(HWND hWnd, CSaveGame* sg) {
    if (hWnd == 0 || sg == 0) {
        return;
    }
    LabelSaveSlot(hWnd, sg->GetSlot(0), 0x435, 0x490, 0x49a, 0x4a4);
    LabelSaveSlot(hWnd, sg->GetSlot(1), 0x436, 0x491, 0x49b, 0x4a5);
    LabelSaveSlot(hWnd, sg->GetSlot(2), 0x437, 0x492, 0x49c, 0x4a6);
    LabelSaveSlot(hWnd, sg->GetSlot(3), 0x438, 0x493, 0x49d, 0x4a7);
    LabelSaveSlot(hWnd, sg->GetSlot(4), 0x439, 0x494, 0x49e, 0x4a8);
    LabelSaveSlot(hWnd, sg->GetSlot(5), 0x43a, 0x495, 0x49f, 0x4a9);
    LabelSaveSlot(hWnd, sg->GetSlot(6), 0x43b, 0x496, 0x4a0, 0x4aa);
    LabelSaveSlot(hWnd, sg->GetSlot(7), 0x43c, 0x497, 0x4a1, 0x4ab);
    LabelSaveSlot(hWnd, sg->GetSlot(8), 0x43d, 0x498, 0x4a2, 0x4ac);
    LabelSaveSlot(hWnd, sg->GetSlot(9), 0x43e, 0x499, 0x4a3, 0x4ad);
}

// ---------------------------------------------------------------------------
// FillGameInfoDialog  (0x0009e0b0): the GAME_INFO dialog's slot roster filler -
// the twin of FillSaveDialog, but labelling through the 0x9e2d0 variant of the
// per-slot helper. Same ten rows / same four base control IDs (0x435 / 0x490 /
// 0x49a / 0x4a4 + slot index). __cdecl(HWND, CSaveGame*); both pointers null-
// checked up front.
RVA(0x0009e0b0, 0x1a3)
void FillGameInfoDialog(HWND hWnd, CSaveGame* sg) {
    if (hWnd == 0 || sg == 0) {
        return;
    }
    LabelGameInfoSlot(hWnd, sg->GetSlot(0), 0x435, 0x490, 0x49a, 0x4a4);
    LabelGameInfoSlot(hWnd, sg->GetSlot(1), 0x436, 0x491, 0x49b, 0x4a5);
    LabelGameInfoSlot(hWnd, sg->GetSlot(2), 0x437, 0x492, 0x49c, 0x4a6);
    LabelGameInfoSlot(hWnd, sg->GetSlot(3), 0x438, 0x493, 0x49d, 0x4a7);
    LabelGameInfoSlot(hWnd, sg->GetSlot(4), 0x439, 0x494, 0x49e, 0x4a8);
    LabelGameInfoSlot(hWnd, sg->GetSlot(5), 0x43a, 0x495, 0x49f, 0x4a9);
    LabelGameInfoSlot(hWnd, sg->GetSlot(6), 0x43b, 0x496, 0x4a0, 0x4aa);
    LabelGameInfoSlot(hWnd, sg->GetSlot(7), 0x43c, 0x497, 0x4a1, 0x4ab);
    LabelGameInfoSlot(hWnd, sg->GetSlot(8), 0x43d, 0x498, 0x4a2, 0x4ac);
    LabelGameInfoSlot(hWnd, sg->GetSlot(9), 0x43e, 0x499, 0x4a3, 0x4ad);
}

// ---------------------------------------------------------------------------
// CSaveGame::~CSaveGame  (0x00085b50)
// Reset() then the two CString members destruct in reverse declaration order
// (m_name @+4, then m_str0 @+0) under the EH unwind frame.
RVA(0x00085b50, 0x56)
CSaveGame::~CSaveGame() {
    Reset();
}

// ---------------------------------------------------------------------------
// CSaveGame::SaveGameFile
// Seed the roster from a directory: m_str0 = dir, m_name = dir +
// "Gruntz.sav", zero the 0xa1c header, Init() + Load() the roster, then for each
// of the ten slots that exists format its per-slot file path dir + "Slot" +
// (i+1) + ".sav" into the slot record at +0x35 (wsprintfA hoists the IAT pointer
// into ebx across the loop). The chained CString operator+ temps + the assigned
// CString member force the /GX EH frame.
RVA(0x000e4b60, 0x158)
i32 CSaveGame::SaveGameFile(const char* dir) {
    if (dir == 0) {
        return 0;
    }
    m_str0 = dir;
    m_name = m_str0 + "Gruntz.sav";
    memset(m_header, 0, 0xa1c);
    Init();
    Load();
    for (i32 i = 0; i < 10; i++) {
        SaveSlot* slot = GetSlot(i);
        if (slot != 0) {
            char numbuf[16];
            _itoa(i + 1, numbuf, 10);
            wsprintfA(slot->m_savePath, m_str0 + "Slot" + numbuf + ".sav");
        }
    }
    return 1;
}

// ---------------------------------------------------------------------------
// CSaveGame::Reset  (0x000e4d20)
// Init() the slots, then empty the file-name CString.
RVA(0x000e4d20, 0x12)
void CSaveGame::Reset() {
    Init();
    m_name.Empty();
}

// ---------------------------------------------------------------------------
// CSaveGame::Init  (0x000e4d50)
// Header field @+0x18 = 0x25, then zero all ten 0x100-byte slot records.
RVA(0x000e4d50, 0x2f)
void CSaveGame::Init() {
    m_maxLevel = 0x25;
    for (i32 i = 0; i < 10; i++) {
        SaveSlot* p = GetSlot(i);
        if (p != 0) {
            for (i32 j = 0; j < 0x40; j++) {
                ((i32*)p)[j] = 0;
            }
        }
    }
}

// ---------------------------------------------------------------------------
// CSaveGame::Load  (0x000e4d90)
// Open(m_name) read-only, read the 0xa1c header then the 0xa00 slot block, close.
RVA(0x000e4d90, 0xcc)
i32 CSaveGame::Load() {
    CFileIO file;
    if (!file.Open(m_name, 0, 0)) {
        return 0;
    }
    file.Read(m_header, 0xa1c);
    file.Read(m_slots, 0xa00);
    file.Close();
    if (!Verify()) {
        Init();
    }
    return 1;
}

// ---------------------------------------------------------------------------
// CSaveGame::Save  (0x000e4ea0)
// @early-stop
// Wait-cursor (AfxGetThreadState()->...->BeginWaitCursor/EndWaitCursor) around a
// create+write+close sequence with two g_gameReg error/notify branches. The MFC
// wait-cursor / module-state internals and the two divergent error paths are not
// yet modeled; logic outline below, byte-match deferred to the final sweep.
RVA(0x000e4ea0, 0x18c)
i32 CSaveGame::Save(i32 a, i32 b) {
    CFileIO file;
    i32 ok = 0;
    if (file.Open(m_name, 0x1000, 0)) {
        file.Close();
        if (file.Open(m_name, 1, 0)) {
            ComputeAll();
            file.Write(m_header, 0xa1c);
            file.Write(m_slots, 0xa00);
            file.Close();
            ok = 1;
        }
    }
    (void)a;
    (void)b;
    (void)ok;
    return 1;
}

// ---------------------------------------------------------------------------
// CSaveGame::ComputeAll  (0x000e50a0)
// Sum Encode() over all ten slots; store header fields (@+0x08..+0x14).
// @early-stop
// regalloc wall (~91%): retail materializes the `1` store via `mov eax,1; mov
// [edi+0xc],eax`; recompile uses the `mov $1,mem` immediate form. Logic exact.
RVA(0x000e50a0, 0x3e)
void CSaveGame::ComputeAll() {
    i32 sum = 0;
    for (i32 i = 0; i < 10; i++) {
        sum += Encode((u8*)GetSlot(i));
    }
    *(i32*)&m_header[0] = 0;
    *(i32*)&m_header[4] = 1;
    *(i32*)&m_header[8] = sum;
    *(i32*)&m_header[0xc] = 0;
}

// ---------------------------------------------------------------------------
// CSaveGame::Verify  (0x000e50f0)
// Re-decode every slot, sum, compare to the stored checksum @ (this+0x18).
RVA(0x000e50f0, 0x2f)
i32 CSaveGame::Verify() {
    i32 sum = 0;
    for (i32 i = 0; i < 10; i++) {
        sum += Decode((u8*)GetSlot(i));
    }
    return *(i32*)&m_header[8] == sum;
}

// ---------------------------------------------------------------------------
// CSaveGame::FillSlot  (0x000e5130)
// `src` is the live game-state object being captured; only two of its members are
// probed here (a level ptr @+0x2c whose +0x1c is the level id, and a world ptr
// @+0x44 whose +0x124 flags a custom world). That object's class is not modeled in
// this TU, so the two fields are read as binary-proven pointer arithmetic (the same
// forced opaque cross-class read the doctrine allows for un-recovered externs).
RVA(0x000e5130, 0x78)
i32 CSaveGame::FillSlot(SaveSlot* dst, const char* name, void* src) {
    if (dst == 0) {
        return 0;
    }
    if (src == 0) {
        return 0;
    }
    dst->m_type = 1;
    dst->m_levelId = *(i32*)((char*)*(void**)((char*)src + 0x2c) + 0x1c);
    dst->m_count = 0;
    dst->m_active = 1;
    if (*(i32*)((char*)*(void**)((char*)src + 0x44) + 0x124) != 0) {
        dst->m_type = 3;
    }
    strncpy(dst->m_name, name, 0x20);
    dst->m_checksum = Register(dst);
    return 1;
}

// ---------------------------------------------------------------------------
// CSaveGame::CopySlot  (0x000e51d0)
RVA(0x000e51d0, 0x49)
i32 CSaveGame::CopySlot(SaveSlot* dst, const SaveSlot* src) {
    if (dst == 0) {
        return 0;
    }
    if (src == 0) {
        return 0;
    }
    dst->m_type = src->m_type;
    dst->m_levelId = src->m_levelId;
    dst->m_count = src->m_count;
    dst->m_active = src->m_active;
    dst->m_checksum = src->m_checksum;
    dst->m_checksum = Register(dst);
    return 1;
}

// ---------------------------------------------------------------------------
// CSaveGame::FillSlot2  (0x000e5240)
RVA(0x000e5240, 0x54)
i32 CSaveGame::FillSlot2(SaveSlot* dst, i32 name, void* src) {
    if (dst == 0) {
        return 0;
    }
    if (src == 0) {
        return 0;
    }
    dst->m_type = 1;
    dst->m_levelId = name;
    dst->m_count = 0;
    if (*(i32*)((char*)*(void**)((char*)src + 0x44) + 0x124) != 0) {
        dst->m_type = 3;
    }
    dst->m_checksum = Register(dst);
    return 1;
}

// ---------------------------------------------------------------------------
// CSaveGame::VerifySlot  (0x000e52c0)
// Re-derive the slot's level rez-path id and validate it: fail (with an "does
// not exist" notice) if the registry can't resolve it, fail (with a "has
// changed" notice) if it no longer matches the slot's stored checksum (+0x10),
// else succeed. Same name-fallback (g_emptyString) and BuildLevelRezPath shape
// as Register.
// @early-stop
// EH-frame wall (same as Register, ~45%): retail builds the local CString name
// temp WITHOUT a /GX unwind frame and never destroys it (no fs:0 prologue, no
// ~CString); the faithful `CString s(name)` forces MSVC5 to emit the EH prolog +
// dtor cleanup, shifting the frame. Field reads, name fallback, BuildLevelRezPath
// args, both error branches and the checksum compare are all exact - only the
// extra frame differs. Deferred to the final sweep.
RVA(0x000e52c0, 0x99)
i32 CSaveGame::VerifySlot(SaveSlot* slot) {
    if (slot == 0) {
        return 0;
    }
    i32 fc = slot->m_pathHi;
    i32 f8 = slot->m_pathLo;
    const char* name = (fc == 0 && f8 == 0) ? g_emptyString : slot->m_levelName;
    CString s(name);
    i32 r = g_gameReg->BuildLevelRezPath(fc == 0, fc, f8, slot->m_levelId);
    if (r == 0) {
        g_gameReg->LogError(
            "The level that this game was saved on does not exist!\n\nThis "
            "saved game cannot be loaded and should be deleted."
        );
        return 0;
    }
    if (slot->m_checksum != r) {
        g_gameReg->LogError(
            "The level that this game was saved on has changed!\n\nThis "
            "saved game cannot be loaded and should be deleted."
        );
        return 0;
    }
    return 1;
}

// ---------------------------------------------------------------------------
// CSaveGame::Register  (0x000e5390)
// Build a CString from the slot's name (or the empty string) and hand the slot's
// level id / flags to g_gameReg->BuildLevelRezPath().
// @early-stop
// EH-frame wall (~45%): retail builds the local CString temp WITHOUT a /GX unwind
// frame and never destroys it (no fs:0 prologue, no ~CString); the reconstructed
// `CString s(name)` local forces MSVC5 to emit the __EH_prolog + dtor cleanup.
// Field reads, name-fallback (g_emptyString) and the BuildLevelRezPath args are
// all exact - only the missing/extra frame differs. Deferred to the final sweep.
RVA(0x000e5390, 0x59)
i32 CSaveGame::Register(SaveSlot* slot) {
    if (slot == 0) {
        return 0;
    }
    i32 fc = slot->m_pathHi;
    i32 f8 = slot->m_pathLo;
    const char* name = (fc == 0 && f8 == 0) ? g_emptyString : slot->m_levelName;
    CString s(name);
    return g_gameReg->BuildLevelRezPath(fc == 0, fc, f8, slot->m_levelId);
}

// ---------------------------------------------------------------------------
// CSaveGame::Encode  (0x000e5410)
// Running XOR-fold checksum over a 0x100-byte slot (forward key). Checksums the
// PLAINTEXT byte (pre-XOR) then writes the XOR'd byte back.
// @early-stop
// regalloc wall (~89%): retail gratuitously saves edi (push edi) and holds the
// reloaded plaintext byte there (spill slot [esp+0xc]); recompile keeps it in the
// volatile edx (spill slot [esp+0x8], no edi save). Logic + spill-reload idiom
// exact, only the temp's register/slot differs.
RVA(0x000e5410, 0x3d)
i32 CSaveGame::Encode(u8* buf) {
    if (buf == 0) {
        return 0;
    }
    i32 acc = 0;
    for (u32 i = 0; i < 0x100; i++) {
        u8 t = buf[i];
        buf[i] = (u8)(t ^ i);
        acc += (i32)(t & 0xff) * (i32)i;
    }
    return acc;
}

// ---------------------------------------------------------------------------
// CSaveGame::Decode  (0x000e5460)
// @early-stop
// regalloc-tiebreak churn (~84%): body byte-identical to the pre-pristine 100%
// match; the pristine field renames elsewhere in the TU perturbed MSVC5's
// identifier-interning-driven register coloring, tipping this Encode/Decode
// checksum-loop family's fragile edi/edx spill choice (same wall as Encode). Not
// source-steerable; deferred to the final sweep (recover the edi/edx pin).
RVA(0x000e5460, 0x3f)
i32 CSaveGame::Decode(u8* buf) {
    if (buf == 0) {
        return 0;
    }
    i32 acc = 0;
    for (u32 i = 0; i < 0x100; i++) {
        u8 t = (u8)(i ^ buf[i]);
        buf[i] = t;
        acc += (i32)(t & 0xff) * (i32)i;
    }
    return acc;
}

// ---------------------------------------------------------------------------
// CSaveGame::GetSlot  (0x000e54b0)
// Bounds-checked accessor into the +0xa24 record array.
RVA(0x000e54b0, 0x1f)
SaveSlot* CSaveGame::GetSlot(i32 i) {
    if (i < 0 || i >= 10) {
        return 0;
    }
    return &m_slots[i];
}

// ---------------------------------------------------------------------------
// CSaveGame::FillSlotByIndex  (0x000e54e0)
RVA(0x000e54e0, 0x25)
i32 CSaveGame::FillSlotByIndex(i32 idx, i32 name, void* src) {
    return FillSlot2(GetSlot(idx), name, src);
}

// CSaveGame::StoreSlot  (0x000e5520) - copy `src` into the slot at index `idx`.
RVA(0x000e5520, 0x20)
i32 CSaveGame::StoreSlot(i32 idx, const SaveSlot* src) {
    return CopySlot(GetSlot(idx), src);
}

// A save-slot record probed by the two temp-file helpers below: an int/flag at
// +0x00 (bit0 = "has a temp file"; cleared to 0 by the closer) and the temp-file
// path string at +0x35. Only these two offsets are touched.
SIZE_UNKNOWN(SaveTempRec);
struct SaveTempRec {
    i32 m_flags;        // +0x00  flags (bit0) / cleared to 0 by the closer
    char m_pad04[0x31]; // +0x04..+0x34
    char m_path[1];     // +0x35  the temp-file path
};

// DeleteFileA wrapper at 0x1bf559 (__stdcall; throws the OS error on failure).
extern "C" void __stdcall FileDelete_1bf559(char* lpszFileName);

// ---------------------------------------------------------------------------
// CloseTempFile  (0x000e5550) - if the record's temp file opens (read), close
// and delete it, then clear the record's flag. Returns 1 once the record was
// processed (0 only for a null record). Free __stdcall helper (callee-cleans).
RVA(0x000e5550, 0x9a)
int __stdcall CloseTempFile_e5550(SaveTempRec* p) {
    if (p == 0) {
        return 0;
    }
    CFileIO file;
    if (file.Open(p->m_path, 0, 0)) {
        file.Close();
        FileDelete_1bf559(p->m_path);
    }
    p->m_flags = 0;
    return 1;
}

// CSaveGame::SetMaxLevel (0x000e5620) is now an inline member in the header.


// CSaveGame::SetCurLevel (0x000e5660) is now an inline member in the header.


// ---------------------------------------------------------------------------
// CSaveGame::CheckMagic  (0x000e5690)
RVA(0x000e5690, 0xf)
i32 CSaveGame::CheckMagic() {
    i32 v = m_magic;
    return v == 0x42a;
}

// ---------------------------------------------------------------------------
// CSaveGame::SetMagic  (0x000e56b0) - stamp the save-header magic 0x42a at +0x20.
// Spatially re-homed from src/Stub/BoundaryLowerThunks.cpp (was CSettere56b0::Set);
// dissolved onto CSaveGame (m_magic@+0x20 is the field CheckMagic reads).
RVA(0x000e56b0, 0x8)
void CSaveGame::SetMagic() {
    m_magic = 0x42a;
}

// ---------------------------------------------------------------------------
// TempFileExists  (0x000e5700) - probe whether the record's flagged temp file
// can be opened for read: if bit0 is set and the path opens, close it and return
// 1, else 0. Free __cdecl helper (caller cleans the argument).
RVA(0x000e5700, 0x9e)
int TempFileExists_e5700(SaveTempRec* p) {
    if (p != 0 && (p->m_flags & 1)) {
        CFileIO file;
        if (file.Open(p->m_path, 0, 0)) {
            file.Close();
            return 1;
        }
    }
    return 0;
}

// ---------------------------------------------------------------------------
// The two per-slot dialog labellers (re-homed from src/Stub/ApiCallers.cpp - they
// walk this file's CSaveGame::GetSlot() records). __cdecl(hWnd, item, id3..id6):
// label the slot's short name (m_name) into id3, "(Empty)" when IsSlotOccupied (0x2694 ->
// 0xe5700 slot-occupancy probe) fails; then set the four control enables.
// 0x9e2d0 (GAME_INFO dialog variant): all four enables track occupancy.
RVA(0x0009e2d0, 0x84)
void LabelGameInfoSlot(HWND hWnd, SaveSlot* item, i32 id3, i32 id4, i32 id5, i32 id6) {
    i32 flag;
    if (IsSlotOccupied(item)) {
        SetDlgItemTextA(hWnd, id3, item->m_name);
        flag = 1;
    } else {
        SetDlgItemTextA(hWnd, id3, "(Empty)");
        flag = 0;
    }
    EnableWindow(GetDlgItem(hWnd, id3), flag);
    EnableWindow(GetDlgItem(hWnd, id4), flag);
    EnableWindow(GetDlgItem(hWnd, id5), flag);
    EnableWindow(GetDlgItem(hWnd, id6), flag);
}

// 0xe3e80 (save dialog variant): first two enables unconditional, last two
// track occupancy.
RVA(0x000e3e80, 0x86)
void LabelSaveSlot(HWND hWnd, SaveSlot* item, i32 id3, i32 id4, i32 id5, i32 id6) {
    i32 flag;
    if (IsSlotOccupied(item)) {
        SetDlgItemTextA(hWnd, id3, item->m_name);
        flag = 1;
    } else {
        SetDlgItemTextA(hWnd, id3, "(Empty)");
        flag = 0;
    }
    EnableWindow(GetDlgItem(hWnd, id3), 1);
    EnableWindow(GetDlgItem(hWnd, id4), 1);
    EnableWindow(GetDlgItem(hWnd, id5), flag);
    EnableWindow(GetDlgItem(hWnd, id6), flag);
}

// ---------------------------------------------------------------------------
// The save-flow "show" blit (0x0d00a0), re-homed from the ApiCaller stubs: the
// object reached in the savegame temp-file path (via the 0x3b48 thunk from
// TempFileExists_e5700). Copies its source object's RECT ([m_c->m_24 + 0x10])
// into a stack rect and blits it (mode 8, flags 0x10) into the +0x5c draw sink.
// A placeholder host whose concrete class is not yet recovered; offsets + code
// bytes load-bearing.
struct BlitDrawOwner {
    char m_pad0[0x5c];
    CFontConfig* m_5c; // +0x5c
};
struct BlitRectSrc {
    char m_pad0[0x24];
    char* m_24; // +0x24 (its [+0x10] is the source RECT)
};
struct BlitHost {
    char m_pad0[4];
    BlitDrawOwner* m_4; // +0x04
    char m_pad8[0xc - 8];
    BlitRectSrc* m_c; // +0x0c
    void Show(i32 arg);
};
SIZE_UNKNOWN(BlitDrawSink);
SIZE_UNKNOWN(BlitDrawOwner);
SIZE_UNKNOWN(BlitRectSrc);
SIZE_UNKNOWN(BlitHost);
RVA(0x000d00a0, 0x5a)
void BlitHost::Show(i32 arg) {
    RECT src = *(RECT*)(m_c->m_24 + 0x10);
    RECT dst;
    CopyRect(&dst, &src);
    m_4->m_5c->winapi_022360_DrawTextA_SelectObject_SetTextColor(8, arg, (i32)&dst, 0x10);
}

// Class-metadata annotations (EOF-hosted).
SIZE(SaveSlot, 0x100);   // 0x100-byte slot record (m_slots[] array stride)
SIZE_UNKNOWN(CSaveGame); // fully modeled but tail not proven; owner may upgrade
