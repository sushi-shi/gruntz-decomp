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
    // The 0x8011..0x801f run, each pinned to the RT_STRING text it loads. The
    // four DDRAW ids used to be numbered the other way round, paired ASCENDING
    // against ReportWorldStatus' ascending `case WORLDERR_DDRAW_*` arms; the
    // resource table says retail pairs them DESCENDING, and the switch's
    // jump-table arms agree (0x80ea -> 0x801d, ... 0x80ed -> 0x801a).
    IDS_WORLD_UNKNOWN = 0x8011,                 // "Unable to initialize the engine."
    IDS_WORLD_SOUND_REGISTRY = 0x8012,          // "Can't initialize the sound manager."
    IDS_WORLD_SOUND_OUTPUT = 0x8013,            // "Can't initialize DirectSound."
    IDS_WORLD_CREATE_DEVICE = 0x8014,           // "Can't initialize DirectDraw."
    IDS_WORLD_CREATE_PAGES = 0x8015,            // "Can't initialize the page manager."
    IDS_WORLD_CREATE_PALETTE_SURFACE = 0x8016,  // "Can't get the primary surface. ..."
    IDS_WORLD_OVERLAY_SURFACE = 0x8017,         // "Can't create the background page."
    IDS_WORLD_BACK_SURFACE = 0x8018,            // "Can't create the work page."
    IDS_WORLD_FRONT_SURFACE = 0x8019,           // "Can't create the view page."
    IDS_WORLD_DDRAW_COLOR_MASKS = 0x801a,       // "Unable to set the DirectDraw RGB format."
    IDS_WORLD_DDRAW_DISPLAY_MODE = 0x801b,      // "Unable to set the DirectDraw display mode."
    IDS_WORLD_DDRAW_CAPABILITIES = 0x801c,      // "Unable to get the DirectDraw capabilities."
    IDS_WORLD_DDRAW_COOPERATIVE_LEVEL = 0x801d, // "Unable to set the DirectDraw cooperative level."
    IDS_WORLD_DDRAW_CREATE = 0x801e,            // "Unable to create the DirectDraw object."
    IDS_CHANGE_COLOR_DEPTH = 0x801f,
    IDS_LOAD_VOICE_RESOURCE_FILE = 0x8149
GZ_ENUM_END(ErrorStringId)

#endif // GRUNTZ_GRUNTZ_ERRORSTRINGID_H
