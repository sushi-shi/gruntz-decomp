#include <rva.h>
// ApiHiCallers.cpp - drained (HIGH-RVA half, RVA >= 0x0e0000). Its two reconstructed
// game API-caller methods have been re-homed to their RVA-neighborhood TUs:
//
//  - 0x00168080 (the compound-widget / WwdPlaneRender-Init builder) -> re-homed to
//    src/Gruntz/GameLevel.cpp (Builder_168080::Init + SubWidget_168080/Pt_168080 views;
//    fold onto WwdPlaneRender::Init deferred, see the note there).
//  - 0x0017c3f0 (the page/cursor command handler) -> re-homed to
//    src/Io/SmackerVideoWindow.cpp (Handler_17c3f0::Init + ObjA2/ObjA3 fn-table views;
//    a stack-local command block CGruntzMgr::ChangeState_8fab0 builds, placeholder shape).
//
// (0x0017fe00 fader-noise init was earlier re-homed to src/Gruntz/Fader.cpp as
// CFaderSine::ApplyInit.)
