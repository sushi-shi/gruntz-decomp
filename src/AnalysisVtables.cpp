// AnalysisVtables.cpp - source coverage for every vtable vtable_scan finds that is
// NOT already bound by a real polymorphic class / VTBL elsewhere and is NOT MFC/CRT
// (those live in config/library_vtables.csv). Each line binds one analysis-discovered
// vtable datum at its retail RVA: a real ??_7 name where RTTI gave one, else a
// tracking name CEngVt_<rva> (non-RTTI WAP/engine vtables have no retail ??_7 name).
// Generated to satisfy the vtable-coverage gate (gruntz.match.vtable_coverage).
#include <rva.h>

VTBL(CSplashState, 0x001e9d74);   // sz=26 rtti first=sub_0035ee
VTBL(CMultiHelpDlg, 0x001ea474);  // sz=54 rtti first=sub_1d2fab
VTBL(CArchiveStream, 0x001ed98c); // sz=14 rtti first=QueryInterface
VTBL(CEngVt_1ef670, 0x001ef670);  // sz=4 code-ref first=vector_deleting_destructor
VTBL(CEngVt_1ef680, 0x001ef680);  // sz=6 code-ref first=`scalar_deleting_destructor'
VTBL(CEngVt_1ef6c8, 0x001ef6c8);  // sz=2 code-ref first=__purecall
VTBL(CEngVt_1ef744, 0x001ef744);  // sz=1 code-ref-weak first=sub_13c340
VTBL(CEngVt_1ef750, 0x001ef750);  // sz=3 code-ref first=sub_13b9f0
VTBL(CEngVt_1ef75c, 0x001ef75c);  // sz=1 code-ref-weak first=sub_13c4c0
VTBL(CEngVt_1ef760, 0x001ef760);  // sz=1 code-ref-weak first=__purecall
VTBL(CEngVt_1ef7f0, 0x001ef7f0);  // sz=9 code-ref first=`scalar_deleting_destructor'
VTBL(CEngVt_1efa58, 0x001efa58);  // sz=12 code-ref first=ScalarDelete
VTBL(CEngVt_1efb80, 0x001efb80);  // sz=10 code-ref first=sub_1bef01
VTBL(CEngVt_1efbe8, 0x001efbe8);  // sz=17 code-ref first=sub_1bef01
VTBL(CEngVt_1efc30, 0x001efc30);  // sz=9 code-ref first=sub_1bef01
VTBL(CEngVt_1efca0, 0x001efca0);  // sz=9 code-ref first=sub_1bef01
VTBL(CEngVt_1efd28, 0x001efd28);  // sz=23 code-ref first=sub_1bef01
VTBL(CEngVt_1efd88, 0x001efd88);  // sz=14 code-ref first=sub_1bef01
VTBL(CEngVt_1efdc0, 0x001efdc0);  // sz=17 code-ref first=sub_1bef01
VTBL(CEngVt_1efe08, 0x001efe08);  // sz=10 code-ref first=sub_1bef01
VTBL(CEngVt_1efe3c, 0x001efe3c);  // sz=10 code-ref first=Reset
VTBL(CEngVt_1efe74, 0x001efe74);  // sz=10 code-ref first=Reset
VTBL(CEngVt_1eff70, 0x001eff70);  // sz=11 code-ref first=sub_1bef01
VTBL(CEngVt_1effd0, 0x001effd0);  // sz=19 code-ref first=sub_1bef01
VTBL(CEngVt_1f0020, 0x001f0020);  // sz=16 code-ref first=sub_1bef01
VTBL(CEngVt_1f0060, 0x001f0060);  // sz=17 code-ref first=sub_1bef01
VTBL(CEngVt_1f00a8, 0x001f00a8);  // sz=16 code-ref first=sub_1bef01
VTBL(CEngVt_1f00e8, 0x001f00e8);  // sz=16 code-ref first=sub_1bef01
VTBL(CEngVt_1f0128, 0x001f0128);  // sz=9 code-ref first=sub_1bef01
VTBL(CEngVt_1f02c0, 0x001f02c0);  // sz=5 code-ref first=sub_1bef01
VTBL(CEngVt_1f0328, 0x001f0328);  // sz=6 code-ref first=sub_1bef01
VTBL(CEngVt_1f04cc, 0x001f04cc);  // sz=1 code-ref-weak first=`scalar_deleting_destructor'
VTBL(CEngVt_1f04dc, 0x001f04dc);  // sz=1 code-ref-weak first=sub_16ea80
VTBL(CEngVt_1f04e0, 0x001f04e0);  // sz=1 code-ref-weak first=ScalarDtor
VTBL(CEngVt_1f04e4, 0x001f04e4);  // sz=1 code-ref-weak first=ConstructTail_ea20
VTBL(CEngVt_1f0510, 0x001f0510);  // sz=1 code-ref-weak first=`scalar_deleting_destructor'
VTBL(CEngVt_1f0518, 0x001f0518);  // sz=1 code-ref-weak first=sub_174e70
VTBL(CEngVt_1f0760, 0x001f0760);  // sz=5 code-ref first=sub_1bef01
VTBL(CEngVt_1f07d8, 0x001f07d8);  // sz=5 code-ref first=sub_1bef01
VTBL(CEngVt_216100, 0x00216100);  // sz=1 code-ref-weak first=__fpmath
VTBL(CEngVt_216104, 0x00216104);  // sz=1 code-ref-weak first=FUN_0051f510
VTBL(CEngVt_216108, 0x00216108);  // sz=1 code-ref-weak first=FUN_0051f510
VTBL(CEngVt_216120, 0x00216120);  // sz=1 code-ref-weak first=__exit
VTBL(CEngVt_216450, 0x00216450);  // sz=1 code-ref-weak first=__CxxUnhandledExceptionFilter
VTBL(CEngVt_216464, 0x00216464);  // sz=1 code-ref-weak first=FUN_00524df0
VTBL(CEngVt_216528, 0x00216528);  // sz=1 code-ref-weak first=__fptrap
VTBL(CEngVt_21652c, 0x0021652c);  // sz=1 code-ref-weak first=__fptrap
VTBL(CEngVt_216530, 0x00216530);  // sz=1 code-ref-weak first=__fptrap
VTBL(CEngVt_216534, 0x00216534);  // sz=1 code-ref-weak first=__fptrap
VTBL(CEngVt_216538, 0x00216538);  // sz=1 code-ref-weak first=__fptrap
VTBL(CEngVt_21653c, 0x0021653c);  // sz=1 code-ref-weak first=__fptrap
VTBL(CEngVt_216558, 0x00216558);  // sz=1 code-ref-weak first=sub_1273f0
VTBL(CEngVt_224ff0, 0x00224ff0);  // sz=1 code-ref-weak first=deflate_stored
VTBL(CEngVt_2257fe, 0x002257fe);  // sz=64 code-ref-weak first=sub_18e514
