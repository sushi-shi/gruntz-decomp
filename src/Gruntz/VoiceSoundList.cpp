#include <Mfc.h> // CPtrList/CString machinery (reloc-masked); /GX EH frame
#include <Bute/SymParser.h>

#include <Bute/ButeMgr.h>            // canonical CButeMgr (one shape)
#include <Gruntz/GruntSpawnConfig.h> // canonical CGruntSpawnConfig (this) + CSpawnResolver
#include <Gruntz/SpawnList.h>        // canonical CSpawnList (the built voice list)
#include <rva.h>
// VoiceSoundList.cpp - CGruntSpawnConfig::BuildVoiceSoundList (0x11c210, 669 B):
// build the CSpawnList of valid "VOICES_<dir>[_<sub>]" sound names for speech-group
// `n`, reading the group's DIR / sub-keys from g_buteMgr ([SG<n>]) and keeping only
// the names that resolve to a 'WAV' resource through m_00->m_34 (the owner's
// CSpawnResolver). The four CString temps + the heap CSpawnList node give the
// routine its /GX exception frame.
//
// (The old CVoiceBuilder / VoiceRoot / VoiceResolver / VoiceList views were folded
// onto the canonicals: CVoiceBuilder was CGruntSpawnConfig (m_0 == m_00, the
// CSpawnOwner), VoiceResolver the owner's +0x34 CSpawnResolver, and VoiceList the
// canonical CSpawnList - its AddName was CSpawnList::AddVoiceSound(name, 0), per
// the retail call site `push 0; push <name copy>; call 0x2446->0x11c560`.)
//
// Only offsets / code bytes are load-bearing; engine callees are reloc-masked.

// CButeMgr getter (g_buteMgr @ 0x6453d8): GetStringDef(tag, key, def) returns the
// stored CString (or `def`) - 0x173180 __thiscall. On the canonical CButeMgr.
// g_buteMgr (canonical CButeMgr) comes from <Bute/ButeMgr.h>.

// @early-stop
// /GX EH-state + regalloc wall (~79%): complete body - the n bounds guards, the four
// CString temps, the [SG<n>] DIR/sub-key g_buteMgr reads, the VOICES_<dir>[_<sub>]
// name format, the 'WAV'-resolve filter and the heap CSpawnList accumulation all
// align with retail.  Residual: the per-CString-temp destruct trylevel slot index
// + the descending dtor sweep on the failure path are the documented /GX EH-state
// wall (docs/seh-eh.md).  Deferred to the final sweep.
RVA(0x0011c210, 0x29d)
CSpawnList* CGruntSpawnConfig::BuildVoiceSoundList(i32 n) {
    if (n <= 0) {
        return 0;
    }
    if (n >= 0x4b0) {
        return 0;
    }

    CString dir, scratch, sub, name;
    scratch.Format("SG%i", n);
    scratch = *g_buteMgr.GetStringDef((LPCTSTR)scratch, "DIR", &dir);

    sub.Format("S%i", 1);
    sub = *g_buteMgr.GetStringDef((LPCTSTR)scratch, (LPCTSTR)sub, &dir);

    CSpawnList* list = 0;
    if (!scratch.IsEmpty()) {
        list = new CSpawnList();
    }

    if (!sub.IsEmpty()) {
        i32 i = 1;
        do {
            i++;
            if (sub.IsEmpty()) {
                name.Format("VOICES_%s", (LPCTSTR)scratch);
            } else {
                name.Format("VOICES_%s_%s", (LPCTSTR)scratch, (LPCTSTR)sub);
            }
            void* res = (void*)((CSymParser*)m_owner->m_34)
                            ->ResolveQualified((LPCTSTR)name, (void*)0x574156);
            if (res != 0) {
                // retail copy-ctors `name` straight into the by-value arg slot
                // (push 0; push ecx; mov ecx,esp; copy-ctor) - no local temp.
                list->AddVoiceSound(name, 0);
                sub.Format("S%i", i);
                sub = *g_buteMgr.GetStringDef((LPCTSTR)scratch, (LPCTSTR)sub, &dir);
            } else {
                sub.Empty();
            }
        } while (!sub.IsEmpty());
    }
    return list;
}
