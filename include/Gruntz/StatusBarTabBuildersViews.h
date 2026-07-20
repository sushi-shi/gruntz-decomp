// StatusBarTabBuildersViews.h - TOMBSTONE. The eight "Build*" view structs that
// lived here are all DISSOLVED onto canon (2026-07-20):
//   CSbGeom          == the SbiRect geometry block (CStatusBarItem::m_rect14)
//   CSbNamespaceMap  == the MFC map (band-selected at the call)
//   CSbMapHost       == CDDrawWorkerRegistry (map @+0x10 = m_10map)
//   CSbOwner         == CDDrawSurfaceMgr     (+0x10 = m_imageRegistry)
//   CSbImageSet      == CImageSet/CDDrawWorker (the SIXTH view of the 0x6c shape)
//   CSbParent        == CStatusBarMgr        (m_10/m_rect14.m_4 geometry anchors)
//   CSbWorldSlot     == the m_options[] GruntzPlayer array REBASED by -0x18
//                       (+0x138[i*0x238]+0x20 == +0x150[i*0x238]+0x08 == m_options[i].m_008)
//   CSbTab           == CSBI_SideTab (BuildStatzTabStatusBar's proven owner)
// Only ONE had a live code use (the world-slot read); the rest were dead decls.
#ifndef GRUNTZ_STATUSBAR_TAB_BUILDERS_VIEWS_H
#define GRUNTZ_STATUSBAR_TAB_BUILDERS_VIEWS_H

namespace StatusBarTabBuilders {} // (the TU still opens this namespace for its g_curPlayer alias)

#endif // GRUNTZ_STATUSBAR_TAB_BUILDERS_VIEWS_H
