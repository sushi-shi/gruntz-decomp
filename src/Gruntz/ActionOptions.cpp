// ActionOptions.cpp - the action-options menu bar sprite loader.
// LoadActionOptionsMenuBar @0x0090e0 (256 B) - __thiscall on a menu-bar-like
// object (offsets +0x10/+0x2c/+0x30/+0x34/+0x38/+0x3c). Looks up four sprites
// from the global sprite cache (reached through g_gameReg->m_30->m_10+0x10):
// the base bar sprite + three chip-level indicator sprites. Returns 1 on
// success (0 if any lookup fails).
//
// Plain /O2 /MT (no /GX): scalar leaf, no stack C++ object / EH frame.
// ---------------------------------------------------------------------------

// The game registry pointer (the global at VA 0x64556c).
// @data: 0x24556c
extern int *g_gameReg;

// The string-keyed sprite cache lookup (thiscall, ret 8 = 2 stack args).
struct SpriteTable {
    void Lookup(const char *key, void *&result);  // @0x1b8008
};

// The menu-bar class (only the members LoadActionOptionsMenuBar touches are
// modeled; the class name is a project placeholder, not the target's RTTI name).
class ActionOptionsMenuBar {
public:
    int LoadActionOptionsMenuBar();

private:
    // Offsets pinned by the @0x0090e0 disassembly:
    int m_vtbl;                 // +0x00
    char m_pad04[0x10 - 4];     // +0x04..+0x0c
    int  m_framePtr;            // +0x10  extracted frame from bar sprite
    char m_pad14[0x2c - 0x14]; // +0x14..+0x28
    int  m_gate;                // +0x2c  (set to 0 at entry)
    void *m_normChip;           // +0x30  NORMCHIPZ sprite
    void *m_highChip;           // +0x34  HIGHCHIPZ sprite
    void *m_greyChip;           // +0x38  GREYCHIPZ sprite
    int  m_loaded;              // +0x3c  = 1 on success
};

// ---------------------------------------------------------------------------
// LoadActionOptionsMenuBar  @ RVA 0x0090e0  (thiscall, returns int)
//
// @address: 0x0090e0
// @size:    0x100
// ---------------------------------------------------------------------------
int ActionOptionsMenuBar::LoadActionOptionsMenuBar()
{
    void *result = 0;
    m_gate = 0;

    // Derive the sprite-table ptr: g_gameReg->m_30->m_10 + 0x10.
    // The target re-derives it fresh for each lookup (no caching).
    int *m30 = (int *)g_gameReg[0x30 / 4];
    SpriteTable *tbl = (SpriteTable *)((char *)((int *)m30[0x10 / 4]) + 0x10);

    // --- lookup 1: base menu-bar sprite ---
    tbl->Lookup("GAME_ACTIONOPTIONZMENUBAR", result);

    if (result != 0) {
        int *spr = (int *)result;
        if (spr[0x64 / 4] <= 1 && spr[0x68 / 4] >= 1)
            m_framePtr = ((int *)spr[0x14 / 4])[0x04 / 4];
        else
            m_framePtr = 0;
    } else {
        m_framePtr = 0;
    }

    if (m_framePtr == 0)
        return 0;

    // --- lookups 2-4: chip sprites ---
    result = 0;
    m30 = (int *)g_gameReg[0x30 / 4];
    tbl = (SpriteTable *)((char *)((int *)m30[0x10 / 4]) + 0x10);
    tbl->Lookup("GAME_INGAMEICONZ_NORMCHIPZ", result);
    if (result == 0) return 0;
    m_normChip = result;

    result = 0;
    m30 = (int *)g_gameReg[0x30 / 4];
    tbl = (SpriteTable *)((char *)((int *)m30[0x10 / 4]) + 0x10);
    tbl->Lookup("GAME_INGAMEICONZ_HIGHCHIPZ", result);
    if (result == 0) return 0;
    m_highChip = result;

    result = 0;
    m30 = (int *)g_gameReg[0x30 / 4];
    tbl = (SpriteTable *)((char *)((int *)m30[0x10 / 4]) + 0x10);
    tbl->Lookup("GAME_INGAMEICONZ_GREYCHIPZ", result);
    if (result == 0) return 0;
    m_greyChip = result;

    m_loaded = 1;
    return 1;
}
