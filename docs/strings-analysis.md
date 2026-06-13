# GRUNTZ.EXE String-Table Analysis (decomp RE notes)

Binary: `GRUNTZ.EXE`, PE32 x86 GUI, 2,511,872 bytes, 6 sections, MSVC 5.0,
statically-linked MFC, WAP32 engine. Retail EN.

Version resource (authoritative):
- ProductName **Gruntz**, InternalName **Gruntz**, OriginalFilename **Gruntz.EXE**
- FileVersion / ProductVersion **1, 0, 0, 76** (i.e. 1.0.0.76)
- CompanyName **Monolith Productions Inc.**, FileDescription "The Ultimate Puzzle-Strategy-Action Game"
- LegalCopyright Copyright (c) 1998, Monolith Productions Inc.

Section map (`objdump -h`):
- `.text` VA 0x00401000 len 0x1e526b
- `.rdata` VA 0x005e7000 len 0x20fa8
- `.data` VA 0x00608000 len 0x21400  ← **RTTI `.?AV…` type_info names live here** (MSVC5 puts them in .data, not .rdata)
- `.idata` VA 0x006c3000 (imports)
- `.rsrc` VA 0x006c7000 (resources, incl. VERSION + string table)

Imported DLLs (`*.dll` strings): ADVAPI32, COMCTL32, comdlg32, CTL3D32, DDRAW,
DINPUT, DPLAYX, DSOUND, GDI32, KERNEL32, mss32 (Miles Sound System),
SFMAN32 (SoundFont manager), SHELL32, smackw32 (Smacker), USER32, VERSION, WINMM.

Leaked source paths (already in srcpaths.txt) confirm the module layout:
`C:\Proj\DDrawMgr` (DDRAWMGR/DIRPAL/DIRSURF), `C:\Proj\DinMgr2` (DinMgr2/InputDevice),
`C:\Proj\Dsndmgr` (DSNDMGR/DSndMgSR), `C:\Proj\Gruntz\GruntzMgr.cpp`,
`C:\Proj\NetMgr\NetMgr.cpp`, shared `c:\proj\incs\{ddrawmgr,netmgr}.h`.

---

## 1. RTTI mangled class names (the object model)

Total distinct `.?A[VU]…@@`: **231**.
- C-prefixed (game + MFC + WAP `CWap*`): **198**
- z-prefixed WAP32 runtime: **3** (+ `_zvec`/`_zdvec`, see below)
- Template instantiations: **2**
- CRT/iostream/AFX/COM struct types: **28**

### (a) WAP32 runtime / base classes
zlith-style runtime (lowercase `z`) and the WAP wrapper bases:
```
.?AVzErrHandling@@      error-handling/exception base
.?AVzPtrColl@@          pointer collection (intrusive container base)
.?AVzPTree@@            balanced/ptr tree
.?AV_zvec@@             vector helper
.?AV_zdvec@@            (double) vector helper
.?AVCWapObj@@           WAP base object (all WAP-managed objects derive here)
.?AVCWapX@@             WAP extension/COM-ish wrapper
```
This is the LithTech/WAP32 ("zlith") runtime substrate. `zPtrColl` / `zPTree` /
`zDArray` are the engine's own container library, distinct from MFC's CArray/CObArray.
Manager classes by name (from error strings, not RTTI): `DirectDrawMgr`,
`DirectInputMgr2`, `DirectSoundMgr`, `SFManager`, `RezSync`, `ButeMgr` (attribute
manager). `CDirectDrawMgr::GetGDISurface()` appears verbatim.

### (b) C-prefixed game / MFC classes (full list, 198)

MFC framework classes present (link-time, expected):
`CWinApp CWinThread CCmdTarget CCmdUI CTestCmdUI CWnd CFrameWnd CDialog CView
CCtrlView CScrollView CDocument CDC CClientDC CPaintDC CWindowDC CGdiObject
CBrush CPen CRgn CMenu CImageList CImage CButton CComboBox CEdit CStatic
CListBox CDragListBox CScrollBar CSliderCtrl CSpinButtonCtrl CProgressCtrl
CHeaderCtrl CHotKeyCtrl CListCtrl CTabCtrl CTreeCtrl CRichEditCtrl
CAnimateCtrl CStatusBarCtrl CToolBarCtrl CFile CMemFile CMirrorFile
CArchiveStream CRecentFileList CCommandLineInfo CByteArray CDWordArray
CStringArray CPtrArray CObArray CStringList CPtrList CObList
CMapStringToOb CMapStringToPtr CMapPtrToPtr CNoTrackObject CObject
CException CArchiveException CFileException CMemoryException
CNotSupportedException CResourceException CSimpleException CUserException`

**Game-specific classes (the decomp targets):**

App / engine / managers:
`CGruntzApp` (CWinApp subclass; window class "GruntzClass"), `CGameApp`,
`CGruntzWnd` `CGameWnd`, `CGruntzMgr` (the central game manager — maps to
GruntzMgr.cpp), `CGameMgr`, `CGruntzMapMgr` `CMapMgr`, `CNetMgr`.

Game state machine (`CState` base; `STATEZ_*` ids drive it):
`CState CSplashState CMenuState CHelpState CCreditsState CAttract CDemo CPlay
CMulti CMultiBootyState CBootyState`.

Grunt + gameplay objects (`CObject` base):
`CGrunt CGruntVoice CGruntCreationPoint CGruntStartingPoint CGruntPuddle
CDroppedObject CDroppedObjectShadow CObjectDropper`.

Grunt UI sprites:
`CGruntHealthSprite CGruntStaminaSprite CGruntPowerupSprite CGruntSelectedSprite
CGruntToySprite CGruntToyTimeSprite CGruntWingzTimeSprite CCursorSnapSprite
CStatusBarSprite`.

World hazards / level objects:
`CGiantRock CGiantRockLogic CRollingBall CRainCloud CSpotLight CLightFx CUFO
CPathHazard CStaticHazard CKitchenSlime CToobSpikez CBrickz CExplosion CTimeBomb
CBoomerang CProjectile CParticlez`.

Triggers / switches / logic (large family — switch/trigger system):
`CTileTrigger CTileTriggerLogic CTileTriggerSwitch CTileTriggerSwitchLogic
CTileTriggerTransition CTileMultiTriggerSwitchLogic CTileExclusiveTriggerSwitchLogic
CTileTimeTriggerLogic CTileTimeTriggerSwitchLogic CTileSecretTrigger
CTileSecretTriggerLogic CTileSecretTriggerSwitchLogic CCheckpointTrigger
CCheckpointTriggerSwitchLogic CSecretLevelTrigger CSecretTeleporterTrigger
CExitTrigger CVoiceTrigger CActionArea CGuardPoint CWayPoint
CCoveredPowerup CCoveredPowerupLogic CMovingLogic CUserLogic CUserBase`.

Teleport / warp / fortress:
`CTeleporter CWormhole CWarpStonePad CWarlord CFortressFlag`.

Eye-candy / ambient:
`CEyeCandy CEyeCandyAni CFrontCandy CFrontCandyAni CBehindCandy CBehindCandyAni
CMenuSparkle CToyPeek CAmbientSound CAmbientPosSound CRandomAmbientSound
CMenuSparkle CAniCycle CSimpleAnimation CSingleAnimation`.

Status-bar / HUD widget hierarchy (`CSBI_*` = status-bar item):
`CSBI_Image CSBI_ImageSet CSBI_ImageSetAni CSBI_MenuItem CSBI_RectOnly CSBI_SideTab
CSBI_GruntMachine CSBI_StatzTabArrow CSBI_StatzTabGruntBar CSBI_WarlordHead
CSBI_WellGoo CStatusBarItem CInGameIcon CInGameText CLevelTime CGruntPowerupSprite`.

Commands (input/command pattern):
`CGruntzCommand CGruntzSingleCommand CGruntzMultiCommand`.

Dialogs (multiplayer/options):
`CBattlezDlg CBattlezDlgColors CBattlezDlgCustom CMultiStartDlg CMultiHelpDlg
CCheckpointDlg`.

Misc/no-ops: `CDoNothing CDoNothingNormal CSingleFrameMessage`.

### (c) Template instantiations
```
.?AV?$zDArray@P8CUserLogic@@AEHXZ@@        zDArray< int(CUserLogic::*)() >  (array of member-fn ptrs → logic dispatch table)
.?AV?$CArray@PAUPLAYLISTINFOSTRUCT@@PAU1@@@ CArray<PLAYLISTINFOSTRUCT*>      (music playlist)
```
`PLAYLISTINFOSTRUCT` is a real struct in the music/SF2 subsystem.

### CRT/iostream/AFX struct types (link artifacts, 28)
`type_info`; iostream family `ios iostream istream ostream istream_withassign
ostream_withassign istrstream ostrstream strstream filebuf fstream ifstream
ofstream streambuf strstreambuf`; COM `IUnknown IStream ISequentialStream`;
MFC internal `_AFX_*`/`AFX_MODULE_STATE`/`AFX_MODULE_THREAD_STATE`/`CThreadData`.

---

## 2. Registry config (HKLM `Monolith Productions\Gruntz\1.0`)

API used: `RegOpenKeyA RegOpenKeyExA RegQueryValueA RegQueryValueExA`
(read-only path; the `Software\` prefix is assembled at runtime, not a literal).
Each value-name below maps **1:1 to a CGruntzMgr / options field**:

Audio:
```
Disable Audio
Disable Sound
Disable Music
Disable SoundFonts
Disable Sound Effectz
Sound Volume        Music Volume        Voice Volume
Sound               Music               Voice
```
Video:
```
Resolution
Disable Direct Video Access
Disable Fades
Disable High Quality Movie
Enable Emulation
Enable HiColor
Enable TrueColor
Enable Triple        (triple buffering)
```
Input/scroll:
```
Disable Joystick
Scroll Speed
```
Gameplay:
```
Easy Mode
Enable Cheatzfile
Player Name
Game Name
DefaultMaxGruntz
```
Per-save / last-used (note `%d`/`%i` suffixes ⇒ indexed slots):
```
LastMap        LastMultiMap        LastDiff%d        LastColour%d
LastMaxGruntz%d        Last Warp Level
Level %i Warp X        Level %i Warp Y
SG%i        Saved Game #%i        File%d
```
(`Easy`/`Hard`/`EasyDifficulty`/`HardDifficulty` are difficulty enum labels;
`EnableMenuItem`/`EnableWindow`/`ShowWindow`/`ScrollWindow`/`SaveDC`/etc. are Win32
API names, **not** registry values.)

---

## 3. Command-line tokens / launch modes (WinMain dispatch)

Verbatim tokens (drive the init/state dispatch):
```
PLAY        single-player launch
MULTI       multiplayer launch
DEMO        demo playback
ATTRACT     attract loop
SELECT      level/area select
EDIT        editor mode
HOST        MP host
JOIN        MP join
LOAD:       load saved game (LOAD:<name>)
LOADGAME    load game
NOLOGO      skip logo movie
NOMOVIES    skip all movies
LOBBYLAUNCH launched from a DirectPlay lobby (IDirectPlayLobby)
QUICKSTART  quick start
```
Related state/movie ids (consumed by the state machine, not necessarily CLI):
`LOGO INTRO TITLE PREVIEW CREDITZ FINAL TRAINING`.

---

## 4. Asset & data path strings (loader layout)

### Top-level archives
```
Gruntz.REZ      main resource archive (CRez* / RezSync loader)
GRUNTZ.VRZ      video/secondary resource archive
```
`CRezDir::Load Failed! (File is not sorted!)` ⇒ REZ directory must be sorted.

### CD-ROM probing (drive letter substituted via %c)
```
%c:\GAME\GRUNTZ.EXE     CD-presence check
%c:\DATA\%s
%c:\MOVIEZ\%s   /  %c:\Movies\%s
%c:\MUSIC\Gruntz.SF2  /  %c:\MUSIC\Gruntz4.SF2
"Can't get game settings/Please insert the Gruntz CD-ROM into the drive."
```

### Fonts
`large.fnt medium.fnt small.fnt tiny.fnt` (bitmap fonts), plus `ARIAL IMPACT` faces.

### SoundFonts / music (SFMAN32 + Miles)
`%s\Gruntz.SF2  %s\Gruntz4.SF2` (Gruntz4 = 4MB bank), `MIDIZ`, `MIDI%i`.

### Attribute / config text files (ButeMgr-driven)
```
attributez.txt / Attributez.txt   game attribute database (ButeMgr)
dwrects.txt                       dword-rect table
VERSION.TXT / CHEATZ.TXT          (CHEATZ.TXT gated by "Enable Cheatzfile")
```

### FEC files (LithTech front-end config / event tables)
```
Gruntz.FEC  GruntzLo.FEC
"Opened FEC File %s"   "FEC File Version: %d.%d"
```

### World / level naming (asset loader expects these directory/key patterns)
```
WORLDZ\LEVEL%i      WORLDZ\TRAINING%i
*.WWD / .WWD        world/level files (WAP "World Data")
custom\*.wwd        user/custom maps
AREA%i  AREA%i_WORLDZ   (AREA1..AREA8)
CUSTOM_WORLD  CUSTOM_WORLDINFO
\SCREENZ\%s  \SCREENZ\%sTEXT  \SCREENZ\TITLE%d   (full-screen images + caption text)
.PID  .PAL  .PCX  .ANI  .BMP  (PID=sprite/picture data, PAL=palette, ANI=animation)
Gruntz%04i.BMP    (screenshot output, Gruntz0001.BMP …)
```

### Sprite / animation resource-name namespaces (string keys into the REZ)
Top-level prefixes seen as `%s`-formatted lookups:
`GRUNTZ_<grunttype>`, `IMAGEZ_<x>` (`IMAGEZ_%s`), `ANIZ_<x>` (`ANIZ_%s`),
`SOUNDZ_<x>` (`SOUNDZ_%s`), `VOICES_<x>` (`VOICES_%s`, `VOICES_%s_%s`),
`GAME_*`, `LEVEL_*`, `MENU_*`, `STATEZ_*`, `WARLORDZ_*`, `TOOLZ_*`, `TOYZ_*`,
`POWERUPZ_*`, `GRUNTZ_PALETTEZ_%s`, `GRUNTZ_WARLORDZ_%s_BOOTY/_JOY`.

---

## 5. printf / sprintf format strings (function-signature & log/error evidence)

Map-load validators (CGruntzMapMgr; reveal x,y int signature):
```
Bad {brickz|rock|trigger|switch|secret switch|secret trigger|toggle switch|
     toggle-bridge trigger|up-down switch|hold switch|multi switch|time switch|
     once-only switch|pressure plate|covered powerup} at: x=%d, y=%d
No {switch|trigger} logic found for {plate|switch} at: x=%d, y=%d
No giant rock logic found {around|at}: x=%d, y=%d
Switch on an unknown tile at: x=%d, y=%d
Could not add Grunt: Player=%d, x=%d, y=%d
Plane %s: Bad map {tile|image set} value (%i) at %i,%i
```
Grunt state dump (full CGrunt field layout!):
```
[p=%d][g=%d][health=%d][x=%d][y=%d][dir=%d][stm=%d][ttl=%d][tool=%d][toy=%d]
[da=%d][wp=%d][iic=%d][qat=%d][qax=%d][ia=%d][iad=%d][rnd=%d]
```
(p=player, g=grunt id, stm=stamina, ttl=time-to-live, da/wp/iic/qat/qax/ia/iad
are internal fields — strong typing hints for CGrunt members.)

Assert / file-line reporter (the diagnostic call site):
```
%s, line %i
%s, line %i: %s (%i) - %s
Error: %s - %i
save/load out of sync at %s, %d
```
DDraw surface diagnostics (CDirectDrawMgr):
```
Surface Information for surface pointer %p:
Surface: width = %i, height = %i, depth = %i, pitch = %i
16-bit color bitmasks are: R = %04X, G = %04X, B = %04X
Source color key = %lu        Z Buffer bit depth = %s
Best Device number is %d      Device %d's rating is %d
```
Profiler/frame counters (debug HUD):
```
Delta=%i, Update=%i, Draw=%i, NumUpdates=%i
Fps = %i    FpsLimit = %i    Objs = %i    Sprites = %i    Pos = %i,%i
Input=%i, Activate=%i, Deact=%i, Update=%i, HitTest=%i, Draw=%i, Fixed=%i, StatusBar=%i, Flip=%i
Sent = %i, Rcvd = %i, Frame = %i Counter = %lu
```
Net/lobby (URL-style param packing → HTTP/registration ping):
```
Name=%s&Type=%i&Location=%s&Version=%lu      &Checksum=%lu
&Month=%i&Day=%i&Year=%i   &S=%lu&H=%i&M=%02i&SE=%02i
Using CmdDelay of %d and ResendDelay of %d.
*** %s has a different version of the game.   New Player Drop-In Request: %s
Not Receiving Data From Client: %s            Querying %s
```
Misc notable: `GUID format {%08X-%04X-%04X-%02X%02X-%02X%02X%02X%02X%02X%02X}`,
`Gruntz v%d.%d` / `Gruntz v%d.%d%d`, `Resolution is now %ix%ix%i`,
`Afx:%x:%x[:%x:%x:%x]` (MFC window-class autogen), `%sClass` (window-class builder),
`Questz: Stage %d of %s`, `Battlez: %s`, `Custom Level: %s`.

---

## 6. Error / diagnostic / status messages

Resource string-table IDs (decimal from VERSION dump; unlock/expiry path):
```
0x800c locked      0x800d expired      0x800e bad key      0x800f alpha-locked
0x81ef CONVERSION powerup help text
```
Init/load failures (concatenated in the string table):
```
"Unable to initialize the game."   "Unable to load the resource file."
"Unable to load the voice resource file."
"Can't initialize DirectDraw."  "Can't initialize the page manager."
"Can't get the primary surface.  DirectX 5 is required."
"Can't create the background page."   "Can't launch the application."
"CDirectDrawMgr::GetGDISurface() - Cannot get the GDI surface!"
"ERROR - Cannot Save Game."   "This saved game cannot be loaded and should be deleted."
"Unable to continue game."
```
ButeMgr (attribute-file parser) errors — confirms ButeMgr is compiled in:
```
ButeMgr (%d):  A formatting error in the attribute file was encountered
ButeMgr (%d):  Bad symbol encountered.    Invalid token encountered.
ButeMgr:  duplicate {symbol|tag} encountered - %s
ButeMgr:  Invalid tag specified - [%s]    Symbol not found - [%s]:%s
ButeMgr:  Type mismatch - [%s]:%s
```
DirectX symbolic error tables (string lookups of HRESULTs): `DDERR_*`
(GENERIC, NO3D, NOGDI, NOTFOUND, NOVSYNCHW, OUTOFCAPS, XALIGN, NODIRECTDRAWHW,
DIRECTDRAWALREADYCREATED), `DIERR_*` (ACQUIRED, INPUTLOST, NOTFOUND, READONLY,
OLDDIRECTINPUTVERSION), `DSERR_*` (ALLOCATED, BADFORMAT, NODRIVER, GENERIC),
`DPERR_*` (full DirectPlay set), each with `DD_OK/DS_OK/DP_OK`.
"The application requires a newer version of DirectInput."
CRT runtime errors present (R6002, R6008-R6028, "Runtime Error!",
"Microsoft Visual C++ Runtime Library", "- unable to initialize heap",
"- unable to open console device") = standard MSVC5 CRT.

---

## 7. Cheats / debug features / easter eggs

Cheat-engine internals (registry/file backed):
```
Cheatz   NonCheat   NumCheatz   Cheat%i   BootyCheatz
Enable Cheatzfile   "Cheatz cleared"
"Congratulations!  You have just enabled %d new cheats!"
"WARP letterz recovered! Prepare to receive your cheat codez!"
"WARP letterz not recovered! No cheatz for you."
"Please Wait...3Since you have cheated, you may not save your game."
GAME_MAJORCHEAT   GAME_MINORCHEAT   (cheat-acknowledge sounds)
```
Debug feature tokens (developer console / debug command parser):
```
DEBUG_GRUNTTYPE   DEBUG_JUMPLEVEL   DEBUG_POSITION   DEBUG_SETSKILL
DPRINTF   FILEAPPEND   STDOUT
World Position Display   "Enter New World Position"
```
Easter-egg / signature strings:
```
"Brian L. Goble is a programming God..."          (Goble = lead programmer)
"My name is Kevin Lambert.  You typed in my cheat code.  Prepare to die."
"Hey, how did you get this cheat?"
"They should call you Cheat Cheatelson from Cheatstown Virginia who lives at
 1105 Cheat Circle just behind the CheatMart superstore."
"Scrollz are cool!"   "Monolith Rulez..."   "Now is the time at Monolith when we dance"
"We will make no commentz at this time, but check out the Gruntz website at
 http://www.gruntzgoo.com!"
http://www.gruntzgoo.com/
"Gruntz, start your enginez"  "I get by with a little help from my friendz"
```

---

## 8. Window classes, mutex/event, GUIDs

- Window class: **`GruntzClass`** (registered via `RegisterClassA`); template
  builder `%sClass`; MFC auto class `Afx:%x:%x[:%x:%x:%x]`.
- Smacker playback window: **"Smacker Video Window"**.
- App/Wnd classes from RTTI: `CGruntzApp`, `CGruntzWnd`, `CGameWnd`.
- **No registry-format GUID string literals** were found ({xxxxxxxx-…}); the only
  GUID-related text is the *format string* `{%08X-%04X-%04X-…}` and the
  CLSID/CGGC tokens — DirectPlay/app GUIDs are stored as binary, not text.
- No explicit named mutex/event string was found in the table (likely built at
  runtime or unnamed single-instance guard).

---

## 9. Third-party / toolchain fingerprints

- **Miles Sound System**: `mss32.dll` (digital audio + XMI/MIDI). No printable
  Miles version banner survived in the table.
- **AIL SoundFont**: `SFMAN32.DLL` + `SFManager` (Gruntz.SF2 / Gruntz4.SF2 banks).
- **Smacker** (RAD): `smackw32.dll`, named exports
  `_SmackOpen@12 _SmackClose@4 _SmackDoFrame@4 _SmackNextFrame@4 _SmackGoto@8
   _SmackWait@4 _SmackToBuffer@28 _SmackToBufferRect@8 _SmackSoundOnOff@8
   _SmackSoundUseDirectSound@4`, window "Smacker Video Window".
- **MFC**: statically linked (RTTI shows full MFC class set; `Afx:` window classes).
- **MSVC 5.0 CRT**: "Microsoft Visual C++ Runtime Library", R60xx error codes,
  iostream RTTI (`ios/istream/ofstream/strstream…`), `VC20XC00U` marker.
- **CTL3D32.DLL** (3D dialog controls), **COMCTL32** (common controls).
- **DirectX 5** baseline: "DirectX 5 is required"; DDRAW/DSOUND/DINPUT/DPLAYX.

---

## 10. Multiplayer / DirectPlay

- Lib: `DPLAYX.dll`; launch via `LOBBYLAUNCH` (IDirectPlayLobby::RunApplication).
- NetMgr module = `C:\Proj\NetMgr\NetMgr.cpp` (class `CNetMgr`), `RezSync` for
  resource sync, `Using CmdDelay of %d and ResendDelay of %d.` (lockstep model).
- Modes: `MULTI HOST JOIN`, single vs multi via `SINGLEPLAYER`/`MULTIPLAYER`,
  `GAME_MULTI`, MP UI in `MENU_MULTIPLAYER_*`, status bar `GAME_STATUSBAR_TABZ_MULTIPLAYERTAB_*`.
- Battlez (vs. Questz) mode: `BATTLEZ`/`Gruntz Battlez`/`Battlez Setup`/
  `Custom Battlez Level`; objective text describes capturing enemy FORTZ.
- Session/connection messages: "A new {session|player|group} cannot be created.",
  "A player has lost the connection to the session.",
  "*** A player had a different version of the game.",
  "Unable to verify custom level with other players. The game will not start.",
  "New Player Drop-In Request: %s", "&Drop Player", `Chat`/`GAME_CHATBOX`.
- Full `DPERR_*` symbolic table + verbose IDirectPlay2/IDirectPlayLobby HRESULT
  descriptions are embedded (used by a stringify-error helper).
- "Gruntz Network Configuration Help" dialog; no plaintext DPlay service-provider
  GUID/name (binary).

---

## 11. Other RE-useful data

### Grunt-type roster (36; each → a sprite namespace `GRUNTZ_<TYPE>` + CGrunt subtype)
```
NORMALGRUNT  WANDGRUNT  CLUBGRUNT  SWORDGRUNT  GLOVEZGRUNT  GAUNTLETZGRUNT
SHOVELGRUNT  SPRINGGRUNT  GOOBERGRUNT  BOMBGRUNT  TIMEBOMBGRUNT  ROCKGRUNT
BRICKGRUNT  BOOMERANGGRUNT  NERFGUNGRUNT  GUNHATGRUNT  SHIELDGRUNT  SPYGRUNT
TOOBGRUNT  TOOBWATERGRUNT  WELDERGRUNT  WINGZGRUNT  GRAVITYBOOTZGRUNT
WARPSTONEGRUNT  SCROLLGRUNT  REAPERGRUNT  POGOSTICKGRUNT  YOYOGRUNT
JUMPROPEGRUNT  BEACHBALLGRUNT  BIGWHEELGRUNT  BABYWALKERGRUNT  GOKARTGRUNT
JACKINTHEBOXGRUNT  SQUEAKTOYGRUNT  HAREKRISHNAGRUNT
```
### Toolz (22) and Toyz (10) namespaces
```
TOOLZ_: BOMBZ BOOMERANGZ BRICKZ CLUBZ GAUNTLETZ GLOVEZ GOOBERZ GRAVITYBOOTZ
        GUNHATZ NERFGUNZ ROCKZ SHIELDZ SHOVELZ SPRINGZ SPYZ SWORDZ TIMEBOMBZ
        TOOBZ WANDZ WARPSTONEZ1 WELDERZ WINGZ
TOYZ_:  BABYWALKERZ BEACHBALLZ BIGWHEELZ GOKARTZ JACKINTHEBOXZ JUMPROPEZ
        POGOSTICKZ SCROLLZ SQUEAKTOYZ YOYOZ
```
### Warlordz (enemy bosses): `KING NAPOLEAN PATTON VIKING` (`WARLORDZ_<name>`).
### Powerupz: `POWERUPZ_{COIN,GHOST,MINICAM,ROIDZ,CONVERSION}`.
### Color variants (tool/toy ownership tint, ~17 colors):
`BLACK BLUE CYAN DKBLUE DKGREEN DKRED DKYELLOW GREEN GREY HOTPINK ORANGE PINK
PURPLE RED TURQ WHITE YELLOW` × `{TOOL,TOY}`.
### State machine states: `STATEZ_{SPLASH,MENU,HELP,CREDITZ,ATTRACT,PREVIEW,BOOTY,MULTI}`.
### Directions: `NORTH SOUTH EAST WEST NORTH/SOUTH-EAST/WEST` (8-dir movement).
### Areas/stages: `AREA1..AREA8`, `STAGE1..STAGE4`, `MENU_QUESTZ_AREA1..8`.
### Sound categories used as `%s` keys: `GAME_* LEVEL_* MENU_* AMBIENT%d`,
e.g. `GAME_REZMACHINE GAME_TELEPORTER GAME_TIMEBOMB LEVEL_UFO LEVEL_ROLLINGBALL`.

---

## Surprising / notable

- **Pre-release/alpha unlock-key machinery is still compiled into the RETAIL
  1.0.0.76 build.** Strings `Unlock UnlockFile`, "Enter your key below to unlock
  Gruntz:", "...registered alpha testers only...", "...still locked...",
  "...has expired...", "...the specified key is not correct..." (string-table IDs
  0x800c-0x800f), plus the URL-param registration ping
  (`Name=%s&Type=%i&Location=%s&Version=%lu&Checksum=%lu`). A locking/expiry/
  unlock-by-key code path exists even though retail is unlocked.
- **"Gruntz Pre-Release"** and **"1869577261I Monolith internal review copy. Do
  not distribute. We know who you are."** survive in the table → built from an
  internal/review source tree.
- **"Alpha Version, Build %i, …"** format string present (alpha build banner).
- Debug HUD / profiler instrumentation (`Fps=`, `Delta/Update/Draw`,
  `DEBUG_*`, `DPRINTF`, `World Position Display`) compiled in → a debug overlay is
  reachable; good for matching the debug-render functions.
- "Gruntz in Space" / "Gruntz4.SF2" hint at the 4MB SoundFont and possibly
  unused/extra content.
- RTTI is in `.data` (MSVC5 behavior) — when locating vtables/type_info during
  decomp, scan `.data` (0x00608000+), not `.rdata`.
- The complete WAP32 container/runtime (`zPtrColl zPTree zDArray zErrHandling
  _zvec CWapObj CWapX`) is statically linked → the engine substrate is fully
  recoverable from this single EXE; `ButeMgr` and `CRez*`/`RezSync` are the
  data-pipeline backbone.
