#include <rva.h>

#include <Mfc.h>

#include <Bute/ButeMgr.h>
#include <Bute/SymParser.h>
#include <Gruntz/Enums.h>
#include <Gruntz/GruntSpawnConfig.h>
#include <Gruntz/GruntzMgr.h>
#include <Gruntz/SpawnList.h>

// @early-stop
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
                name.Format(
                    "VOICES_%s_%s",
                    static_cast<LPCTSTR>(scratch),
                    static_cast<LPCTSTR>(sub)
                );
            }
            CParseSource* res =
                m_owner->m_symParser->ResolveQualified(static_cast<LPCTSTR>(name), REZ_TAG_WAV);
            if (res != 0) {

                list->AddVoiceSound(name, 0);
                sub.Format("S%i", i);
                sub = *g_buteMgr.GetStringDef(
                    static_cast<LPCTSTR>(scratch),
                    static_cast<LPCTSTR>(sub),
                    &dir
                );
            } else {
                sub.Empty();
            }
        } while (!sub.IsEmpty());
    }
    return list;
}
