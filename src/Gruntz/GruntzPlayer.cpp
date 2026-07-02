// GruntzPlayer.cpp - the per-player options/state record CGruntzMgr embeds at
// +0x150 (a 4-element array; each slot 0x238 bytes). The slot carries a name
// CString (default "Player"), a scalar config block, an embedded
// CBattlezSpawnMgr_or_CGruntSpawnMgr config bundle at +0x38, and a trailing scalar block. See
// <Gruntz/GruntzPlayer.h> for the layout.
//
// The class is non-polymorphic (no vtable). The five reconstructed members:
//   GetName        @0x01f450 (0x20  B) - return the name CString by value (NRV).
//   ctor(index)    @0x0da870 (0xb8  B) - /GX: seed the name with "Player" + the
//                                        scalar config block from `index`.
//   ctor()         @0x0da960 (0x5b  B) - frameless default: empty name + zeros.
//   Serialize      @0x0dace0 (0x239 B) - stream every field through the archive
//                                        order object (kind 4 = save, 7 = load),
//                                        then forward the 4-arg command to +0x38.
//   GetDefaultName @0x0dafb0 (0x71  B) - /GX static: return the literal "Player".
//
// Field names are placeholders; only OFFSETS + code bytes are load-bearing.
// ---------------------------------------------------------------------------
#include <rva.h>

#include <Gruntz/GruntzPlayer.h>

#include <string.h> // inlined memset / strcpy in Serialize (rep stos / rep movs)
#include <Globals.h>

// The MFC empty C string (the afxEmptyString data buffer @0x6293f4); the name
// CString members default-init to it. Reloc-masked DATA.
extern "C" char g_emptyString[]; // 0x6293f4

// Per-serialize round counter the CString archive helpers bump (g_serialCounter,
// = ?g_serialCounter@@3HA @0x629ad0). Reloc-masked DATA.
DATA(0x00629ad0)
extern i32 g_serialCounter;

// The per-player config name tables the two free getters below index by enum.
// Each is an array of char* into the rodata string pool. Reloc-masked DATA.

// The archive/order object Serialize streams through. Its field-transfer entries
// are the virtuals at vtable byte-offsets 0x2c (Load) and 0x30 (Save), each taking
// a buffer ptr + a byte count. Modeled polymorphic (slot decls only, never defined
// -> no ??_7) so `ar->Save(buf,n)` lowers to the exact
// `mov eax,[ar]; push n; push buf; mov ecx,ar; call [eax+0x30]` __thiscall dispatch.
struct PlayerArchive {
    virtual void Slot00();
    virtual void Slot04();
    virtual void Slot08();
    virtual void Slot0C();
    virtual void Slot10();
    virtual void Slot14();
    virtual void Slot18();
    virtual void Slot1C();
    virtual void Slot20();
    virtual void Slot24();
    virtual void Slot28();
    virtual void Load(void* buf, i32 n); // +0x2c
    virtual void Save(void* buf, i32 n); // +0x30
};

// The embedded CBattlezSpawnMgr_or_CGruntSpawnMgr config bundle at this+0x38: Serialize forwards
// its 4-arg command to the bundle's Method_02bfc0 (thunk 0x2df1 -> 0x2bfc0), a
// __thiscall returning nonzero on success. Modeled as a tiny helper laid over
// (this+0x38) so the `mov ecx,this+0x38; call` lowers + reloc-masks (no body).
struct PlayerConfigBundle {
    i32 Command(void* ar, i32 kind, i32 a3, i32 a4); // 0x2bfc0
};

// ===========================================================================
// GruntzPlayer::GetName  @0x01f450
// Return the name CString by value (NRV-construct the return slot as a copy of
// m_name). The dead `[esp+4]=0` store is the MSVC5 NRV bookkeeping artifact that
// `return m_name;` reproduces.
// ===========================================================================
RVA(0x0001f450, 0x20)
CString GruntzPlayer::GetName() {
    return m_name;
}

// ===========================================================================
// GruntzPlayer::GruntzPlayer(i32)  @0x0da870  (/GX EH frame)
// Seed the name with "Player" (the temp from GetDefaultName() assigned into the
// member) and the scalar config block from the index. The /GX frame comes from
// the destructible "Player" temp.
// ===========================================================================
// @early-stop
// MFC-version wall (cstring-empty-init-version-divergence.md): the field stores +
// GetDefaultName call + op= are byte-correct, but the packaged MFC's
// CString::CString() is out-of-line, so our base emits an extra ??0CString@@QAE@XZ
// default-ctor call before the m_name = g_emptyString op= where retail elides it
// (its inline CString() folds + dead-store-eliminates). Retail also reads fs:0
// before push -1 in the /GX prologue where ours pushes first. Both are static-MFC
// build artifacts, not source-steerable. Deferred to the final sweep.
RVA(0x000da870, 0xb8)
GruntzPlayer::GruntzPlayer(i32 index) {
    m_name = g_emptyString;
    m_playerIndex = index;
    m_018 = -2;
    m_020 = 0;
    m_028 = 0;
    m_014 = 1;
    m_name = GetDefaultName();
    m_008 = index;
    m_010 = 0;
    m_220 = 0;
    m_224 = 0;
    m_228 = 0xf;
    m_02c = 0;
    m_030 = 0;
    m_22c = 0;
    m_230 = 0;
}

// ===========================================================================
// GruntzPlayer::GruntzPlayer()  @0x0da960
// Frameless default ctor: empty name, the scalar config block zeroed (with the
// sentinel -1 / 1 / -2 / 0xf seeds).
// ===========================================================================
// @early-stop
// MFC-version wall (cstring-empty-init-version-divergence.md): all field stores +
// the m_name = g_emptyString op= are byte-correct, but the packaged MFC's
// out-of-line CString::CString() default-ctor call ALSO drags in a /GX EH frame
// (the member becomes throwing-destructible during construction) that retail lacks
// entirely - retail's inline CString() folds to nothing and the ctor is frameless,
// so our base is larger than retail's 0x5b and objdiff can't align it. Static-MFC
// build artifact, not source-steerable. Deferred to the final sweep.
RVA(0x000da960, 0x5b)
GruntzPlayer::GruntzPlayer() {
    m_playerIndex = -1;
    m_name = g_emptyString;
    m_018 = -2;
    m_020 = 0;
    m_014 = 1;
    m_008 = 0;
    m_010 = 0;
    m_220 = 0;
    m_224 = 0;
    m_228 = 0xf;
    m_02c = 0;
    m_030 = 0;
    m_22c = 0;
    m_230 = 0;
}

// ===========================================================================
// GruntzPlayer::Reset  @0x0da9e0
// Frameless re-init of an already-live slot: re-empty the name CString (op= to
// the MFC empty string, NO default ctor / no EH frame since the member is already
// constructed) and re-seed the scalar config block. Returns 1 (success).
// ===========================================================================
// @early-stop
// store-scheduling wall: every instruction (the op= to g_emptyString + all 14 field
// stores) is byte-correct, but MSVC's scheduler floats the m_228 = 0xf IMMEDIATE
// store to the tail of the register-store cluster, where retail keeps it in source
// position (between m_224 and m_02c). Reordering the source / hoisting to a local
// does not flip it (the scheduler re-floats the imm). ~94.9%.
RVA(0x000da9e0, 0x60)
i32 GruntzPlayer::Reset() {
    m_playerIndex = -1;
    m_018 = -2;
    m_020 = 0;
    m_014 = 1;
    m_name = g_emptyString;
    m_008 = 0;
    m_010 = 0;
    m_220 = 0;
    m_224 = 0;
    m_228 = 0xf;
    m_02c = 0;
    m_030 = 0;
    m_22c = 0;
    m_230 = 0;
    return 1;
}

// ===========================================================================
// GruntzPlayer::Serialize  @0x0dace0
// Stream every field through the archive order object. kind 7 = Load (read each
// scalar via [+0x2c], then load the 0x80 name buffer and assign it into m_name),
// kind 4 = Save (write each scalar via [+0x30], then the inlined memset+strcpy of
// the name into a 0x80 buffer and write it). Either way, forward the 4-arg command
// to the +0x38 config bundle and negate (!!) its result.
// ===========================================================================
RVA(0x000dace0, 0x239)
i32 GruntzPlayer::Serialize(void* arArg, i32 kind, i32 a3, i32 a4) {
    PlayerArchive* ar = (PlayerArchive*)arArg;
    char tmp[0x80];
    // Retail lays the kind==4 (Save, [+0x30]) arm out of line and keeps the
    // kind==7 (Load, [+0x2c]) arm inline: `cmp 4; je SAVE / cmp 7; jne TAIL`.
    if (kind != 4) {
        if (kind == 7) {
            // Load.
            ar->Load(&m_playerIndex, 4);
            ar->Load(&m_008, 4);
            ar->Load(&m_00c, 4);
            ar->Load(&m_010, 4);
            ar->Load(&m_014, 4);
            ar->Load(&m_018, 4);
            ar->Load(&m_01c, 4);
            ar->Load(&m_020, 4);
            ar->Load(&m_028, 4);
            ar->Load(&m_024, 4);
            g_serialCounter++;
            ar->Load(tmp, 0x80);
            m_name = tmp;
            ar->Load(&m_220, 4);
            ar->Load(&m_224, 4);
            ar->Load(&m_228, 4);
        }
    } else {
        // Save.
        ar->Save(&m_playerIndex, 4);
        ar->Save(&m_008, 4);
        ar->Save(&m_00c, 4);
        ar->Save(&m_010, 4);
        ar->Save(&m_014, 4);
        ar->Save(&m_018, 4);
        ar->Save(&m_01c, 4);
        ar->Save(&m_020, 4);
        ar->Save(&m_028, 4);
        ar->Save(&m_024, 4);
        g_serialCounter++;
        memset(tmp, 0, sizeof(tmp));
        strcpy(tmp, (const char*)m_name);
        ar->Save(tmp, 0x80);
        ar->Save(&m_220, 4);
        ar->Save(&m_224, 4);
        ar->Save(&m_228, 4);
    }
    i32 r = ((PlayerConfigBundle*)&m_038)->Command(ar, kind, a3, a4);
    return r != 0;
}

// ===========================================================================
// GruntzPlayer::GetDefaultName  @0x0dafb0  (/GX EH frame, static)
// Return the literal default player name "Player" by value (NRV); the /GX frame
// comes from the destructible CString("Player") temp.
// ===========================================================================
RVA(0x000dafb0, 0x71)
CString GruntzPlayer::GetDefaultName() {
    // Retail builds a named local temp, then NRV-copies it into the return slot
    // (op= copy-ctor) and destructs the temp -> the /GX frame. A direct
    // `return CString("Player");` would NRV-construct in place (frameless).
    CString name("Player");
    return name;
}

// ===========================================================================
// GetColorName  @0x0db050  (/GX EH frame, free fn)
// Build a CString from the color-name table (g_colorNames[idx]); uppercase it
// when `upper` is set, then return by value (NRV: the local is copy-ctor'd into
// the caller's return slot and destructed). Called by the GruntzPlayer player-
// option accessor at 0x0dab18 (same TU).
// ===========================================================================
RVA(0x000db050, 0x90)
CString GetColorName(i32 colorIdx, i32 upper) {
    CString s;
    s = g_colorNames[colorIdx];
    if (upper) {
        s.MakeUpper();
    }
    return s;
}

// ===========================================================================
// GetDifficultyName  @0x0db110  (/GX EH frame, free fn)
// As GetColorName but over the difficulty table ("Easy"/"Normal"/"Hard").
// Called by the GruntzPlayer accessor at 0x0dac38 (same TU).
// ===========================================================================
RVA(0x000db110, 0x90)
CString GetDifficultyName(i32 diffIdx, i32 upper) {
    CString s;
    s = g_difficultyNames[diffIdx];
    if (upper) {
        s.MakeUpper();
    }
    return s;
}

SIZE_UNKNOWN(PlayerArchive);
SIZE_UNKNOWN(PlayerConfigBundle);
