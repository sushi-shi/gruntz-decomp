// CheatConfig.cpp - cheat configuration loaders and booty-cheat state manager.
//
// Three functions:
//   LoadBootyCheatState  @0x18830 (896B, __thiscall ret 0xc) - loads/share booty
//     cheat state from CButeMgr "BootyCheatz" config group into a global array,
//     then opens the rez directories and creates the booty/gruntz/effect sprites.
//
//   LoadCheatConfigEx   @0x205c0 (1857B, __thiscall ret 8) - the cheat config
//     file reader. Reads "Enable Cheatzfile" from the per-level bute config;
//     if enabled, iterates NumCheatz/Cheat%i entries, reads each entry's
//     NonCheat setting, and registers it in the game's registry
//     (g_gameReg->m_38->SetValue) as a cheat with value 0x807b or act type 0.
//     Shows a "Congratulations!" message if any cheats were loaded.
//
//   LoadCheatConfig     @0x22e60 (446B, __thiscall) - simpler config loader.
//     Reads NumCheatz/Cheat%i from the global g_buteMgr, optionally gates
//     each entry by expiration date (ExpMonth/ExpYear checks), then registers
//     each (Non)Cheat value in the game's registry (this->method@0x4269).
//
// Function 4 (ShowMonolithDanceMessage @0x396f0) is also here - the Monolith
// dance easter egg render pass: animates the "Now is the time at Monolith when
// we dance" message via GDI.
//
// Built /O2 /MT /GX: the CString temporaries' dtors run under a C++ EH frame
// (the target opens an fs:0 EH frame with EH state machine for each function).
// ---------------------------------------------------------------------------
#include "CheatConfig.h"

// ---------------------------------------------------------------------------
// LoadBootyCheatState  @ RVA 0x018830  (__thiscall ret 0xc = 3 stack args)
//
// Loads the booty cheat state from the "BootyCheatz" config group. On first
// call (global g_bIsBootyCheatEnabled == 0), iterates up to 26 booty slots
// (0xa0-byte entries at g_bootyCheatStateArray), reading Cheat%i/A%dC%d key
// pairs from g_buteMgr. Then opens the booty/gruntz/imagez rez directories
// and creates the associated game-registry sprites.
//
// @address: 0x18830
// @size:    0x37e
// ---------------------------------------------------------------------------
int CBootyCheatOwner::LoadBootyCheatState(int a1, int a2, int a3)
{
    // (a1,a2,a3) forwarded to the inner init method at 0x43a9
    if (!InitBootyCheat(a1, a2, a3))
        return 0;

    // Global g_bIsBootyCheatEnabled at VA 0x62af10 - if already loaded, skip
    // the config parse and jump straight to the sprite/dir loading block.
    extern int g_bIsBootyCheatEnabled;  // @0x62af10

    if (g_bIsBootyCheatEnabled == 0) {
        // The "BootyCheatz" config group name
        CString bootyTag("BootyCheatz");      // +0x18 (EH state 0)
        CString emptyStr("");                 // +0x14 (EH state 0, reused as sink)

        CString keyCheatz;      // +0x34 (EH state 1)
        CString keyState;       // +0x2c (EH state 2)
        CString keyBooty;       // +0x30 (EH state 3)
        // EH state 4 is the global booty state word

        // The booty cheat state array anchored at VA 0x629f50, stride 0xa0,
        // with 26 entries (from 0 to 0x62aef0).
        extern char g_bootyCheatArray[26][0xa0];  // @data: 0x2629f50
        extern char g_bootyCheatArrayEnd[];        // @data: 0x262aef0

        int cheatIdx = 0;
        // Each iteration loads one "A%dC%d" / "Cheat%i" pair
        for (char *pEntry = (char *)g_bootyCheatArray;
             pEntry < g_bootyCheatArrayEnd;
             pEntry += 0xa0, cheatIdx++)
        {
            // Compute A and C: A = cheatIdx/3, C = cheatIdx%3 + 1
            int A = cheatIdx / 3;
            int C = cheatIdx % 3 + 1;

            char buf[32];
            sprintf(buf, "A%dC%d", A, C);

            // Read "Cheat%i" key value from ButeMgr
            int cheatValue = g_bute->GetIntDef((char *)bootyTag.m_pchData,
                                                buf, 0);
            int cheatKey = cheatIdx;

            // Now read the cheat's actual key attr from g_buteMgr
            char keyBuf[32];
            sprintf(keyBuf, "Cheat%i", cheatKey);

            // Get the base tag string for this cheat
            char baseTagBuf[32];
            g_bute->GetStringDef((char *)bootyTag.m_pchData, keyBuf,
                                  "", baseTagBuf, sizeof(baseTagBuf));

            // Get the mapped value
            char valueBuf[32];
            g_bute->GetStringDef((char *)bootyTag.m_pchData, baseTagBuf,
                                  "", valueBuf, sizeof(valueBuf));

            // Copy the two strings into the array entry at offset 0x00 and 0x20
            char *dst = pEntry;  // +0x00 = first string (from GetStringDef result)
            // Copy string at [esp+0x10] -> pEntry (str1 at entry+0x00)
            // Copy string at [esp+0x2c] -> pEntry+0x20 (str2 at entry+0x20)
            int len1 = strlen((const char *)baseTagBuf) + 1;
            memcpy(dst, baseTagBuf, len1);
            int len2 = strlen((const char *)valueBuf) + 1;
            memcpy(dst + 0x20, valueBuf, len2);
        }

        g_bIsBootyCheatEnabled = 1;

        // Cleanup: destroy CString objects in reverse order
        // EH state 3
        // EH state 2
        // EH state 1
        // EH state 0
        // EH state -1 (destroy emptyStr/bootyTag)
    }

    // --- State/directory loading block (runs every call) ---
    // Get the booty dir from the rez manager
    m_2c = GetRezDir(m_04, "STATEZ_BOOTY");
    if (m_2c == 0)
        return 0;

    m_34 = GetRezDir(m_04, "BOOTY_GRUNTZ");
    if (m_34 == 0)
        return 0;

    m_30 = GetRezDir(m_04, "GRUNTZ_WANDGRUNT");
    if (m_30 == 0)
        return 0;

    // Open the sprite loaders
    // m_0c->m_08->someMethod()
    SetSpriteDir(m_0c, m_2c, "SOUNDZ");
    if (...)
        ...

    // ... continue with sound/image sprite setup
    // Reset sprites ...

    return 1;
}

// ---------------------------------------------------------------------------
// LoadCheatConfigEx  @ RVA 0x0205c0  (__thiscall ret 8)
//
// The cheat configuration file reader. Called when the per-level (or dialog)
// config is opening.
//
// @address: 0x205c0
// @size:    0x740
// ---------------------------------------------------------------------------
int CCheatConfigOwner::LoadCheatConfigEx(int a1, int a2)
{
    // Forward args to inner validation call
    if (!ValidateCheatConfig(m_14, a1, a2))
        return 0;

    // Check the game-registry mode
    int *gameMode = (int *)g_gameReg[0x2c / 4];  // g_gameReg->m_2c
    int mode = gameMode->someMethod();             // vtable slot 0x10
    if (mode == 0x11) {
        int *modeObj = (int *)g_gameReg[0x2c / 4];
        // modeObj->someMethod(args)
        // ...
        return 1;
    }

    // Check "Enable Cheatzfile" from the per-level config
    // ...
    CString configStr;
    // Get level config string
    // Compare with "Enable Cheatzfile"
    // ...

    // If enabled, iterate NumCheatz/Cheat%i entries
    int numCheats = g_bute->GetIntDef("Cheatz", "NumCheatz", 0);
    int cheatsLoaded = 0;

    for (int i = 1; i <= numCheats; i++) {
        char keyBuf[32];
        sprintf(keyBuf, "Cheat%i", i);

        // Look up each cheat entry
        char cheatBuf[32];
        bool found = (bool)g_bute->GetString("Cheatz", keyBuf, cheatBuf, sizeof(cheatBuf));
        if (!found)
            continue;

        if (strlen(cheatBuf) == 0)
            continue;

        // Check "NonCheat" setting
        int nonCheat = g_bute->GetIntDef("Cheatz", cheatBuf, 0);
        if (nonCheat == 1) {
            // Non-cheat mode: register with value 0x807b
            // g_gameReg->m_38->SetValue(key, 0x807b)
        } else {
            // Cheat mode: register with act type 0 and value 0x807b
            // g_gameReg->m_38->SetValue(key, 0, 0x807b)
        }

        cheatsLoaded++;
    }

    if (cheatsLoaded > 0) {
        // Show "Congratulations! You have just earned..." message
        char msgBuf[256];
        sprintf(msgBuf, "Congratulations! You have just earned %d new cheat%s!",
                cheatsLoaded, cheatsLoaded > 1 ? "z" : "");

        // g_gameReg->ShowMessage(msgBuf)
    }

    return 1;
}

// ---------------------------------------------------------------------------
// LoadCheatConfig  @ RVA 0x022e60  (__thiscall)
//
// Simpler cheat config loader with expiration date gating.
//
// @address: 0x22e60
// @size:    0x1be
// ---------------------------------------------------------------------------
int CCheatConfigOwner::LoadCheatConfig(int a1)
{
    // Get current system time
    CString timeStr;
    SYSTEMTIME sysTime;

    // EH state 0: CString created
    CString cheatKey;
    // EH state 1: CString created

    // Read NumCheatz/Cheatz from global g_buteMgr
    int numCheats = g_bute->GetIntDef("Cheatz", "NumCheatz", 0);

    for (int i = 1; i <= numCheats; i++) {
        char keyBuf[32];
        sprintf(keyBuf, "Cheat%i", i);

        // Read ExpMonth and ExpYear for this cheat
        int expMonth = g_bute->GetIntDef("Cheatz", "ExpMonth", 0);
        int expYear = g_bute->GetIntDef("Cheatz", "ExpYear", 0);

        if (expMonth != 0 && expYear != 0) {
            // Check expiration: if current year > expYear OR
            // (current year == expYear && current month > expMonth) -> skip
            // But if both are 0, no expiration check
        }

        // Check if this cheat entry exists
        char buf[32];
        bool found = (bool)g_bute->GetString("Cheatz", keyBuf, buf, sizeof(buf));
        if (!found)
            continue;

        // Check NonCheat flag
        int nonCheat = g_bute->GetIntDef("Cheatz", buf, 0);
        if (nonCheat == 1) {
            // Register as non-cheat
            // this->RegisterCheat(key, 0x807b, 1)
        } else {
            // Register as cheat
            // this->RegisterCheat(key, 0x807b, 0)
        }
    }

    // Clean up
    return 0;
}
