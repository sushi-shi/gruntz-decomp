// Copyright (c) Creative Technology Ltd. / E-mu Systems, Inc.
// 1996-1997. All rights reserved.
// SFMANL100API excerpt; source and extraction details are in README.md.
#ifndef GRUNTZ_VENDOR_SFMAN100_H
#define GRUNTZ_VENDOR_SFMAN100_H

#include <SFMAN.H>

typedef struct SFMANL100TAG {
    // Global functions
    LRESULT (CDECL *SF_GetNumDevs) ( PWORD ) ;

    // Device functions which do not require allocation (opening)
    LRESULT (CDECL *SF_GetDevCaps)    ( SFDEVINDEX, PSFCAPSOBJECT ) ;
    LRESULT (CDECL *SF_GetRouterID)   ( SFDEVINDEX, PDWORD ) ;
    LRESULT (CDECL *SF_IsDeviceFree)  ( SFDEVINDEX ) ;
    LRESULT (CDECL *SF_Open)          ( SFDEVINDEX ) ;
    LRESULT (CDECL *SF_Close)         ( SFDEVINDEX ) ;
 
    // Query functions
    LRESULT (CDECL *SF_IsMIDIBankUsed) (SFDEVINDEX, PWORD);
    LRESULT (CDECL *SF_QueryStaticSampleMemorySize) ( SFDEVINDEX, PDWORD, PDWORD ) ;

    // Synthesizer Emulation Functions
    LRESULT (CDECL *SF_GetAllSynthEmulations) ( SFDEVINDEX, PSFBUFFEROBJECT ) ;
    LRESULT (CDECL *SF_GetSynthEmulation)     ( SFDEVINDEX, PSFBUFFEROBJECT ) ;
    LRESULT (CDECL *SF_SelectSynthEmulation)  ( SFDEVINDEX, WORD ) ;

    // Sound Bank management functions
    LRESULT (CDECL *SF_LoadBank)  ( SFDEVINDEX, PSFMIDILOCATION, PSFBUFFEROBJECT ) ;
    LRESULT (CDECL *SF_GetLoadedBankDescriptor) ( SFDEVINDEX, PSFMIDILOCATION,
                                            PSFBUFFEROBJECT ) ;
//    LRESULT (CDECL *SF_GetLoadedBankPathname) ( SFDEVINDEX, PSFMIDILOCATION,
//                                          PSFBUFFEROBJECT ) ;
    LRESULT (CDECL *SF_ClearLoadedBank) ( SFDEVINDEX, PSFMIDILOCATION ) ;

    // Sound Preset management functions
    LRESULT (CDECL *SF_LoadPreset) ( SFDEVINDEX, PSFMIDILOCATION,
                               PSFMIDILOCATION, PSFBUFFEROBJECT ) ;
    LRESULT (CDECL *SF_GetLoadedPresetDescriptor) ( SFDEVINDEX, PSFMIDILOCATION,

                                              PSFBUFFEROBJECT ) ;
    LRESULT (CDECL *SF_ClearLoadedPreset) (SFDEVINDEX, PSFMIDILOCATION ) ;
 
    // Sound Waveform management functions
    LRESULT (CDECL *SF_LoadWaveform)  ( SFDEVINDEX, PSFMIDILOCATION, PSFBUFFEROBJECT ) ;
    LRESULT (CDECL *SF_ClearLoadedWaveform)  ( SFDEVINDEX, PSFMIDILOCATION ) ;

    // Device Specific Message
    LRESULT (CDECL *SF_DeviceSpecificMessage) ( SFDEVINDEX, UINT, LPARAM, LPARAM ) ;

} SFMANL100API, *PSFMANL100API;

#endif
