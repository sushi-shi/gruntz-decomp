#include <rva.h>
#include <Dsndmgr/DirectSoundMgr.h>
#include <Dsndmgr/SoundVoiceList.h>

#include <Gruntz/SoundCueMgr.h>
// CStatusBarMgrGetItem.cpp - CSoundCueMgr::GetItem in its OWN dedicated unit.
//
// GetItem (0x135d70) is the Dsndmgr sound-cue manager's pooled-buffer resolver: it
// walks the +0x58 item list for a live/finished DirectSound buffer, reconfigures it
// (pan/pitch/volume from m_18/m_1c/m_20) and, when none is free, Create()s a fresh
// one, then unlinks + re-appends it to the +0x58 list.
//
// Its owner CSoundCueMgr was rtti-mislabeled "CStatusBarMgr"; it is a GENUINELY
// DIFFERENT class from the Gruntz HUD status-bar builder (CStatusBarMgr,
// src/Gruntz/StatusBarMgr.cpp) - the +0x10 pointer + the +0x58 list head that
// overlaps the HUD builder's +0x48 CPtrList make one folded layout impossible. See
// <Gruntz/SoundCueMgr.h>. Kept in its own unit; flags="eh" (matching-neutral).

// @confidence: med
// @source: reloc-correlation (1 caller)
// @early-stop
// shrink-wrapped callee-save push wall (~90%): logic + offsets + externs byte-exact.
// Retail saves only edi at entry and defers `push esi`/`push ebx` past the m_78 null
// guard (the early-out restores just edi); cl pushes all three upfront. Not source-
// steerable; docs/patterns/shrink-wrapped-callee-save-push.md. Final sweep.
RVA(0x00135d70, 0x92)
CStatusBarItem2* CSoundCueMgr::GetItem() {
    if (!m_10->m_78) {
        return 0;
    }
    SBNode* node = m_58.m_head;
    if (node) {
        while (1) {
            if (node->m_8->m_50 && ((DirectSoundMgr*)node->m_8)->IsPlaying() == 0) {
                break;
            }
            node = node->m_0;
            if (!node) {
                break;
            }
        }
    }
    CStatusBarItem2* found;
    if (!node) {
        found = 0;
    } else {
        found = node->m_8;
    }
    if (found) {
        ((DirectSoundMgr*)found)->SetVolume(m_20);
        ((DirectSoundMgr*)found)->SetPan(m_1c);
        ((DirectSoundMgr*)found)->SetFrequency(m_18);
    }
    if (!found) {
        found = Create(1);
        if (!found) {
            return found;
        }
    }
    ((DSoundList*)&m_58)->Unlink((DSoundLink*)&found->m_link44);
    ((DSoundList*)&m_58)->InsertTail((DSoundLink*)&found->m_link44);
    return found;
}
