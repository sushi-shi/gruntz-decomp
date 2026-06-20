// CGameRegistry.h - the global game-manager singleton (the object at *0x64556c).
// One canonical definition shared by CPlay.h (CPlay::Render reads the cue sink,
// viewport and resource map) and Grunt.h (the CGrunt animation resolvers read the
// visible-bounds gate and build HUD sprites via m_30->m_8->CreateSprite). Only the
// offsets those paths touch are modeled; everything else is padding.
#ifndef GRUNTZ_GRUNTZ_CGAMEREGISTRY_H
#define GRUNTZ_GRUNTZ_CGAMEREGISTRY_H

struct CSpriteFactory;  // +0x30 -> +0x08 factory (CreateSprite); Grunt.h completes it
class CGruntCueSink;    // +0x60 on-screen cue receiver; Grunt.h completes it

struct CSpriteFactoryHolder {   // the +0x30 resource/sprite-factory holder
    char m_pad0[0x8];
    CSpriteFactory *m_8;        // +0x08
};

// The tile occupancy grid (*g_pGameRegistry+0x70). A flat row-table of
// pointers (m_8); each row is an array of 0x1c-byte (7-dword) tile cells
// indexed by tile-x. CGrunt::LoadEntranceConfig stamps the grunt's footprint
// into the cell occupied by (m_10->m_5c>>5, m_10->m_60>>5): sets/clears bit
// 0x20 in cell byte+3 and writes a packed (m_1ec<<8)|m_1f0 owner word into
// cell[1]. m_c/m_10 are the grid dimensions (tile width/height bounds).
struct CTileGrid {
    char  m_pad0[0x8];
    int **m_8;          // +0x08  row-pointer table (m_8[tileY] = row base; cell = (int*)m_8[tileY] + tileX*7)
    int   m_c;          // +0x0c  width in tiles
    int   m_10;         // +0x10  height in tiles
};

struct CGameRegistry {
    char m_pad0[0x14];
    int   m_14;         // +0x14  has-window / dev flag (gates rect-update calls)
    char m_pad18[0x30 - 0x18];
    CSpriteFactoryHolder *m_30; // +0x30
    char m_pad34[0x60 - 0x34];
    CGruntCueSink *m_60;   // +0x60  cue sink / on-screen cue receiver (->Cue)
    char m_pad64[0x68 - 0x64];
    void *m_68;         // +0x68  cue sink B (message poster)
    char m_pad6c[0x70 - 0x6c];
    CTileGrid *m_70;    // +0x70  tile occupancy grid (LoadEntranceConfig)
    char m_pad74[0x8c - 0x74];
    int   m_8c;         // +0x8c  viewport X
    int   m_90;         // +0x90  viewport Y
    char m_pad94[0x134 - 0x94];
    int   m_134;        // +0x134 mode discriminator (==1 visible-bounds gate, ==2 alt path)
    char m_pad138[0x13c - 0x138];
    int   m_13c;        // +0x13c view min X
    int   m_140;        // +0x140 view min Y
    int   m_144;        // +0x144 view max X
    int   m_148;        // +0x148 view max Y
    char m_pad14c[0x15c - 0x14c];
    void *m_15c;        // +0x15c level/entity-tree holder
};

#endif  // GRUNTZ_GRUNTZ_CGAMEREGISTRY_H
