#include <rva.h>
// CSplashState.cpp - the splash-screen game state. LoadSounds() pulls the splash
// state's "SOUNDZ" asset namespace out of the engine's named-namespace registry
// and feeds it to the sound loader (C:\Proj\Gruntz). The idiom mirrors the other
// asset loaders: look a node up by name through a manager, then recurse to load.
//
// (Lives in the engine_label_stubs aggregate, where ApiCallers.cpp already pulls
// <windows.h> in first, so MFC's <afx.h> can't follow - the gating global CString
// is modeled as its single LPTSTR m_pszData member, whose CStringData header (at
// m_pszData-8 == nDataLength) GetLength() reads. Matching-neutral.)

// The engine's named-namespace registry node. Its child-lookup helpers are
// matched-elsewhere engine functions, modeled body-less so their `call rel32`
// reloc-masks: Lookup() finds a named child node by key.
class CAssetNamespace {
public:
    CAssetNamespace* Lookup(char* szKey);    // FUN_0053c030 (thiscall)
    CAssetNamespace* LoadGroup(char* szKey); // FUN_0053a230 (thiscall)
};

// The sound loader the SOUNDZ namespace is handed to (reloc-masked external).
class CSoundLoader {
public:
    int LoadNamespace(CAssetNamespace* ns, char* szPrefix, char* szSuffix); // FUN_00557ee0
};

// The display/view object reached through this->m_4. SetVideoMode() is the
// 0x280x0x1e0 (640x480) display-mode setter (ErrorThunk_08ddd0, thiscall).
class CSplashView {
public:
    int SetVideoMode(int bForce);
};

// The owner that hands the loaded sound set off through its m_28 sound loader.
class CSplashOwner {
public:
    char m_pad00[0x28];
    CSoundLoader* m_28; // +0x28
};

// The global empty C string the sound loader's prefix is seeded from (0x6293f4).
extern "C" char g_emptyString[]; // 0x6293f4

// MFC CString modeled by its single member (the LPTSTR m_pszData at offset 0);
// GetLength() reads nDataLength out of the CStringData header at m_pszData - 8.
struct CString {
    char* m_pszData;
    int GetLength() const {
        return ((int*)m_pszData)[-2];
    }
};

// The global asset-root CString whose emptiness gates the load (0x64e25c).
DATA(0x0064e25c)
extern CString g_assetRoot;

class CSplashState {
public:
    int LoadSounds(int a, int b, int c);

    int LoadGameAssetNamespaces(int a, int b, int c); // LoadGameAssetNamespaces

    char m_pad00[0x4];
    CSplashView* m_4;     // +0x04 display/view object
    CAssetNamespace* m_8; // +0x08 named-namespace registry root
    CSplashOwner* m_c;    // +0x0c owner (its m_28 is the sound loader)
    char m_pad10[0x2c - 0x10];
    CAssetNamespace* m_2c; // +0x2c looked-up STATEZ_SPLASH namespace
};

// @confidence: high
// @source: decomp-xref
// @early-stop
// Code bytes 100% byte-identical to retail (fuzzy 100%); all 6 referents are
// UNNAMED engine placeholders on the target side (DAT_0064e25c, LoadGameAsset-
// Namespaces, ErrorThunk_08ddd0, FUN_0053c030/0053a230/00557ee0), so the relocs
// can never pair by name -> stuck at the documented reloc-masked plateau, NOT
// exact. See docs/patterns/external-nobody-callee.md + reloc-typing-vptr-global.md.
RVA(0x000f9780, 0x8c)
int CSplashState::LoadSounds(int a, int b, int c) {
    if (g_assetRoot.GetLength() == 0) {
        return 0;
    }
    if (!LoadGameAssetNamespaces(a, b, c)) {
        return 0;
    }
    SetCursor(0);
    m_4->SetVideoMode(0);

    m_2c = m_8->Lookup("STATEZ_SPLASH");
    if (!m_2c) {
        return 0;
    }

    CAssetNamespace* soundz = m_2c->LoadGroup("SOUNDZ");
    if (soundz) {
        m_c->m_28->LoadNamespace(soundz, g_emptyString, "_");
    }
    return 1;
}
