#ifndef GRUNTZ_GRUNTZ_ERRORSTRINGID_H
#define GRUNTZ_GRUNTZ_ERRORSTRINGID_H

#include <Enums.h>

// The STRING-TABLE resource id CGameApp::ReportError takes as its first
// argument. CGruntzApp::ShowError is what says so: it stores the argument in
// m_errorCode and then feeds it straight to
// `LoadStringA(m_hInstance, id, g_errorText, 0xfa)`, falling back to
// IDS_DEFAULT_ERROR when that fails.
//
// These values are decoded directly from retail's RT_STRING resources. They
// overlap numerically with GruntzCommandId, but are a separate domain: for
// example, 0x8009 is both CMD_TOGGLE_SOUND and the default error string.
//
// The SECOND argument of ReportError is a different thing again and needs no
// domain at all: ShowError prints it verbatim with `sprintf(detail, "(%i)",
// detailVal)` and appends it to the message. It is a per-call-site tag whose
// only job is to be unique, which is why its ~113 values (0x141 .. 0x1232) mean
// nothing individually and can never be named.
GZ_ENUM_BEGIN(ErrorStringId)
    IDS_SET_GAME_STATE = 0x8005,
    IDS_RESTORE_GAME = 0x8006,
    IDS_CHANGE_LEVEL = 0x8007,
    IDS_SET_VIDEO_MODE = 0x8008,
    IDS_DEFAULT_ERROR = 0x8009,
    IDS_INITIALIZE_GAME = 0x800a,
    IDS_LOAD_RESOURCE_FILE = 0x800b,
    IDS_WORLD_UNKNOWN = 0x8011,
    IDS_WORLD_SOUND_REGISTRY = 0x8012,
    IDS_WORLD_SOUND_OUTPUT = 0x8013,
    IDS_WORLD_CREATE_DEVICE = 0x8014,
    IDS_WORLD_CREATE_PAGES = 0x8015,
    IDS_WORLD_CREATE_PALETTE_SURFACE = 0x8016,
    IDS_WORLD_OVERLAY_SURFACE = 0x8017,
    IDS_WORLD_BACK_SURFACE = 0x8018,
    IDS_WORLD_FRONT_SURFACE = 0x8019,
    IDS_WORLD_DDRAW_COOPERATIVE_LEVEL = 0x801a,
    IDS_WORLD_DDRAW_CAPABILITIES = 0x801b,
    IDS_WORLD_DDRAW_DISPLAY_MODE = 0x801c,
    IDS_WORLD_DDRAW_COLOR_MASKS = 0x801d,
    IDS_WORLD_DDRAW_CREATE = 0x801e,
    IDS_CHANGE_COLOR_DEPTH = 0x801f,
    IDS_LOAD_VOICE_RESOURCE_FILE = 0x8149
GZ_ENUM_END(ErrorStringId)

#endif // GRUNTZ_GRUNTZ_ERRORSTRINGID_H
