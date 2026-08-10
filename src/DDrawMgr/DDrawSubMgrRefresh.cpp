#include <rva.h>

#include <Mfc.h>

#include <Bute/SymParser.h>
#include <Bute/SymTab.h>
#include <DDrawMgr/AniAdvance.h>
#include <DDrawMgr/DDrawChildGroup.h>
#include <DDrawMgr/DDrawPtrCollections.h>
#include <DDrawMgr/DDrawSubMgr.h>
#include <DDrawMgr/DDrawSubMgrLeaf.h>
#include <DDrawMgr/DDrawSubMgrLeafScan.h>
#include <DDrawMgr/DDrawSubMgrPages.h>
#include <DDrawMgr/DDrawSurfaceMgr.h>
#include <DDrawMgr/DDrawSurfacePair.h>
#include <DDrawMgr/DDrawWorker.h>
#include <DDrawMgr/DDrawWorkerCache.h>
#include <DDrawMgr/DDrawWorkerHost.h>
#include <DDrawMgr/DDrawWorkerList.h>
#include <DDrawMgr/DDrawWorkerMapSmall.h>
#include <DDrawMgr/DDrawWorkerNode.h>
#include <DDrawMgr/DDrawWorkerRegistry.h>
#include <DDrawMgr/DirectDrawMgr.h>
#include <Dsndmgr/DirectSoundMgr.h>
#include <Dsndmgr/SoundDevice.h>
#include <Dsndmgr/SoundStream.h>
#include <Gruntz/AniAdvanceCursor.h>
#include <Gruntz/AniElement.h>
#include <Gruntz/GameLevel.h>
#include <Gruntz/LeafCue.h>
#include <Gruntz/ParseSource.h>
#include <Gruntz/SerialArchive.h>
#include <Gruntz/SoundState.h>
#include <Gruntz/Sprite.h>
#include <Gruntz/StateId.h>
#include <Image/CImage.h>
#include <Io/FileMem.h>
#include <Rez/FrameClock.h>
#include <Utils/MapTyped.h>
#include <Wap32/Object.h>
#include <Wap32/WapObj.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// @identity-TODO RefreshAsset@CDDrawSubMgrLeafScan - thunk oracle: retail gave this an incremental
// thunk, so it was compiled into a LINK-LINE OBJECT, while the rest of this TU
// (118 fns) came from the static library. It belongs to another compiland.
RVA(0x00114120, 0x70)
i32 CDDrawSubMgrLeafScan::RefreshAsset(const char* key) {
    if (m_emitGate != 0) {
        return 0;
    }
    void* val = 0;
    m_cues.Lookup(key, val);
    if (val == NULL) {
        return 0;
    }
    i32 gate = g_sndEnabled;
    i32 item = g_sndCueTag;
    if (gate == 0) {
        return 0;
    }
    LeafCue* p = static_cast<LeafCue*>(val);

    if (g_killCueClock - static_cast<u32>(p->m_lastPlayTime)
        >= static_cast<u32>(p->m_replayDelay)) {
        p->m_lastPlayTime = g_killCueClock;
        return p->m_sound->ConfigureItem(item, 0, 0, 0);
    }
    return 0;
}
