#include <Mfc.h> // CPtrList/CString machinery (reloc-masked); /GX EH frame
#include <Bute/SymParser.h>

#include <Bute/ButeMgr.h>            // canonical CButeMgr (one shape)
#include <Gruntz/GruntSpawnConfig.h> // canonical CGruntSpawnConfig (this) + CSpawnResolver
#include <Gruntz/SpawnList.h>        // canonical CSpawnList (the built voice list)
#include <rva.h>

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
    scratch = *g_buteMgr.GetStringDef(static_cast<LPCTSTR>(scratch), "DIR", &dir);

    sub.Format("S%i", 1);
    sub = *g_buteMgr.GetStringDef(static_cast<LPCTSTR>(scratch), static_cast<LPCTSTR>(sub), &dir);

    CSpawnList* list = 0;
    if (!scratch.IsEmpty()) {
        list = new CSpawnList();
    }

    if (!sub.IsEmpty()) {
        i32 i = 1;
        do {
            i++;
            if (sub.IsEmpty()) {
                name.Format("VOICES_%s", static_cast<LPCTSTR>(scratch));
            } else {
                name.Format("VOICES_%s_%s", static_cast<LPCTSTR>(scratch), static_cast<LPCTSTR>(sub));
            }
            void* res = reinterpret_cast<void*>((reinterpret_cast<CSymParser*>(m_owner->m_34))
                            ->ResolveQualified(static_cast<LPCTSTR>(name), reinterpret_cast<void*>(0x574156)));
            if (res != 0) {
                // retail copy-ctors `name` straight into the by-value arg slot
                // (push 0; push ecx; mov ecx,esp; copy-ctor) - no local temp.
                list->AddVoiceSound(name, 0);
                sub.Format("S%i", i);
                sub = *g_buteMgr.GetStringDef(static_cast<LPCTSTR>(scratch), static_cast<LPCTSTR>(sub), &dir);
            } else {
                sub.Empty();
            }
        } while (!sub.IsEmpty());
    }
    return list;
}
