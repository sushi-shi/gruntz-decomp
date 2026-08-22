#ifndef GRUNTZ_DDRAWMGR_DDRAWSUBMGRLEAFSCANINLINE_H
#define GRUNTZ_DDRAWMGR_DDRAWSUBMGRLEAFSCANINLINE_H

#include <DDrawMgr/DDrawSubMgrLeafScan.h>
#include <Dsndmgr/SoundStream.h>

inline void PurgeVoices(CDDrawSubMgrLeafScan* registry) {
    if (registry->m_soundStream != NULL) {
        registry->m_soundStream->PurgeVoiceList(-1);
    }
}

#endif // GRUNTZ_DDRAWMGR_DDRAWSUBMGRLEAFSCANINLINE_H
