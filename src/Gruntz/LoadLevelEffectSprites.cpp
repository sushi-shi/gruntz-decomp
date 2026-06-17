// LoadLevelEffectSprites - preloads all level effect sprites (hazards, deathz,
// teleporter, bridge, and special effects) and sets their frame-display duration.
// Strings xref'd from the reloc table: GAME_PYRAMIDMOVE through LEVEL_PLANEHAZARDFLY
// (30 sprites, ~30 in-game hazard/effect animation classes).
//
// The sprite lookup hash map is reached through the following pointer chain:
//   this->m_container (+0x0c)
//     -> m_mgr (+0x28)
//       -> m_map (+0x10, a CMapStringToOb keyed by const char*)
// The found CSprite has a writable frame-duration field at +0x18.
//
// BYTE-EXACT body (1307 B, __thiscall ret). Plain /O2 /MT (no /GX): scalar leaf.
//
// @address: 0x0dc060
// @size:    0x51b

// ---------------------------------------------------------------------------
// Forward declarations of engine/MFC types (external, no body - reloc-masked).
// ---------------------------------------------------------------------------
struct CSprite {
    char m_pad00[0x18];
    int  m_duration;          // +0x18  frame-display duration (ticks/ms?)
};

// The engine string-keyed sprite hash table (MFC CMapStringToOb wrapper).
// Lookup is __thiscall(this, key, &out) ret 8.
class CSpriteMap {
public:
    int Lookup(const char *szKey, CSprite **ppOut);  // external
};

// The sprite manager containing the look-up map at +0x10.
struct CSpriteMgr {
    char       m_pad00[0x10];
    CSpriteMap m_map;         // +0x10  the keyed sprite table
};

// The intermediate object the loader reaches the sprite manager through.
struct CContainer {
    char       m_pad00[0x28];
    CSpriteMgr *m_mgr;        // +0x28
};

// ---------------------------------------------------------------------------
// The loader class (minimal, only the load-bearing offset is modeled).
// ---------------------------------------------------------------------------
class CLevelEffectLoader {
public:
    int LoadLevelEffectSprites();

    char        m_pad00[0x0c];
    CContainer *m_container;  // +0x0c
};

// @address: 0x0dc060
// @size:    0x51b
int CLevelEffectLoader::LoadLevelEffectSprites()
{
    // The Hash map (CMapStringToOb) is at [container->m_mgr + 0x10].
    // Each block: zero local out-param, push key+&spr, set ecx, call Lookup, set duration.
    CSprite *spr;

    // The sequence of sprite names and their per-frame display durations.
    // Most entries use 100 (0x64); teleporter/weapon effects use 1000 (0x3e8);
    // cloud/plane hazards use larger values.

    { // 0x0dc072: "GAME_PYRAMIDMOVE" -> 100
        spr = 0;
        m_container->m_mgr->m_map.Lookup("GAME_PYRAMIDMOVE", &spr);
        if (spr) spr->m_duration = 100;
    }

    { // 0x0dc09e: "GAME_TELEPORTEROPEN" -> 1000
        spr = 0;
        m_container->m_mgr->m_map.Lookup("GAME_TELEPORTEROPEN", &spr);
        if (spr) spr->m_duration = 1000;
    }

    { // 0x0dc0c9: "GAME_TELEPORTERCLOSE" -> 1000
        spr = 0;
        m_container->m_mgr->m_map.Lookup("GAME_TELEPORTERCLOSE", &spr);
        if (spr) spr->m_duration = 1000;
    }

    { // 0x0dc0f4: "GAME_TELEPORTERALL" -> 4000
        spr = 0;
        m_container->m_mgr->m_map.Lookup("GAME_TELEPORTERALL", &spr);
        if (spr) spr->m_duration = 4000;
    }

    { // 0x0dc11f: "GAME_BRICKBREAK" -> 100
        spr = 0;
        m_container->m_mgr->m_map.Lookup("GAME_BRICKBREAK", &spr);
        if (spr) spr->m_duration = 100;
    }

    { // 0x0dc146: "LEVEL_DEATHBRIDGEMOVE" -> 100
        spr = 0;
        m_container->m_mgr->m_map.Lookup("LEVEL_DEATHBRIDGEMOVE", &spr);
        if (spr) spr->m_duration = 100;
    }

    { // 0x0dc16d: "LEVEL_WATERBRIDGEMOVE" -> 100
        spr = 0;
        m_container->m_mgr->m_map.Lookup("LEVEL_WATERBRIDGEMOVE", &spr);
        if (spr) spr->m_duration = 100;
    }

    { // 0x0dc194: "LEVEL_ROCKBREAK" -> 100
        spr = 0;
        m_container->m_mgr->m_map.Lookup("LEVEL_ROCKBREAK", &spr);
        if (spr) spr->m_duration = 100;
    }

    { // 0x0dc1bb: "LEVEL_LAVAGEYSER" -> 100
        spr = 0;
        m_container->m_mgr->m_map.Lookup("LEVEL_LAVAGEYSER", &spr);
        if (spr) spr->m_duration = 100;
    }

    { // 0x0dc1e2: "LEVEL_TRAPDOORCLOSE" -> 100
        spr = 0;
        m_container->m_mgr->m_map.Lookup("LEVEL_TRAPDOORCLOSE", &spr);
        if (spr) spr->m_duration = 100;
    }

    { // 0x0dc209: "LEVEL_TRAPDOOROPEN" -> 100
        spr = 0;
        m_container->m_mgr->m_map.Lookup("LEVEL_TRAPDOOROPEN", &spr);
        if (spr) spr->m_duration = 100;
    }

    { // 0x0dc230: "LEVEL_CANDLEIGNITE" -> 100
        spr = 0;
        m_container->m_mgr->m_map.Lookup("LEVEL_CANDLEIGNITE", &spr);
        if (spr) spr->m_duration = 100;
    }

    { // 0x0dc257: "LEVEL_CANDLEUP" -> 100
        spr = 0;
        m_container->m_mgr->m_map.Lookup("LEVEL_CANDLEUP", &spr);
        if (spr) spr->m_duration = 100;
    }

    { // 0x0dc27e: "LEVEL_CANDLEDOWN" -> 100
        spr = 0;
        m_container->m_mgr->m_map.Lookup("LEVEL_CANDLEDOWN", &spr);
        if (spr) spr->m_duration = 100;
    }

    { // 0x0dc2a5: "LEVEL_GOLFBALLAIR2" -> 250
        spr = 0;
        m_container->m_mgr->m_map.Lookup("LEVEL_GOLFBALLAIR2", &spr);
        if (spr) spr->m_duration = 250;
    }

    { // 0x0dc2d1: "LEVEL_GOLFBALLHOLE" -> 250
        spr = 0;
        m_container->m_mgr->m_map.Lookup("LEVEL_GOLFBALLHOLE", &spr);
        if (spr) spr->m_duration = 250;
    }

    { // 0x0dc2f8: "LEVEL_GOLFBALLSINK" -> 250
        spr = 0;
        m_container->m_mgr->m_map.Lookup("LEVEL_GOLFBALLSINK", &spr);
        if (spr) spr->m_duration = 250;
    }

    { // 0x0dc31f: "GAME_EXPLOSION1" -> 100
        spr = 0;
        m_container->m_mgr->m_map.Lookup("GAME_EXPLOSION1", &spr);
        if (spr) spr->m_duration = 100;
    }

    { // 0x0dc346: "LEVEL_OUTLETHAZARD" -> 100
        spr = 0;
        m_container->m_mgr->m_map.Lookup("LEVEL_OUTLETHAZARD", &spr);
        if (spr) spr->m_duration = 100;
    }

    { // 0x0dc36d: "GRUNTZ_DEATHZ_DEATHZFREEZE1A" -> 100
        spr = 0;
        m_container->m_mgr->m_map.Lookup("GRUNTZ_DEATHZ_DEATHZFREEZE1A", &spr);
        if (spr) spr->m_duration = 100;
    }

    { // 0x0dc394: "GRUNTZ_DEATHZ_DEATHZFREEZE2A" -> 100
        spr = 0;
        m_container->m_mgr->m_map.Lookup("GRUNTZ_DEATHZ_DEATHZFREEZE2A", &spr);
        if (spr) spr->m_duration = 100;
    }

    { // 0x0dc3bb: "GRUNTZ_DEATHZ_DEATHZUNFREEZE1A" -> 100
        spr = 0;
        m_container->m_mgr->m_map.Lookup("GRUNTZ_DEATHZ_DEATHZUNFREEZE1A", &spr);
        if (spr) spr->m_duration = 100;
    }

    { // 0x0dc3e2: "GRUNTZ_DEATHZ_DEATHZUNFREEZE1A" -> 100 (duplicated!)
        spr = 0;
        m_container->m_mgr->m_map.Lookup("GRUNTZ_DEATHZ_DEATHZUNFREEZE1A", &spr);
        if (spr) spr->m_duration = 100;
    }

    { // 0x0dc409: "GRUNTZ_DEATHZ_RESSURECT" -> 100
        spr = 0;
        m_container->m_mgr->m_map.Lookup("GRUNTZ_DEATHZ_RESSURECT", &spr);
        if (spr) spr->m_duration = 100;
    }

    { // 0x0dc430: "GRUNTZ_DEATHZ_DEATHZSQUASH1A" -> 100
        spr = 0;
        m_container->m_mgr->m_map.Lookup("GRUNTZ_DEATHZ_DEATHZSQUASH1A", &spr);
        if (spr) spr->m_duration = 100;
    }

    { // 0x0dc457: "LEVEL_CLOUDHAZARDMOVE" -> 10000
        spr = 0;
        m_container->m_mgr->m_map.Lookup("LEVEL_CLOUDHAZARDMOVE", &spr);
        if (spr) spr->m_duration = 10000;
    }

    { // 0x0dc482: "LEVEL_CLOUDHAZARDKILL" -> 3000
        spr = 0;
        m_container->m_mgr->m_map.Lookup("LEVEL_CLOUDHAZARDKILL", &spr);
        if (spr) spr->m_duration = 3000;
    }

    { // 0x0dc4ad: "GRUNTZ_DEATHZ_DEATHZELECTROCUTE1" -> 1000
        spr = 0;
        m_container->m_mgr->m_map.Lookup("GRUNTZ_DEATHZ_DEATHZELECTROCUTE1", &spr);
        if (spr) spr->m_duration = 1000;
    }

    { // 0x0dc4d9: "GRUNTZ_NERFGUNGRUNT_NERFGUNZGRUNT" -> 1000
        spr = 0;
        m_container->m_mgr->m_map.Lookup("GRUNTZ_NERFGUNGRUNT_NERFGUNZGRUNT", &spr);
        if (spr) spr->m_duration = 1000;
    }

    { // 0x0dc500: "GRUNTZ_GUNHATGRUNT_GUNHATGRUNTP1" -> 1000
        spr = 0;
        m_container->m_mgr->m_map.Lookup("GRUNTZ_GUNHATGRUNT_GUNHATGRUNTP1", &spr);
        if (spr) spr->m_duration = 1000;
    }

    { // 0x0dc527: "GRUNTZ_WELDERGRUNT_WELDERZGRUNTP1" -> 1000
        spr = 0;
        m_container->m_mgr->m_map.Lookup("GRUNTZ_WELDERGRUNT_WELDERZGRUNTP1", &spr);
        if (spr) spr->m_duration = 1000;
    }

    { // 0x0dc54e: "LEVEL_PLANEHAZARDFLY" -> 5000
        spr = 0;
        m_container->m_mgr->m_map.Lookup("LEVEL_PLANEHAZARDFLY", &spr);
        if (spr) spr->m_duration = 5000;
    }

    return 1;
}
