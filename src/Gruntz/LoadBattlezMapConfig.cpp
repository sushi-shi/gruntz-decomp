// LoadBattlezMapConfig - the Battlez random-map setup function. Loads config
// values from the global CButeMgr "Battlez" group (GruntCreationTime,
// ResourceCreationTime, various chance/ratio/probability attributes), processes
// level-object spawn lists (three linked-list walks filtering by type identity),
// applies difficulty-based scaling (Easy/Normal/Hard), and seeds the random item
// distribution (ToolzPercent / ToyzPercent / BrickzPercent + item-name
// probabilities).
//
// @address: 0x025020
// @size:    0x981

extern "C" long __ftol(double d);         // 0x11f570
struct CButeMgr;
extern CButeMgr g_buteMgr;                // @0x6453d8
extern int g_battlezDifficultyCount;       // @0x62b738
extern int GruntRand();                    // @0x11fee0

struct PoolNode {
    PoolNode *m_next;           // +0x00
    int       m_tileX;          // +0x04
    int       m_tileY;          // +0x08
};
extern PoolNode *s_poolHead;   // @data: 0x645544

class CDWordArray {
public:
    char  m_padVtbl[0x04];
    int  *m_pData;
    int   m_nSize;
    int   m_nMaxSize;
    int   m_nGrowBy;
    void SetAtGrow(int nIndex, unsigned long newElement);   // @0x1b5144
};

class CButeMgr {
public:
    int GetDwordDef(char *tag, char *key, int def);   // @0x1721e0
    int GetIntDef(char *tag, char *key, int def);       // @0x171aa0
    int GetInt(char *tag, char *key);                   // @0x171af0
};

struct ParentObj {
    char  _pad0[0x2c];
    void *_2c;
    void *_30;
    char  _pad34[0x68 - 0x34];
    int   _68;
    char  _pad6c[0x70 - 0x6c];
    int   _70;
};

class CBattlezMapConfig {
public:
    int LoadBattlezMapConfig(ParentObj *parent, int someInt, int difficulty);
    int           m_00;
    ParentObj    *m_parent;
    int           m_08;
    int           m_0c;
    void         *m_10;
    int           m_14;
    int           m_18;
    int           m_pad1c[5];   // 20 B
    int           m_30;
    int           m_pad34[5];   // 20 B
    int           m_48;
    int           m_4c;
    int           m_50;
    int           m_54;
    int           m_58;
    int           m_5c;
    int           m_60;
    int           m_64;
    int           m_68;
    int           m_6c;
    int           m_70;
    int           m_74;
    int           m_78;
    int           m_7c;
    int           m_80;
    int           m_84;
    int           m_pad88[1];   // 4 B
    int           m_8c;
    int           m_90;
    int           m_94;
    int           m_98;
    int           m_pad9c[2];   // 8 B
    int           m_a4;
    int           m_pada8[1];   // 4 B
    int           m_ac;
    int           m_b0;
    int           m_padb4[3];   // 12 B
    int           m_c0;
    int           m_padc4[3];   // 12 B
    int           m_d0;
    int           m_d4;
    int           m_d8pad[1];   // 4 B
    CDWordArray   m_arrayA;
    CDWordArray   m_arrayB;
    int           m_pad104[15]; // 60 B
    int           m_140;
    int           m_cumul[41];    // +0x144 (indices correspond to offsets)
};

// @address: 0x025020
// @size:    0x981
int CBattlezMapConfig::LoadBattlezMapConfig(ParentObj *parent, int someInt, int difficulty)
{
    m_48 = 0;
    m_4c = 0;
    m_50 = 0;
    m_54 = 0;
    m_58 = 0;
    m_5c = 0;

    m_parent = parent;
    m_18 = someInt;
    m_08 = parent->_68;
    m_0c = parent->_70;
    m_10 = parent->_2c;
    m_14 = *(int *)((char *)parent->_2c + 0x2e4);
    m_00 = 1;

    m_48 = g_buteMgr.GetDwordDef("Battlez", "GruntCreationTime", 10000);
    m_54 = g_buteMgr.GetDwordDef("Battlez", "ResourceCreationTime", 10000);
    m_60 = g_buteMgr.GetDwordDef("Battlez", "GauntletzChance", 50);
    m_64 = g_buteMgr.GetDwordDef("Battlez", "ShovelzChance", 50);
    m_68 = g_buteMgr.GetDwordDef("Battlez", "SpyzChance", 50);
    m_6c = g_buteMgr.GetDwordDef("Battlez", "BrickzChance", 50);
    m_70 = g_buteMgr.GetDwordDef("Battlez", "GooberzChance", 50);
    m_74 = g_buteMgr.GetDwordDef("Battlez", "GruntRatio", 25);
    m_30 = g_buteMgr.GetDwordDef("Battlez", "DefenderChance", 50);

    // ---- list walk 1 (type 0x004017e4) ----
    {
        struct LNode  { LNode *_n; char _p[4]; void *_d; };
        struct LData  { char _[0x5c]; int _x; int _y;
                        char _2[0x7c-0x64]; void *_s; char _3[0x124-0x80]; int _id; };

        void *listPtr = *(void **)((char *)parent->_30 + 0x08);
        LNode *cur = *(LNode **)((char *)listPtr + 0x14);
        *(LNode **)((char *)listPtr + 0x64) = cur;

        LData *data;
        if (cur) {
            LNode *next = cur->_n;
            *(LNode **)((char *)listPtr + 0x64) = next;
            data = (LData *)cur->_d;
        } else {
            data = 0;
        }

        while (data) {
            if (*(int *)((char *)data->_s + 0x10) == 0x004017e4 && data->_id == someInt) {
                PoolNode *pn = s_poolHead;
                PoolNode *next = pn->m_next;
                PoolNode *alloc = 0;
                if (next) { alloc = (PoolNode *)((char *)pn + 4); s_poolHead = next; }
                int tx = data->_x / 32;
                int ty = data->_y / 32;
                if (alloc) { alloc->m_tileX = tx; alloc->m_tileY = ty; }
                m_arrayA.SetAtGrow(m_arrayA.m_nSize, (unsigned long)alloc);
            }

            listPtr = *(void **)((char *)parent->_30 + 0x08);
            cur = *(LNode **)((char *)listPtr + 0x64);
            if (cur) {
                LNode *next = cur->_n;
                *(LNode **)((char *)listPtr + 0x64) = next;
                data = (LData *)cur->_d;
            } else {
                data = 0;
            }
        }
    }

    // ---- list walk 2 (type 0x0040192e) ----
    {
        struct LNode2 { LNode2 *_n; char _p[4]; void *_d; };
        struct LData2 { char _[0x5c]; int _x; int _y;
                        char _2[0x7c-0x64]; void *_s; char _3[0x124-0x80]; int _id; };

        void *listPtr = *(void **)((char *)parent->_30 + 0x08);
        LNode2 *cur = *(LNode2 **)((char *)listPtr + 0x14);
        *(LNode2 **)((char *)listPtr + 0x64) = cur;

        LData2 *data;
        if (cur) {
            LNode2 *next = cur->_n;
            *(LNode2 **)((char *)listPtr + 0x64) = next;
            data = (LData2 *)cur->_d;
        } else {
            data = 0;
        }

        while (data) {
            if (*(int *)((char *)data->_s + 0x10) == 0x0040192e && data->_id == someInt) {
                m_d0 = data->_x / 32;
                m_d4 = data->_y / 32;
            }

            listPtr = *(void **)((char *)parent->_30 + 0x08);
            cur = *(LNode2 **)((char *)listPtr + 0x64);
            if (cur) {
                LNode2 *next = cur->_n;
                *(LNode2 **)((char *)listPtr + 0x64) = next;
                data = (LData2 *)cur->_d;
            } else {
                data = 0;
            }
        }
    }

    // ---- list walk 3 (type 0x00401087) ----
    {
        struct LNode3 { LNode3 *_n; char _p[4]; void *_d; };
        struct LData3 { char _[0x5c]; int _x; int _y;
                        char _2[0x7c-0x64]; void *_s; char _3[0x124-0x80]; int _id; };

        void *listPtr = *(void **)((char *)parent->_30 + 0x08);
        LNode3 *cur = *(LNode3 **)((char *)listPtr + 0x14);
        *(LNode3 **)((char *)listPtr + 0x64) = cur;

        LData3 *data;
        if (cur) {
            LNode3 *next = cur->_n;
            *(LNode3 **)((char *)listPtr + 0x64) = next;
            data = (LData3 *)cur->_d;
        } else {
            data = 0;
        }

        while (data) {
            if (*(int *)((char *)data->_s + 0x10) == 0x00401087 && data->_id == someInt) {
                PoolNode *pn = s_poolHead;
                PoolNode *next = pn->m_next;
                PoolNode *alloc = 0;
                if (next) { alloc = (PoolNode *)((char *)pn + 4); s_poolHead = next; }
                int tx = data->_x >> 5;
                int ty = data->_y >> 5;
                if (alloc) { alloc->m_tileX = tx; alloc->m_tileY = ty; }
                m_arrayB.SetAtGrow(m_arrayB.m_nSize, (unsigned long)alloc);
                *(int *)((char *)data + 0x08) |= 0x10000;
            }

            listPtr = *(void **)((char *)parent->_30 + 0x08);
            cur = *(LNode3 **)((char *)listPtr + 0x64);
            if (cur) {
                LNode3 *next = cur->_n;
                *(LNode3 **)((char *)listPtr + 0x64) = next;
                data = (LData3 *)cur->_d;
            } else {
                data = 0;
            }
        }
    }

    // ---- difficulty-based scaling ----
    if (difficulty == 0) {
        g_buteMgr.GetIntDef("Battlez", "EasyDifficulty", 100);
        g_battlezDifficultyCount = 0x14;
    } else if (difficulty == 1) {
        int dv = g_buteMgr.GetIntDef("Battlez", "NormalDifficulty", 50);
        g_battlezDifficultyCount = 0x0a;
        m_48 = (int)((double)m_48 * 0.01 * dv);
        m_54 = (int)((double)m_54 * 0.01 * dv);
    } else {
        int dv = g_buteMgr.GetIntDef("Battlez", "HardDifficulty", 25);
        g_battlezDifficultyCount = 0x05;
        m_48 = (int)((double)m_48 * 0.01 * dv);
        m_54 = (int)((double)m_54 * 0.01 * dv);
    }

    m_50 = 0;
    m_cumul[2] = 0;          // m_14c

    {
        int r = GruntRand();
        r = ((r % 4) + 5) * 1000;
        m_cumul[0] = r;      // m_144
    }
    m_cumul[1] = 0;          // m_148

    m_8c = 6;
    m_90 = 6;
    m_94 = 6;
    m_98 = 6;
    m_a4 = 8;

    {
        int tileCount = *(int *)((char *)m_0c + 0x0c);
        m_ac = (tileCount / 3) / 2;
        m_b0 = (tileCount / 3) / 2;
        m_c0 = tileCount / 4;
    }
    m_140 = 0;

    // ---- cumulative percentage chain ----
    {
        // m_cumul[3..40] ↔ +0x150..+0x1e4
        m_cumul[3] = g_buteMgr.GetInt("Battlez", "ToolzPercent");           // m_150

        m_cumul[4] = g_buteMgr.GetInt("Battlez", "ToyzPercent") + m_cumul[3];
        m_cumul[5] = g_buteMgr.GetInt("Battlez", "BrickzPercent") + m_cumul[4];
        m_cumul[6] = g_buteMgr.GetInt("Battlez", "RedBrick") + m_cumul[5];
        m_cumul[7] = g_buteMgr.GetInt("Battlez", "BlueBrick") + m_cumul[6];
        m_cumul[8] = g_buteMgr.GetInt("Battlez", "GoldBrick") + m_cumul[7];
        m_cumul[9] = g_buteMgr.GetInt("Battlez", "BlackBrick") + m_cumul[8];
        m_cumul[10] = g_buteMgr.GetInt("Battlez", "BabyWalkerz") + m_cumul[9];
        m_cumul[11] = g_buteMgr.GetInt("Battlez", "BeachBallz") + m_cumul[10];
        m_cumul[12] = g_buteMgr.GetInt("Battlez", "BigWheelz") + m_cumul[11];
        m_cumul[13] = g_buteMgr.GetInt("Battlez", "GoKartz") + m_cumul[12];
        m_cumul[14] = g_buteMgr.GetInt("Battlez", "JackInTheBoxz") + m_cumul[13];
        m_cumul[15] = g_buteMgr.GetInt("Battlez", "JumpRopez") + m_cumul[14];
        m_cumul[16] = g_buteMgr.GetInt("Battlez", "PogoStickz") + m_cumul[15];
        m_cumul[17] = g_buteMgr.GetInt("Battlez", "Scrollz") + m_cumul[16];
        m_cumul[18] = g_buteMgr.GetInt("Battlez", "SqueakToyz") + m_cumul[17];
        m_cumul[19] = g_buteMgr.GetInt("Battlez", "Yoyoz") + m_cumul[18];
        m_cumul[20] = g_buteMgr.GetInt("Battlez", "Bombz") + m_cumul[19];
        m_cumul[21] = g_buteMgr.GetInt("Battlez", "Boomerangz") + m_cumul[20];
        m_cumul[22] = g_buteMgr.GetInt("Battlez", "Brickz") + m_cumul[21];
        m_cumul[23] = g_buteMgr.GetInt("Battlez", "Clubz") + m_cumul[22];
        m_cumul[24] = g_buteMgr.GetInt("Battlez", "Gauntletz") + m_cumul[23];
        m_cumul[25] = g_buteMgr.GetInt("Battlez", "Glovez") + m_cumul[24];
        m_cumul[26] = g_buteMgr.GetInt("Battlez", "Gooberz") + m_cumul[25];
        m_cumul[27] = g_buteMgr.GetInt("Battlez", "GravityBootz") + m_cumul[26];
        m_cumul[28] = g_buteMgr.GetInt("Battlez", "GunHatz") + m_cumul[27];
        m_cumul[29] = g_buteMgr.GetInt("Battlez", "NerfGunz") + m_cumul[28];
        m_cumul[30] = g_buteMgr.GetInt("Battlez", "Rockz") + m_cumul[29];
        m_cumul[31] = g_buteMgr.GetInt("Battlez", "Shieldz") + m_cumul[30];
        m_cumul[32] = g_buteMgr.GetInt("Battlez", "Shovelz") + m_cumul[31];
        m_cumul[33] = g_buteMgr.GetInt("Battlez", "Springz") + m_cumul[32];
        m_cumul[34] = g_buteMgr.GetInt("Battlez", "Swordz") + m_cumul[33];
        m_cumul[35] = g_buteMgr.GetInt("Battlez", "TimeBombz") + m_cumul[34];
        m_cumul[36] = g_buteMgr.GetInt("Battlez", "Toobz") + m_cumul[35];
        m_cumul[37] = g_buteMgr.GetInt("Battlez", "Wandz") + m_cumul[36];
        m_cumul[38] = g_buteMgr.GetInt("Battlez", "Welderz") + m_cumul[37];
        m_cumul[39] = g_buteMgr.GetInt("Battlez", "Wingz") + m_cumul[38];
        m_cumul[40] = m_cumul[39];
    }

    m_78 = 0;
    m_80 = 0;
    m_7c = 0;
    m_84 = 0;

    return 1;
}
