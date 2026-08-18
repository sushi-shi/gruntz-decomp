#include <rva.h>

#include <Gruntz/ChatBox.h>

#include <DDrawMgr/DDrawSubMgrPages.h>
#include <DDrawMgr/DDrawSurfacePair.h>
#include <DDrawMgr/DDrawWorkerRegistry.h>
#include <DDrawMgr/DirectDrawMgr.h>
#include <Dsndmgr/DirectSoundMgr.h>
#include <Gruntz/GameRegistry.h>
#include <Gruntz/MenuPage.h>
#include <Gruntz/SoundState.h>
#include <Image/CImage.h>
#include <Image/ImageSet.h>
#include <Rez/FrameClock.h>
#include <Wap32/CoordUnset.h>

#include <stddef.h>

RVA(0x00182ab0, 0x7b)
i32 CChatBox::InitRegion(CDDrawSurfaceMgr* src, HWND wnd, RECT* rc, i32 d, i32 e, i32 f) {
    if (!src) {
        return 0;
    }
    m_page = src;
    m_wnd = wnd;
    m_wrapFlag = f;
    m_headGap = d;
    m_rowSpacing = e;
    m_activeNode = NULL;
    if (rc) {
        CopyRect(&m_rect8, rc);
        return 1;
    }
    m_rect8.left = 0;
    m_rect8.top = 0;
    m_rect8.right = src->m_drawTarget->m_frontPair->m_width - 1;
    m_rect8.bottom = src->m_drawTarget->m_frontPair->m_height - 1;
    return 1;
}

RVA(0x00182b30, 0x30)
void CChatBox::Reset() {
    Clear();
    m_page = NULL;
    m_wnd = NULL;
    m_activeNode = NULL;
    m_row0Anim = NULL;
    m_row1Anim = NULL;
    m_row0Frame = NULL;
    m_row1Frame = NULL;
    m_row0Key.Empty();
    m_row1Key.Empty();
}

RVA(0x00182b60, 0x3e)
void CChatBox::Clear() {
    POSITION pos = m_nodeList.GetHeadPosition();
    while (pos) {
        CMenuPage* payload = static_cast<CMenuPage*>(m_nodeList.GetNext(pos));
        delete payload;
    }
    m_nodeList.RemoveAll();
    m_activeNode = NULL;
}

RVA(0x00182ba0, 0x35)
i32 CChatBox::AddNode(CMenuPage* node) {
    if (!node) {
        return 0;
    }
    m_nodeList.AddTail(node);
    if (!m_activeNode) {
        AttachNode(node);
    }
    return 1;
}

RVA(0x00182be0, 0x8d)
CMenuPage* CChatBox::Find(const char* s) {
    POSITION pos = m_nodeList.GetHeadPosition();
    while (pos) {
        CMenuPage* payload = static_cast<CMenuPage*>(m_nodeList.GetNext(pos));
        if (payload) {
            if (strcmp(payload->GetKey(), s) == 0) {
                return payload;
            }
        }
    }
    return 0;
}

RVA(0x00182c70, 0x38)
i32 CChatBox::Step(u32 dt) {
    if (!m_activeNode) {
        return 0;
    }
    if (!m_activeNode->NotifyAll(dt)) {
        return 0;
    }
    return Step(static_cast<i32>(dt)) != 0;
}

RVA(0x00182cb0, 0x26)
i32 CChatBox::Pre() {
    if (!m_activeNode) {
        return 0;
    }
    CDDrawSurfacePair* target = m_page->m_drawTarget->m_backPair;
    if (!target) {
        return 0;
    }
    return m_activeNode->Layout(target) != 0;
}

RVA(0x00182ce0, 0x36)
i32 CChatBox::Post() {
    CDDrawSubMgrPages* s = m_page->m_drawTarget;
    s->m_frontPair->m_surface->Flip(0);
    s->m_backPair->m_surface
        ->BltFast(0, 0, s->m_overlayPair->m_surface, &s->m_overlayPair->m_srcRect, 0x10);
    return 1;
}

RVA(0x00182d20, 0x16)
i32 CChatBox::MoveFocusDown() {
    if (!m_activeNode) {
        return 0;
    }
    return m_activeNode->FocusNext() != 0;
}

RVA(0x00182d40, 0x16)
i32 CChatBox::MoveFocusUp() {
    if (!m_activeNode) {
        return 0;
    }
    return m_activeNode->FocusPrev() != 0;
}

RVA(0x00182d60, 0x16)
i32 CChatBox::ActivateFocusedItem() {
    if (!m_activeNode) {
        return 0;
    }
    return m_activeNode->Activate() != 0;
}

RVA(0x00182d80, 0x18)
i32 CChatBox::ReturnToPreviousPage() {
    if (!m_activeNode) {
        return 0;
    }
    return m_activeNode->Switch(1) != 0;
}

RVA(0x00182da0, 0x2a)
i32 CChatBox::AttachNode(CMenuPage* n) {
    if (!n) {
        return 0;
    }
    m_activeNode = n;
    n->ReleaseAll();
    m_activeNode->RestoreFocus();
    return 1;
}

RVA(0x00182dd0, 0x19)
i32 CChatBox::ReplaceNode(const char* key) {
    return AttachNode(Find(key));
}

// @early-stop
RVA(0x00182df0, 0x69)
i32 CChatBox::ConfigureLeftCursorAnimation(const char* key, i32 x, i32 y) {
    if (!m_page) {
        return 0;
    }
    CObject* a_ob = 0;
    m_page->m_imageRegistry->m_workersByName.Lookup(key, a_ob);
    CDDrawWorker* a = static_cast<CDDrawWorker*>(a_ob);
    m_row0Anim = a;
    if (!a) {
        return 0;
    }
    m_row0Frame = static_cast<CImage*>(a->m_items.GetAt(a->m_minIndex));
    m_row0FrameIdx = a->m_minIndex;
    m_row0Period = x;
    m_row0Timer = x;
    m_row0Offset = y;
    return 1;
}

// @early-stop
RVA(0x00182e60, 0x69)
i32 CChatBox::ConfigureRightCursorAnimation(const char* key, i32 x, i32 y) {
    if (!m_page) {
        return 0;
    }
    CObject* a_ob = 0;
    m_page->m_imageRegistry->m_workersByName.Lookup(key, a_ob);
    CDDrawWorker* a = static_cast<CDDrawWorker*>(a_ob);
    m_row1Anim = a;
    if (!a) {
        return 0;
    }
    m_row1Frame = static_cast<CImage*>(a->m_items.GetAt(a->m_minIndex));
    m_row1FrameIdx = a->m_minIndex;
    m_row1Period = x;
    m_row1Timer = x;
    m_row1Offset = y;
    return 1;
}

// @early-stop
RVA(0x00182ed0, 0xbc)
i32 CChatBox::Step(i32 delta) {
    CDDrawWorker* a = m_row0Anim;
    if (a) {
        if (static_cast<u32>(m_row0Timer) > static_cast<u32>(delta)) {
            m_row0Timer -= delta;
        } else {
            m_row0Timer = m_row0Period;
            i32 f = m_row0FrameIdx + 1;
            m_row0FrameIdx = f;
            CImage* v = a->GetAt(f);
            m_row0Frame = v;
            if (v == NULL) {
                m_row0Frame = static_cast<CImage*>(a->m_items.GetAt(a->m_minIndex));
                m_row0FrameIdx = a->m_minIndex;
            }
        }
    }
    CDDrawWorker* b = m_row1Anim;
    if (b) {
        if (static_cast<u32>(m_row1Timer) > static_cast<u32>(delta)) {
            m_row1Timer -= delta;
            return 1;
        }
        m_row1Timer = m_row1Period;
        i32 f = m_row1FrameIdx + 1;
        m_row1FrameIdx = f;
        CImage* v = b->GetAt(f);
        m_row1Frame = v;
        if (v == NULL) {
            m_row1Frame = static_cast<CImage*>(b->m_items.GetAt(b->m_minIndex));
            m_row1FrameIdx = b->m_minIndex;
        }
    }
    return 1;
}

// @early-stop
RVA(0x00182f90, 0x92)
i32 CChatBox::Draw(CDDrawSurfacePair* target, CMenuItem* sprite, i32 x0, i32 y0) {
    if (!sprite) {
        return 0;
    }
    i32 anchorX, anchorY;
    if (sprite->m_fixedX != UNINIT_FILL) {
        anchorY = sprite->m_fixedY;
        anchorX = sprite->m_fixedX;
    } else {
        anchorY = y0;
        anchorX = x0;
    }
    if (m_row0Frame) {
        i32 x = -(sprite->GetFrameWidth() / 2) - m_row0Offset + anchorX;
        m_row0Frame->RenderFrame(target, x, anchorY, 0);
    }
    if (m_row1Frame) {
        i32 x = sprite->GetFrameWidth() / 2 + m_row1Offset + anchorX;
        m_row1Frame->RenderFrame(target, x, anchorY, 0);
    }
    return 1;
}

// Both PlayFocusSound and PlayActivationSound inline this: cl5 defers the callee-save
// pushes into the inlined region, which is what gives the guard its own epilogue.
static __inline i32 PlayChatCue(CDDrawSubMgrLeafScan* roster, const char* key) {
    if (!roster->m_emitGate) {
        void* t_ob = 0;
        roster->m_cues.Lookup(key, t_ob);
        LeafCue* t = static_cast<LeafCue*>(t_ob);
        if (t != NULL) {
            i32 enabled = g_sndEnabled;
            i32 delta = g_sndCueTag;
            if (enabled != 0) {
                i32 clock = g_killCueClock;
                u32 elapsed = static_cast<u32>(clock) - static_cast<u32>(t->m_lastPlayTime);
                if (elapsed >= static_cast<u32>(t->m_replayDelay)) {
                    t->m_lastPlayTime = clock;
                    return t->m_sound->ConfigureItem(delta, 0, 0, 0);
                }
            }
        }
    }
    return 0;
}

RVA(0x00183030, 0x7b)
i32 CChatBox::PlayFocusSound() {
    if (m_row0Key.GetLength() == 0) {
        return 0;
    }
    return PlayChatCue(m_page->m_soundRegistry, m_row0Key);
}

RVA(0x001830b0, 0x7b)
i32 CChatBox::PlayActivationSound() {
    if (m_row1Key.GetLength() == 0) {
        return 0;
    }
    return PlayChatCue(m_page->m_soundRegistry, m_row1Key);
}

RVA(0x00183130, 0x16)
i32 CChatBox::MoveFocusLeft() {
    if (!m_activeNode) {
        return 0;
    }
    return m_activeNode->MoveFocusLeftColumn() != 0;
}

RVA(0x00183150, 0x16)
i32 CChatBox::MoveFocusRight() {
    if (!m_activeNode) {
        return 0;
    }
    return m_activeNode->MoveFocusRightColumn() != 0;
}

RVA(0x00183170, 0x24)
i32 CChatBox::FocusSelect(i32 x, i32 y) {
    if (!m_activeNode) {
        return 0;
    }
    return m_activeNode->FocusAndSelect(x, y) != 0;
}

RVA(0x001831a0, 0x24)
i32 CChatBox::ClickAt(i32 x, i32 y) {
    CMenuPage* n = m_activeNode;
    if (!n) {
        return 0;
    }
    return n->Click(x, y) != 0;
}

RVA(0x001831d0, 0x16)
i32 CChatBox::MoveFocusLeftFollowingLinks() {
    CMenuPage* n = m_activeNode;
    if (!n) {
        return 0;
    }
    return n->MoveFocusLeft() != 0;
}

RVA(0x001831f0, 0x16)
i32 CChatBox::MoveFocusRightFollowingLinks() {
    CMenuPage* n = m_activeNode;
    if (!n) {
        return 0;
    }
    return n->MoveFocusRight() != 0;
}

RVA(0x00183210, 0x16)
i32 CChatBox::MoveFocusUpFollowingLinks() {
    CMenuPage* n = m_activeNode;
    if (!n) {
        return 0;
    }
    return n->MoveFocusUp() != 0;
}

RVA(0x00183230, 0x16)
i32 CChatBox::MoveFocusDownFollowingLinks() {
    CMenuPage* n = m_activeNode;
    if (!n) {
        return 0;
    }
    return n->MoveFocusDown() != 0;
}
