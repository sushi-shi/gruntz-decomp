# Cast-dissolution plan — every reinterpret_cast + view class to 0 (or a proven keep)

Goal (user, 2026-07-19): cross-class / char-arith / fn-ptr / hier-related / other /
`*Views.h` classes → 0. Allowed keeps: generic collections + raw data, each with a
recorded proof. Measurement: `python -m gruntz.analysis.reinterpret_census` (re-run
before every offset batch; TSV worklist in build/gen/). Every family lands byte-gated
(`gruntz build --fast`, Overall must hold) and is committed separately.

Proven mechanisms (all exercised this campaign):
  M1 identity fold        two names, one class -> fold/typedef (Coord==GruntTilePos)
  M2 quad->RECT           4-dword L/T/R/B runs passed as RECT -> real RECT member
  M3 param/field retype   the cast papers a wrong signature (CreateWorker/GameObjNotifyFn)
  M4 complete-the-type    fwd-decl-only TU -> include real header, cast goes static (CImage)
  M5 real inheritance     container elements missing their CObject base
  M6 error-driven rename  flip the decl, let ninja -k enumerate sites; NEVER blind sed
  M7 measured keep        byte-proof that the cast is load-bearing (Blowfish decay)

## Workstreams, in execution order

WS-K  cross-incomplete (31)          M4. CSymTab<->CResSource 7, CWinApp->CWaitCursorApp 2,
                                     CTriggerMgr->TabzGmFactory 2, CGrunt->CPathEntity 2, rest 1-2s.
WS-B  geometry twins (~140)          M1/M2. LevelCoordRect/BlitRect/ShadeRect/CScanRectInit/
                                     CCueRect/TextRange/CRect-variants -> tagRECT|Coord|POINT;
                                     int*->Coord 18, long*->tagRECT 19, int*->tagRECT 24 residue.
WS-D  container elements (~75)       M5. CObject->CAniDesc 16, CDDrawWorker->CAniElement 16,
                                     CObject->CMenuSprite 5, CObject*->int* 30, CObject**->
                                     CPlaneFrame** 10; MFC band fixes CMapStringToOb->Ptr 5,
                                     CPtrArray->CDWordArray 4 (mfc_class arbitrates).
WS-A  manager/singleton folds (~120) M1/M3. A1 g_gameReg triple-name (WwdGameReg/CGameRegistry/
                                     CGruntzMgr, 35); A2 CDDrawSurfaceMgr facets (CSbiGameMgr 24,
                                     EngStrRenderObj 10); A3 CActReg/LogicFnTable/CLogicActTable/
                                     CIndicatorActReg -> CZDArrayDerived duality (39); A4 CMapMgr
                                     facets (CScanGrid 9 - facet per memory, GruntBoard 8+4 -
                                     PROVEN irreducible, likely M7); A5 CUserLogic slots-3/4/5
                                     arbitration -> CXferArchive dissolves (12; byte evidence
                                     already decoded - slots 3/4 are ret-4 no-op hooks).
WS-E  net/sound views (~35)          M1. CloneNode/CloneList->DSound* 10, CNetChannel->
                                     GruntzPlayer 5, CNetPlayerObj->CNetPlayerDesc 4,
                                     CMultiStartDlg->CNetSessHost 4, CMultiPlayerInfo->
                                     InterfaceObject 4, CGruntCueSink->CGruntSpawnConfig 7.
WS-I  fn-ptr residue (35)            M3 for stored-callback ints (int->fn 8); M7 keeps for
                                     GetProcAddress/_heapinfo CRT interop (~25); strip the
                                     2 DlgProc self-casts.
WS-G  char-arith (1819)              G1 extend census: split arith-participating (array-walk
                                     suspects) from plain data. G2 char*->int* 472: packed
                                     record structs per family (SymTab hdr exemplar). G3
                                     u8*->u16* 204 pixel I/O -> keeps. G4 int*->char*/
                                     char*->void**/void*->char* ~256 byte-I/O -> mostly keeps.
                                     G5 char*->char** 53 row tables (GruntBoard M7?), ->double* 38.
                                     RasterVtx ClipVtx 0x1c-stride walk = the remodel exemplar.
WS-H  other residue (~250 after B/D) int*<->i64* 74 puns; int*->CGruntVoiceRec* 38 (type the
                                     source buffer); signedness/width puns 27 (source typing);
                                     double*->int* 21 FP puns (keeps); __POSITION** 7
                                     (collection keep); BrickzCell*/CStatusBarMgr*->int* 16;
                                     CTrigParam/CPairRecord/_DDCAPS 19.
WS-C  string shells (~75)            zDArray<T> TEMPLATE remodel (retail RTTI proves real
                                     template instantiations) -> the _zvec::m_alloc CString
                                     facet types itself; GruntStrSub/CStringNode/
                                     CAnimScratchString fold onto CString; char*->CString* 55.
WS-J  *Views.h classes (64)          M1 via xref/RTTI identity per shell: BoundaryLowerMethods 20
                                     (C<rva> shells), TriggerMgrViews 12, BoundaryUpper 7,
                                     AniRecordViews 5, Tail 3, SbiTabz/SideTab 4, singles 4.
                                     LogicWorkerHandlersA 6 = ctor-wall M7 keeps (measured).
WS-F  document keeps                 DnnRec->EngRec (wall), CWarlord->CGrunt (blocked alias),
                                     CFile->CFileIODispatch (load-bearing shim), AnimWorkerObj->
                                     CProjBoundCfg (role-union facet), CObjNode->CRezItmBase
                                     (intrusive-list slot), CDDrawPtrCollections->CDirectDrawMgr.

Endstate accounting: every site is either DISSOLVED (model fixed) or a RECORDED KEEP
(collection/data/measured-wall with proof). The census keep-buckets (collection,
to/from-void by origin, measured M7 entries) are the only nonzero remainder the goal
permits; everything else trends to 0 through the workstreams above.
