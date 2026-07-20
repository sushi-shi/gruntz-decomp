// LogicWorkerHandlersAViews.h - TOMBSTONE. The DnnRec / CUserLogicOOL / EngRec /
// DnnWorker / DnnOwner scaffolding is GONE (2026-07-20, 2nd attempt - the refutation
// facts became the fix): HandlerA9E00 is the shared LOGIC_WORKER_PUMP(CDoNothingNormal).
//   - size: CUserLogic(0x34) + CWapX(0x20) == 0x54 natively (the 1st attempt's 0x68 was
//     a wrongly-added tail pad, measured on a stale obj);
//   - ctor shape: the pump TU sets USERLOGIC_OOL_CTOR so the base init is retail's
//     out-of-line `call ??0CUserLogic` (0x58cd0), the derived tail inlines;
//   - the scaffold's CUserLogicOOL/DnnRec HAD reinvented CWapX + the CUserLogic
//     dispatch (DnnRec IS CDoNothingNormal, RTTI 0x1e859c; DnnWorker IS AnimWorkerObj;
//     DnnOwner IS CGameObject).
#ifndef GRUNTZ_LOGICWORKERHANDLERSAVIEWS_H
#define GRUNTZ_LOGICWORKERHANDLERSAVIEWS_H
#endif // GRUNTZ_LOGICWORKERHANDLERSAVIEWS_H
