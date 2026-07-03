// CSplashState.cpp - the splash-screen game state (its own TU; LoadSounds is the
// only reconstructed method so far, so this is the class's first/real TU). Re-homed
// from src/Stub/CSplashState.cpp (C:\Proj\Gruntz).
//
// LoadSounds() pulls the splash state's "SOUNDZ" asset namespace out of the engine's
// named-namespace registry and feeds it to the sound loader. The idiom mirrors the
// other asset loaders: look a node up by name through a manager, then recurse to load.
//
// Pure-Win32 TU (SetCursor); the gating global CString is modeled as its single
// LPTSTR m_pszData member, whose CStringData header (at m_pszData-8 == nDataLength)
// GetLength() reads - kept minimal to preserve the exact GetLength codegen (NOT the
// MFC CString, which would inline differently).
#include <Win32.h> // SetCursor

#include <rva.h>

// The engine's named-namespace registry node. Its child-lookup helpers are
// matched-elsewhere engine functions, modeled body-less so their `call rel32`
// reloc-masks: Lookup() finds a named child node by key.
SIZE_UNKNOWN(CAssetNamespace);
class CAssetNamespace {
public:
    CAssetNamespace* Lookup(char* szKey);    // FUN_0053c030 (thiscall)
    CAssetNamespace* LoadGroup(char* szKey); // FUN_0053a230 (thiscall)
};

// The sound loader the SOUNDZ namespace is handed to (reloc-masked external).
SIZE_UNKNOWN(CSoundLoader);
class CSoundLoader {
public:
    i32 LoadNamespace(CAssetNamespace* ns, char* szPrefix, char* szSuffix); // FUN_00557ee0
};

// The display/view object reached through this->m_4. SetVideoMode() is the
// 0x280x0x1e0 (640x480) display-mode setter (ErrorThunk_08ddd0, thiscall).
SIZE_UNKNOWN(CSplashView);
class CSplashView {
public:
    i32 SetVideoMode(i32 bForce);
};

// The owner that hands the loaded sound set off through its m_28 sound loader.
SIZE_UNKNOWN(CSplashOwner);
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
    i32 GetLength() const {
        return ((i32*)m_pszData)[-2];
    }
};

// The global asset-root CString whose emptiness gates the load (0x64e25c).
DATA(0x0064e25c)
extern CString g_assetRoot;

// Realized real-polymorphic: 26 virtual slots (0x68 vtable) + a real out-of-line
// dtor so cl emits ??_7CSplashState@@6B@. The vptr occupies +0x00 (was
// m_pad00[4]); LoadSounds only touches >= +0x04 so its codegen is unchanged. The
// dtor calls a DEFINED member (RealizeAnchor) so cl's implicit entry vptr-store
// survives (an empty dtor - or one calling only a declared-only virtual - gets
// dead-store-eliminated, dropping the ??_7 reference). The emitted dtor/??_G/??_7
// carry no RVA -> unpaired -> matching-neutral.
class CSplashState {
public:
    virtual ~CSplashState();         // slot 0
    virtual void Vslot01();          // slot 1
    virtual void ReleaseResources(); // slot 2
    virtual void Vslot03();
    virtual void Vslot04();
    virtual void Vslot05();
    virtual void Vslot06();
    virtual void Vslot07();
    virtual void Vslot08();
    virtual void Vslot09();
    virtual void Vslot0a();
    virtual void Vslot0b();
    virtual void Vslot0c();
    virtual void Vslot0d();
    virtual void Vslot0e();
    virtual void Vslot0f();
    virtual void Vslot10();
    virtual void Vslot11();
    virtual void Vslot12();
    virtual void Vslot13();
    virtual void Vslot14();
    virtual void Vslot15();
    virtual void Vslot16();
    virtual void Vslot17();
    virtual void Vslot18();
    virtual void Vslot19();

    i32 LoadSounds(i32 a, i32 b, i32 c);

    i32 LoadGameAssetNamespaces(i32 a, i32 b, i32 c); // LoadGameAssetNamespaces

    i32 RealizeAnchor(); // defined out-of-line; the dtor calls it (keeps the vptr store)

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
i32 CSplashState::LoadSounds(i32 a, i32 b, i32 c) {
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

// Realization anchor (unpaired, no RVA): a defined member the dtor calls so cl
// keeps the implicit vptr-store and emits ??_7CSplashState@@6B@.
i32 CSplashState::RealizeAnchor() {
    return m_2c != 0;
}

// Out-of-line dtor to force the ??_7CSplashState@@6B@ COMDAT (unpaired, no RVA).
CSplashState::~CSplashState() {
    RealizeAnchor();
}
