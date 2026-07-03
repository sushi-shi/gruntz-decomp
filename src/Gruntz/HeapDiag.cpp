// HeapDiag.cpp - the engine's heap self-check / walk diagnostic (RVA 0x118a30).
//
// A __cdecl diagnostic that runs the CRT heap consistency check (_heapchk),
// reports the status through the sibling status-reporter (0x118b50, which itself
// wraps OutputDebugStringA), and - when asked and the heap is NOT _HEAPOK - walks
// the heap to the first non-OK block and dumps it. Misfiled into the CRT/library
// backlog originally; it is game code (it calls the engine's own status reporter,
// not a CRT routine). The CRT heap primitives + OutputDebugStringA are external /
// reloc-masked.
#include <Win32.h>  // OutputDebugStringA
#include <malloc.h> // _HEAPINFO / _heapchk / _heapwalk / _HEAPOK / _USEDENTRY
#include <stdio.h>  // sprintf
#include <string.h> // memset

#include <rva.h>

// The engine wrappers retail calls here. The status reporter (0x118b50) takes the
// heap status - its real owner is this diagnostic TU (its sole caller), DEFINED
// below after HeapCheckDump (it follows in retail-RVA order). The heap-walk wrapper
// (0x1206b0, retail's _heapwalk re-attributed) is declared no-arg but invoked with
// &hinfo, so it is reached through a casted pointer to keep its no-arg mangled
// symbol; it stays external / reloc-masked (defined in src/Stub/ApiCallers.cpp).
namespace ApiCallerStubs {
    void winapi_118b50_OutputDebugStringA(i32 status);
    i32 winapi_1206b0_GetLastError_HeapValidate_HeapWalk();
} // namespace ApiCallerStubs
typedef i32(__cdecl* HeapWalkFn)(_HEAPINFO*);

// @early-stop
// 96.7%: body byte-exact. Residual is the shrink-wrapped `push esi` - retail defers
// the esi callee-save into the (conditionally-entered) walk block; cl pushes all 3
// callee-saved regs at the prologue. The 4-byte esp shift cascades through the first
// _HEAPINFO zero-stores + flips two je targets + the epilogue pop order. Not
// source-steerable (docs/patterns/shrink-wrapped-callee-save-push.md).
RVA(0x00118a30, 0xda)
int HeapCheckDump(int walkOnBad) {
    _HEAPINFO hinfo;
    char buf[80];
    int status = _heapchk();
    OutputDebugStringA("Checking heap...\n");
    ApiCallerStubs::winapi_118b50_OutputDebugStringA(status);
    if (walkOnBad == 0 || status == _HEAPOK) {
        return status;
    }
    memset(&hinfo, 0, sizeof(hinfo));
    ((HeapWalkFn)ApiCallerStubs::winapi_1206b0_GetLastError_HeapValidate_HeapWalk)(&hinfo);
    OutputDebugStringA("Walking heap...\n");
    hinfo._pentry = 0;
    int r = ((HeapWalkFn)ApiCallerStubs::winapi_1206b0_GetLastError_HeapValidate_HeapWalk)(&hinfo);
    while (r == _HEAPOK) {
        r = ((HeapWalkFn)ApiCallerStubs::winapi_1206b0_GetLastError_HeapValidate_HeapWalk)(&hinfo);
    }
    sprintf(
        buf,
        "HEAP: %6s block at %Fp of size %4.4X\n",
        hinfo._useflag == _USEDENTRY ? "USED" : "FREE",
        hinfo._pentry,
        hinfo._size
    );
    OutputDebugStringA(buf);
    ApiCallerStubs::winapi_118b50_OutputDebugStringA(r);
    OutputDebugStringA("Finished walking heap.");
    return status;
}

// 0x118b50: the heap-status reporter HeapCheckDump funnels through - trace a
// _heapchk()/_heapwalk() status code to the debugger. __cdecl(status). Re-homed
// from src/Stub/ApiCallers.cpp (this is its only caller).
namespace ApiCallerStubs {
    RVA(0x00118b50, 0x5b)
    void winapi_118b50_OutputDebugStringA(i32 status) {
        switch (status) {
            case -3:
                OutputDebugStringA("Heap return value: _HEAPBADBEGIN\n");
                return;
            case -4:
                OutputDebugStringA("Heap return value: _HEAPBADNODE\n");
                return;
            case -6:
                OutputDebugStringA("Heap return value: _HEAPBADPTR\n");
                return;
            case -1:
                OutputDebugStringA("Heap return value: _HEAPEMPTY\n");
                return;
            case -2:
                OutputDebugStringA("Heap return value: _HEAPOK\n");
                return;
            default:
                OutputDebugStringA("Heap return value: Unknown return value!\n");
                return;
        }
    }
} // namespace ApiCallerStubs
