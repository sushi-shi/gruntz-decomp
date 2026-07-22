#include <rva.h> // int aliases (i8..u64)
#include <Bute/ButeTree.h>
#include <Globals.h> // g_helperRefCount decl

#include <Gruntz/GruntStartingPoint.h> // g_zvecErrSentinel (ex .cpp extern)
DATA(0x002bf400)
i32 g_helperRefCount; // owner def (zero-init .bss)

struct AttractActorList;
struct CVariantSlot;
struct CActEntry;
class CDDrawPtrCollections;
typedef CDDrawPtrCollections CDirectDrawMgr; // one class, both spellings
SIZE_UNKNOWN();
class CGameWnd; // g_activeGameWnd's pointee (?g_activeGameWnd@@3PAVCGameWnd@@A)
struct CVariantSlot;
struct CDropEntry;
struct CVariantSlot;
struct CHaznEntry;
struct CKSlimeEntry;
struct CVariantSlot;
struct CPartEntry;
class CImagePool; // g_previewMgr (canonical <Image/ImagePool.h>; was CPreviewMgr view)
struct CProjActEntry;
struct CVariantSlot;
struct CVariantSlot;
struct CTBombEntry;
struct CVariantSlot;
struct CToobEntry;
struct CVariantSlot;
struct CVTrigEntry;
struct GLSResetMgr;
struct SFMANL101TAG;
typedef struct SFMANL101TAG SFMANL101API;
class CDDrawWorkerHost; // g_backView's real class (the CLevelPlane scroll plane)
struct ShadeDescr;

DATA(0x00051510)
extern char g_typeDesc1[];
DATA(0x00104358)
extern i32 g_screenTag;
DATA(0x001e8d10)
const i32 g_msgmap_CBattlezDlgColors = 6205544;
DATA(0x001e8e98)
extern void* g_battlezCustomMsgMap;
DATA(0x001e9068)
i32 g_idleSpriteIds[4] = {420, 475, 530, 585};
DATA(0x001e94b8)
const i32 g_msgmap_CCheckpointDlg = 6205544;
DATA(0x001ea3e0)
const double g_slimeSpeedNum = 32.0;
DATA(0x001ef698)
extern const double c_volScale = 100.0; // 0x5ef698  v / c_volScale, and the final * c_volScale
DATA(0x001ef6a0)
extern const double c_volNum = 1.0; // 0x5ef6a0  numerator of the reciprocal
DATA(0x001ef6a8)
extern const double c_powExp = 10.0; // 0x5ef6a8  pow() exponent
DATA(0x001ef6b0)
extern const double c_acosNorm = 2.0; // 0x5ef6b0  acos() normalizer arg
DATA(0x001f04f8)
extern const double g_motionNegHalf = -0.5;
DATA(0x0020cca4)
char s_codeD[] = "D";
DATA(0x0020d2e8)
char s_codeF[] = "F";
DATA(0x0020d7f4)
char s_codeM[] = "M";
DATA(0x0020d7fc)
char s_codeH[] = "H";
DATA(0x0020dc04)
char s_codeN[] = "N";
DATA(0x0020dc08)
char s_codeQ[] = "Q";
DATA(0x0020dc0c)
char s_codeO[] = "O";
DATA(0x0020df94)
char k_60df94[] = "S";
DATA(0x002111b0)
u8 g_titleBuf = 72;
DATA(0x0024553c)
extern "C" i32 g_areaHazardParam = 0;
DATA(0x002455f0)
extern "C" i32 g_levelBias100 = 0;
DATA(0x00248cec)
extern "C" i32 g_activePlayerCount = 0;
DATA(0x0024acb4)
i32 g_poolCount;
DATA(0x002bf428)
void* g_retAddrBreadcrumb;
DATA(0x002bf454)
void* g_projActName;

extern "C" {
    // g_opt_22bd64..g_opt_22bdd4 (the options-dialog staging cells, 0x22bd64..0x22bdd4)
    // are DEFINED in their owning TU src/Gruntz/VideoConfig.cpp (videoconfig.obj's .bss);
    // the reference externs stay in <Globals.h>.
    // The [Config] gate band (0x6455b4..0x6455e4) is DEFINED in src/Rez/RezSync.cpp
    // (its owner: Init loads all twelve from the .bute keys that name them).
    // g_profAccA/g_profAccB DEFINED in src/Gruntz/Play.cpp (owner TU).
}
