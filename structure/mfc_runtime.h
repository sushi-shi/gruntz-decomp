#ifndef GRUNTZ_MFC_RUNTIME_H
#define GRUNTZ_MFC_RUNTIME_H

/*
 * MFC / CRT / iostream / AFX-internal / COM RTTI names — LINK ARTIFACTS.
 *
 * These classes appear in the binary's RTTI because MFC and the MSVC 5.0 CRT are
 * STATICALLY linked. They are NOT game code and NOT decomp targets — they are
 * well-known framework/library types with published layouts. We list every one of
 * them here ONLY so that all 231 RTTI names are accounted for (see ../INDEX.md);
 * we do NOT re-declare or stub them (afxwin.h / the CRT headers provide the real
 * definitions). Each is kept as a comment with its verbatim mangled name.
 *
 * Provenance: ALL from RTTI. Do not treat as scaffolding to fill in.
 */

/* ---- MFC: app / thread / cmd target ---- */
// CWinApp                   .?AVCWinApp@@
// CWinThread                .?AVCWinThread@@
// CCmdTarget                .?AVCCmdTarget@@
// CCmdUI                    .?AVCCmdUI@@
// CTestCmdUI                .?AVCTestCmdUI@@
// CCommandLineInfo          .?AVCCommandLineInfo@@
// CRecentFileList           .?AVCRecentFileList@@
// CNoTrackObject            .?AVCNoTrackObject@@
// CObject                   .?AVCObject@@   (MFC root; some game objects derive from it)

/* ---- MFC: windows / views / docs ---- */
// CWnd                      .?AVCWnd@@
// CFrameWnd                 .?AVCFrameWnd@@
// CDialog                   .?AVCDialog@@   (game dialogs derive from this; see game/dialogs.h)
// CView                     .?AVCView@@
// CCtrlView                 .?AVCCtrlView@@
// CScrollView               .?AVCScrollView@@
// CDocument                 .?AVCDocument@@

/* ---- MFC: device contexts / GDI ---- */
// CDC                       .?AVCDC@@
// CClientDC                 .?AVCClientDC@@
// CPaintDC                  .?AVCPaintDC@@
// CWindowDC                 .?AVCWindowDC@@
// CGdiObject                .?AVCGdiObject@@
// CBrush                    .?AVCBrush@@
// CPen                      .?AVCPen@@
// CRgn                      .?AVCRgn@@
// CMenu                     .?AVCMenu@@
// CImageList                .?AVCImageList@@
// CImage                    .?AVCImage@@

/* ---- MFC: standard controls ---- */
// CButton                   .?AVCButton@@
// CComboBox                 .?AVCComboBox@@
// CEdit                     .?AVCEdit@@
// CStatic                   .?AVCStatic@@
// CListBox                  .?AVCListBox@@
// CDragListBox              .?AVCDragListBox@@
// CScrollBar                .?AVCScrollBar@@
// CSliderCtrl               .?AVCSliderCtrl@@
// CSpinButtonCtrl           .?AVCSpinButtonCtrl@@
// CProgressCtrl             .?AVCProgressCtrl@@
// CHeaderCtrl               .?AVCHeaderCtrl@@
// CHotKeyCtrl               .?AVCHotKeyCtrl@@
// CListCtrl                 .?AVCListCtrl@@
// CTabCtrl                  .?AVCTabCtrl@@
// CTreeCtrl                 .?AVCTreeCtrl@@
// CRichEditCtrl             .?AVCRichEditCtrl@@
// CAnimateCtrl              .?AVCAnimateCtrl@@
// CStatusBarCtrl            .?AVCStatusBarCtrl@@
// CToolBarCtrl              .?AVCToolBarCtrl@@

/* ---- MFC: files / archive ---- */
// CFile                     .?AVCFile@@
// CMemFile                  .?AVCMemFile@@
// CMirrorFile               .?AVCMirrorFile@@
// CArchiveStream            .?AVCArchiveStream@@

/* ---- MFC: collections ---- */
// CByteArray                .?AVCByteArray@@
// CDWordArray               .?AVCDWordArray@@
// CStringArray              .?AVCStringArray@@
// CPtrArray                 .?AVCPtrArray@@
// CObArray                  .?AVCObArray@@
// CStringList               .?AVCStringList@@
// CPtrList                  .?AVCPtrList@@
// CObList                   .?AVCObList@@
// CMapStringToOb            .?AVCMapStringToOb@@
// CMapStringToPtr           .?AVCMapStringToPtr@@
// CMapPtrToPtr              .?AVCMapPtrToPtr@@
// CArray<PLAYLISTINFOSTRUCT*>   .?AV?$CArray@PAUPLAYLISTINFOSTRUCT@@PAU1@@@
//                               (the music playlist; element struct in
//                                managers/directsoundmgr.h)

/* ---- MFC: exceptions ---- */
// CException                .?AVCException@@
// CArchiveException         .?AVCArchiveException@@
// CFileException            .?AVCFileException@@
// CMemoryException          .?AVCMemoryException@@
// CNotSupportedException    .?AVCNotSupportedException@@
// CResourceException        .?AVCResourceException@@
// CSimpleException          .?AVCSimpleException@@
// CUserException            .?AVCUserException@@

/* ---- MFC internal state (AFX_*) ---- */
// _AFX_BASE_MODULE_STATE    .?AV_AFX_BASE_MODULE_STATE@@
// _AFX_CTL3D_STATE          .?AV_AFX_CTL3D_STATE@@
// _AFX_CTL3D_THREAD         .?AV_AFX_CTL3D_THREAD@@
// AFX_MODULE_STATE          .?AVAFX_MODULE_STATE@@
// AFX_MODULE_THREAD_STATE   .?AVAFX_MODULE_THREAD_STATE@@
// _AFX_THREAD_STATE         .?AV_AFX_THREAD_STATE@@
// _AFX_WIN_STATE            .?AV_AFX_WIN_STATE@@
// CThreadData (struct)      .?AUCThreadData@@

/* ---- CRT iostream (MSVC 5.0) ---- */
// type_info                 .?AVtype_info@@
// ios                       .?AVios@@
// iostream                  .?AViostream@@
// istream                   .?AVistream@@
// ostream                   .?AVostream@@
// istream_withassign        .?AVistream_withassign@@
// ostream_withassign        .?AVostream_withassign@@
// istrstream                .?AVistrstream@@
// ostrstream                .?AVostrstream@@
// strstream                 .?AVstrstream@@
// filebuf                   .?AVfilebuf@@
// fstream                   .?AVfstream@@
// ifstream                  .?AVifstream@@
// ofstream                  .?AVofstream@@
// streambuf                 .?AVstreambuf@@
// strstreambuf              .?AVstrstreambuf@@

/* ---- COM interface structs ---- */
// IUnknown (struct)         .?AUIUnknown@@
// IStream  (struct)         .?AUIStream@@
// ISequentialStream (struct) .?AUISequentialStream@@

#endif /* GRUNTZ_MFC_RUNTIME_H */
