// NetSession.cpp - CNetMgr DirectPlay session helpers (the 0x0b8xxx/0x0f9xxx
// "player NAME" cluster). These are __cdecl free helpers the CNetMgr session
// code calls (reached via incremental-link thunks); none take a `this`.
//
// Matched here:
//   0x0f93b0  AppendInt - sprintf an int with "%i" into a scratch buffer, then
//             forward (dst, sep, buf) to the engine string-builder (0xf9280).
//   0x0b89e0  FillPlayerList - clear a listbox then repopulate it from the
//             session's player CObList (a name per node + the node as item data).
#include <Net/NetMgr.h> // <Mfc.h> -> SendMessageA / HWND / LB_* (reloc-masked)
#include <stdio.h>      // engine sprintf (reloc-masked)
#include <rva.h>

// The engine 3-arg string builder at 0x0f9280 (__cdecl, returns 1): concatenates
// a fixed prefix + `a` + `b` into `dst`. External to this TU (reloc-masked).
extern "C" i32 NetStrBuild(char* dst, const char* a, const char* b);

// The engine 3-arg keyed string-format helper at 0x0f9160 (__cdecl): formats a
// value for `key` into `out` and returns nonzero when it produced a string (else
// the caller falls back to the raw source). External (reloc-masked).
extern "C" i32 NetFormatKeyed(char* out, void* src, const char* key);

// ---------------------------------------------------------------------------
// AppendInt() - 0x0f93b0. Format `n` as decimal into a 256-byte scratch buffer
// ("%i"), then hand (dst, sep, buf) to the string builder. __cdecl, 3 args.
// ---------------------------------------------------------------------------
RVA(0x000f93b0, 0x41)
void AppendInt(char* dst, const char* sep, i32 n) {
    char buf[256];
    sprintf(buf, "%i", n);
    NetStrBuild(dst, sep, buf);
}

// The player record held in each list node: it carries its profile/name source
// at +0x34 (NetFormatKeyed reads NAME out of it, else it is used raw as the text).
struct PlayerRecord {
    char m_pad[0x34];
    char* m_profile; // +0x34  keyed text buffer (NetFormatKeyed reads NAME; else used raw as text)
};
SIZE_UNKNOWN(PlayerRecord); // player-record view (only +0x34 pinned); retail size TBD

// A node of the session's player list (CObList-shaped): next ptr + the held
// player record.
struct PlayerNode {
    PlayerNode* m_next;   // +0x00
    void* m_4;            // +0x04
    PlayerRecord* m_item; // +0x08 (the player record)
};
SIZE_UNKNOWN(PlayerNode); // CObList-shaped list-node view; retail size TBD
// The session container: a player CObList (head at +0x3c) + a scratch POSITION
// cursor the listbox fill walks through (+0x80).
struct Session {
    char m_pad[0x3c];
    PlayerNode* m_head; // +0x3c (GetHeadPosition)
    char m_pad2[0x80 - 0x40];
    PlayerNode* m_pos; // +0x80 (the running POSITION)
};
SIZE_UNKNOWN(Session); // session-container view (only +0x3c/+0x80 pinned); size TBD

// ---------------------------------------------------------------------------
// FillPlayerList() - 0x0b89e0. __stdcall(HWND hList, Session* sess): clear the
// listbox, then for each player node walk the +0x34 profile, format its NAME
// (NetFormatKeyed) into a scratch buffer, add that string (or the raw profile on
// a format miss), and stash the node as the new item's data. No-op if either arg
// is null.
// ---------------------------------------------------------------------------
// @early-stop
// regalloc/aliasing wall (~91.5%): logic byte-exact except the advance block -
// retail reloads sess->m_pos into a 2nd register (ecx) for the ->next read while
// keeping the old position in eax for ->item (`mov ecx,[m_80]; mov eax,[eax+8];
// mov edx,[ecx]`); the recompile keeps the position in one reg and derefs both
// fields from it. Aliasing-conservatism choice MSVC made in retail, not
// source-steerable (cf. linked-list-walk-node-eax-rotation.md). Deferred.
RVA(0x000b89e0, 0xc8)
void FillPlayerList(HWND hList, Session* sess) {
    char buf[256];
    if (!hList) {
        return;
    }
    if (!sess) {
        return;
    }
    SendMessageA(hList, LB_RESETCONTENT, 0, 0);
    PlayerNode* node = sess->m_head;
    sess->m_pos = node;
    PlayerRecord* player;
    if (node) {
        sess->m_pos = node->m_next;
        player = node->m_item;
    } else {
        player = 0;
    }
    while (player) {
        const char* str;
        if (NetFormatKeyed(buf + 4, player->m_profile, "NAME")) {
            str = buf;
        } else {
            str = player->m_profile;
        }
        i32 idx = (i32)SendMessageA(hList, LB_ADDSTRING, 0, (LPARAM)str);
        if (idx != -1) {
            SendMessageA(hList, LB_SETITEMDATA, idx, (LPARAM)player);
        }
        PlayerNode* pos = sess->m_pos;
        if (pos) {
            player = pos->m_item;
            sess->m_pos = pos->m_next;
        } else {
            player = 0;
        }
    }
}
