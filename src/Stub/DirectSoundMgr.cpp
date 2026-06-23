// DirectSoundMgr.cpp - all DirectSoundMgr members have been migrated to the real
// TU (src/Dsndmgr/DirectSoundMgr.cpp): the ctor, the thin IDirectSound /
// IDirectSoundBuffer wrapper run, the device-init methods, the GetErrorString
// diagnostic formatter, and LockConvert (the Lock + 16->8-bit sample-conversion
// + Unlock data path, formerly mislabeled ErrorThunk_135f40). Nothing remains to
// stub here; kept as an empty unit so All.cpp's include stays valid.
