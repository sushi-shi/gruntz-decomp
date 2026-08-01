#ifndef GRUNTZ_MENUPAGE_H
#define GRUNTZ_MENUPAGE_H

#include <Ints.h>
#include <rva.h>

#include <Mfc.h>

#include <Gruntz/MenuItem.h>
#include <Gruntz/MenuItem2.h>

class CDDrawSurfaceMgr;
class CDDrawSurfacePair;
class CDDrawWorker;
class CChatBox;

class CMenuPage {
public:
    CMenuPage() {
        m_owner = 0;
        m_host = 0;
        m_subPage = 0;
        m_focus = 0;
        m_flags = 0;
    }

    ~CMenuPage() {
        InitDefaults();
    }
    CString GetKey();

    i32
    Configure(CChatBox* host, const char* label, const char* key, const char* parent, i32 flags);
    void InitDefaults();
    void Clear();
    i32 ResolveSubPage(const char* key);
    i32 Append(CMenuItem* item);

    CMenuItem*
    AddItem(const char* label, const char* spriteKey, i32 cmdId, const char* key, i32 flags);

    CMenuItem* AddSubItem(
        const char* label,
        const char* spriteKey,
        i32 cmdId,
        i32 cmdParam,
        i32 tag,
        const char* key,
        i32 flags
    );
    i32 ReleaseAll();
    i32 RestoreFocus();
    i32 SetFocus(CMenuItem* item, i32 notify);
    i32 NotifyAll(u32 dt);
    i32 Layout(CDDrawSurfacePair* target);
    i32 FocusNext();
    i32 FocusPrev();
    i32 Activate();
    i32 FocusAndSelect(i32 x, i32 y);
    i32 Click(i32 x, i32 y);
    CMenuItem* HitTest(i32 x, i32 y);
    CMenuItem* FindByName(const char* s);
    i32 SelectForward();
    i32 SelectBackward();
    i32 LayoutOne(CDDrawSurfacePair* target);

    CMenuItem2* AddItem2(
        const char* name,
        const char* spriteKey,
        i32 cmdId,
        const char* key,
        i32 flags,
        i32 frame
    );

    CMenuItem2* AddSubItem2(
        const char* name,
        const char* spriteKey,
        i32 cmdId,
        i32 cmdParam,
        i32 parentCtx,
        const char* key,
        i32 flags,
        i32 frame
    );
    i32 Switch(i32 refocus);
    i32 CanWrap();
    i32 FocusForwardN();
    i32 FocusBackwardN();
    i32 SelectFwd2();
    i32 SelectBack2();

    CDDrawSurfaceMgr* m_owner;
    CChatBox* m_host;
    CString m_switchKey;
    CString m_key;
    CString m_focusName;
    CPtrList m_items;

    CMenuItem* NextItem(POSITION& pos) {
        return static_cast<CMenuItem*>(m_items.GetNext(pos));
    }
    CMenuItem* PrevItem(POSITION& pos) {
        return static_cast<CMenuItem*>(m_items.GetPrev(pos));
    }
    i32 m_flags;
    RECT m_rect;

    i32 m_headGap;
    i32 m_rowSpacing;
    i32 m_colWidth;
    i32 m_rowsPerCol;
    i32 m_colOffset;
    i32 m_offsetX;
    i32 m_offsetY;
    CDDrawWorker* m_subPage;

    CMenuItem* m_focus;
};
SIZE(0x68);

#endif // GRUNTZ_MENUPAGE_H
