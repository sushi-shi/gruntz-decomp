#ifndef SBI_STATZTABGRUNTBAR_H
#define SBI_STATZTABGRUNTBAR_H

#include <rva.h>

#include <Clock64.h>
#include <Gruntz/LogicTypeId.h>
#include <Gruntz/SbGeom.h>
#include <Gruntz/SerialArchive.h>
#include <Gruntz/StatusBarItem.h>
#include <Image/CImage.h>
#include <Ints.h>

#include <stddef.h>

class CStatusBarMgr;
class CDDrawSurfaceMgr;

class CDDrawWorker;

class CSBI_StatzTabGruntBar : public CStatusBarItem {
public:
    CSBI_StatzTabGruntBar() {
        m_timerAnchorLo = 0;
        m_timerWindowLo = 0;
        m_timerAnchorHi = 0;
        m_timerWindowHi = 0;
        m_kind = SBI_KIND_STATZ_TAB_GRUNT_BAR;
        m_statusGlyphLatched = NULL;
        m_abilityGlyphLatched = NULL;
        m_overrideGlyphLatched = NULL;
        m_selectGlyph = NULL;
        m_glyphMap = NULL;
        m_statusGlyph = NULL;
        m_abilityGlyph = NULL;
        m_overrideGlyph = NULL;
        m_selectKey = NULL;
        m_overrideValue = -1;
        m_abilityValue = -1;
        m_statusValue = -1;
        m_selectValue = 0;
        m_timerGlyphMap = NULL;
        m_timerValue = -1;
        m_timerGlyph = NULL;
    }
    virtual ~CSBI_StatzTabGruntBar() OVERRIDE;

    virtual i32 SerializeFields(CFileMemBase* ar, SerialMode mode, LogicTypeId typeId, i32 payload)
        OVERRIDE;
    virtual void Reset() OVERRIDE;
    virtual i32 Refresh(i32 a) OVERRIDE;
    virtual i32 Render() OVERRIDE;

    i32 BuildMultiplayerTabStatusBar(
        CStatusBarMgr* owner,
        CDDrawSurfaceMgr* host,
        SbiCommandId cmd,
        StatusBarTab tab,
        RECT g,
        const char* key,
        i32 playerIndex,
        i32 unitIndex,
        i32 selMode
    );

    i32 Update();

    CImage* m_statusGlyph;
    CImage* m_statusGlyphLatched;
    i32 m_statusValue;
    CImage* m_abilityGlyph;
    CImage* m_abilityGlyphLatched;
    i32 m_abilityValue;
    CImage* m_overrideGlyph;
    CImage* m_overrideGlyphLatched;
    i32 m_overrideValue;
    CImage* m_selectKey;
    CImage* m_selectGlyph;
    i32 m_selectValue;
    i32 m_playerIndex;
    i32 m_unitIndex;
    CDDrawWorker* m_timerGlyphMap;
    CImage* m_timerGlyph;
    i32 m_timerValue;
    CDDrawWorker* m_glyphMap;

    union {
        Clock64 m_timerAnchor;
        struct {
            i32 m_timerAnchorLo;
            i32 m_timerAnchorHi;
        };
    };
    union {
        Clock64 m_timerWindow;
        struct {
            i32 m_timerWindowLo;
            i32 m_timerWindowHi;
        };
    };
};

#endif // SBI_STATZTABGRUNTBAR_H
