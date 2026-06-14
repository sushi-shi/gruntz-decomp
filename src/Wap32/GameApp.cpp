// GameApp.cpp - WAP32 CGameApp (Brian Goble's engine).
// Matched: ??0CGameApp@@QAE@XZ @ RVA 0x13d590 (byte-exact code; the instance
// counter at 0x653c6c is a file-scope global here - same store sequence, the
// reloc just names a different symbol than the Ghidra DAT_ at that address).
#include "Wap32.h"

// File-scope instance counter (binary: global int @ 0x653c6c, bumped per ctor).
static int s_gameAppCount;

CGameApp::CGameApp()
{
    m_4   = 0;
    m_8   = 0;
    m_10  = 0;   // the optimiser schedules the +0x10 store before +0x0c
    m_c   = 0;
    m_240 = 0;
    m_248 = 0;
    m_24c = 0;
    m_250 = 0;
    s_gameAppCount++;
}

// -------------------------------------------------------------------------
// CGameApp::CloseResources  @ RVA 0x13d8c0 (66 B) - byte-exact.
// Frees the accelerator table then deletes the two resource objects.
// -------------------------------------------------------------------------
void CGameApp::CloseResources()
{
    if (m_10) {
        DestroyAcceleratorTable(m_10);
        m_10 = 0;
    }
    if (m_8) {
        delete m_8;
        m_8 = 0;
    }
    if (m_4) {
        delete m_4;
        m_4 = 0;
    }
}

// -------------------------------------------------------------------------
// CGameApp::InitializeAccelerators  @ RVA 0x13dc20 (73 B) - byte-exact.
// Reloads the accelerator table; returns whether it loaded.
// -------------------------------------------------------------------------
BOOL CGameApp::InitializeAccelerators(LPCSTR lpTable)
{
    if (lpTable && *lpTable) {
        if (m_10) {
            DestroyAcceleratorTable(m_10);
            m_10 = 0;
        }
        m_10 = LoadAcceleratorsA(m_c, lpTable);
        return m_10 != 0;
    }
    return 0;
}

// -------------------------------------------------------------------------
// CGameApp::ReportError  @ RVA 0x13dcb0 (87 B) - byte-exact.
// Records an error once (guarded by m_248), posting WM_CLOSE to the window.
// -------------------------------------------------------------------------
void CGameApp::ReportError(WPARAM wParam, LPARAM lParam)
{
    if (m_248)
        return;
    m_248 = 1;
    if (m_4 && m_4->m_c == 0)
        PostMessageA((HWND)m_4->m_4, 0x10 /*WM_CLOSE*/, 0, 0);
    m_244 = 0;
    m_24c = (int)wParam;
    m_250 = (int)lParam;
}

// -------------------------------------------------------------------------
// CGameApp::InitializeDefaultWindowClass  @ RVA 0x13d9b0 (160 B) - byte-exact.
// Fills the embedded WNDCLASSA (m_wc @ +0x1e8) and loads its icon/cursor.
// -------------------------------------------------------------------------
void CGameApp::InitializeDefaultWindowClass()
{
    int i;
    for (i = 0; i < 10; i++)
        ((int *)&m_wc)[i] = 0;

    HCURSOR hCursor = LoadCursorA(m_c, m_a0);
    if (m_18 & 1)
        hCursor = LoadCursorA(0, (LPCSTR)0x7f00 /*IDC_ARROW*/);

    m_wc.style         = 8;   // CS_DBLCLKS
    m_wc.lpfnWndProc   = GameWindowProc;
    m_wc.cbClsExtra    = 0;
    m_wc.cbWndExtra    = 0;
    m_wc.hInstance     = m_c;
    m_wc.hIcon         = LoadIconA(m_c, m_a0);
    m_wc.hCursor       = hCursor;
    m_wc.hbrBackground = (HBRUSH)GetStockObject(4);
    m_wc.lpszMenuName  = 0;
    m_wc.lpszClassName = m_160;
}

CGameApp::~CGameApp() {}
int CGameApp::Wap32GameAppVfunc0() { return 0; }

// Out-of-line stubs so referenced symbols are emitted in this TU.
CGameResource::~CGameResource() {}
LRESULT __stdcall CGameApp::GameWindowProc(HWND, UINT, WPARAM, LPARAM) { return 0; }
