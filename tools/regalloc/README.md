# cl 5.0 register-allocator probe harness

Reproduces the measurements in
`docs/relevations/cl5-callcrossing-ebx-first-by-use-schedule.md`: cl 5.0's
`c2.exe` carries VC6's preference table `{EAX,ECX,EDX,ESI,EDI,EBX,EBP}` (2
copies) but assigns call-crossing values by USE SCHEDULE, EBX to the first-used
value, EBP as the fourth callee-saved register when frameless.

```sh
# each probe defines 3-4 values across a call, then uses them in a chosen order
wine "$MSVC_DIR/bin/cl.exe" /nologo /c /O2 /MT /FAs tools/regalloc/probe3-use-ordered.cpp
awk '/PROC/,/ENDP/' probe3-use-ordered.asm | grep -E 'mov (ebx|esi|edi|ebp), eax'
```

Confirm the preference table lives in the pinned c2.exe:

```sh
python3 -c "import struct; d=open('build/il-probe/re/c2.exe','rb').read(); \
print(hex(d.find(struct.pack('<8I',1,2,3,7,8,4,6,0))))"
```

The LEVER: when retail holds a value in EBX and our compile holds it in
ESI/EDI, reorder that value's first post-call use to lead. The residual ESI/EDI
split is schedule/handle state (sort it with the IL tap, build/il-probe/).
