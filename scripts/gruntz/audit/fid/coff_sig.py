#!/usr/bin/env python3
"""
COFF i386 parser + masked-byte signature extractor for MS static libs.

For each .obj member of LIBCMT.LIB / NAFXCW.LIB we:
  - parse the COFF section table, symbol table, string table
  - for every code (.text / .text$*) section, take its raw bytes
  - build a relocation MASK: bytes covered by a relocation become wildcards
    (DIR32 / DIR32NB / REL32 each cover 4 bytes at the reloc offset)
  - associate each external function symbol defined in that section with the
    section's masked body (offset = symbol Value within section)

Output: a pickle of signature records:
  {name, lib, member, sec_index, sym_value, length, bytes(hex), mask(hex)}
plus a JSON summary.
"""
import os, sys, struct, json, pickle

IMAGE_SCN_CNT_CODE       = 0x00000020
IMAGE_SCN_MEM_EXECUTE    = 0x20000000
IMAGE_SCN_LNK_REMOVE     = 0x00000800  # not used for code

# Reloc types (i386)
REL_DIR32   = 0x0006
REL_DIR32NB = 0x0007
REL_REL32   = 0x0014
REL_SECTION = 0x000A
REL_SECREL  = 0x000B
# width of the field each reloc patches (in bytes)
RELOC_WIDTH = {REL_DIR32:4, REL_DIR32NB:4, REL_REL32:4, REL_SECREL:4, REL_SECTION:2}

def read_cstr(buf, off):
    e = buf.find(b'\0', off)
    return buf[off:e].decode('latin1')

class Coff:
    def __init__(self, data):
        self.data = data
        (self.machine, self.nsec, self.timestamp, self.symptr,
         self.nsym, self.opthdr, self.chars) = struct.unpack_from('<HHIIIHH', data, 0)
        self.strtab_off = self.symptr + self.nsym*18
        # string table
        if self.strtab_off + 4 <= len(data):
            self.strtab_size = struct.unpack_from('<I', data, self.strtab_off)[0]
            self.strtab = data[self.strtab_off:self.strtab_off+self.strtab_size]
        else:
            self.strtab = b''
        self._parse_sections()
        self._parse_symbols()

    def _name(self, raw8):
        if raw8[:4] == b'\0\0\0\0':
            off = struct.unpack('<I', raw8[4:8])[0]
            return read_cstr(self.strtab, off)
        return raw8.rstrip(b'\0').decode('latin1')

    def _parse_sections(self):
        self.sections = []
        base = 20 + self.opthdr
        for i in range(self.nsec):
            o = base + i*40
            name = self._name(self.data[o:o+8])
            (vsize, vaddr, rawsize, rawptr, relptr, lnoptr,
             nrel, nln, chars) = struct.unpack_from('<IIIIIIHHI', self.data, o+8)
            sec = dict(index=i+1, name=name, vsize=vsize, vaddr=vaddr,
                       rawsize=rawsize, rawptr=rawptr, relptr=relptr,
                       nrel=nrel, chars=chars, relocs=[])
            self.sections.append(sec)
        # parse relocs
        for sec in self.sections:
            if sec['nrel'] and sec['relptr']:
                o = sec['relptr']
                for _ in range(sec['nrel']):
                    vaddr, symidx, rtype = struct.unpack_from('<IIH', self.data, o)
                    sec['relocs'].append((vaddr, symidx, rtype))
                    o += 10

    def _parse_symbols(self):
        self.symbols = []
        i = 0
        while i < self.nsym:
            o = self.symptr + i*18
            raw = self.data[o:o+18]
            name = self._name(raw[:8])
            value, secnum, typ, sclass, naux = struct.unpack_from('<IhHBB', raw, 8)
            self.symbols.append(dict(name=name, value=value, secnum=secnum,
                                     typ=typ, sclass=sclass, naux=naux, idx=i))
            i += 1 + naux

    def code_section_indices(self):
        out = []
        for sec in self.sections:
            if (sec['chars'] & IMAGE_SCN_CNT_CODE) or (sec['chars'] & IMAGE_SCN_MEM_EXECUTE):
                out.append(sec['index'])
        return set(out)

def extract_signatures(path, lib, member, min_len):
    data = open(path,'rb').read()
    try:
        c = Coff(data)
    except Exception as e:
        return [], f"parse-error:{e}"
    if c.machine != 0x14c:
        return [], f"not-i386:{c.machine:#x}"
    code_idx = c.code_section_indices()
    sec_by_index = {s['index']: s for s in c.sections}

    # group defined function/code symbols by section
    sigs = []
    # collect candidate symbols: External, defined in a code section, function-typed OR sclass external
    for sym in c.symbols:
        if sym['secnum'] <= 0:        # undefined / absolute / debug
            continue
        if sym['secnum'] not in code_idx:
            continue
        # storage class: 2=external, 3=static. We want callable bodies => external
        # (static-class .text label symbols like ".text" have empty/dot names)
        nm = sym['name']
        if not nm or nm.startswith('.'):
            continue
        if sym['sclass'] not in (2,3):
            continue
        sec = sec_by_index[sym['secnum']]
        rawptr = sec['rawptr']; rawsize = sec['rawsize']
        if rawsize == 0 or rawptr == 0:
            continue
        body = bytearray(data[rawptr:rawptr+rawsize])
        if len(body) < rawsize:
            continue
        # the symbol's value is its offset within the section; the function body
        # runs from sym.value to (typically) the section end for a COMDAT (one
        # symbol per section). If multiple external syms share a section, we cut
        # at the next symbol offset.
        start = sym['value']
        # find next external symbol offset in same section
        others = sorted(s['value'] for s in c.symbols
                        if s['secnum']==sym['secnum'] and s['value']>start
                        and s['name'] and not s['name'].startswith('.')
                        and s['sclass'] in (2,3))
        end = others[0] if others else rawsize
        flen = end - start
        if flen < min_len:
            continue
        seg = bytearray(body[start:end])
        mask = bytearray(b'\xff' * flen)   # 0xff = must-match
        for (vaddr, symidx, rtype) in sec['relocs']:
            roff = vaddr - start
            w = RELOC_WIDTH.get(rtype, 4)
            if 0 <= roff < flen:
                for k in range(w):
                    if roff+k < flen:
                        mask[roff+k] = 0x00   # wildcard
        sigs.append(dict(name=nm, lib=lib, member=member,
                         sec=sym['secnum'], val=start, length=flen,
                         bytes=bytes(seg).hex(), mask=bytes(mask).hex()))
    return sigs, None

def main():
    # usage: coff_sig <name_objs dir> [<name_objs dir> ...] <out.pkl> [min_len]
    args = list(sys.argv[1:])
    min_len = 1
    if args and args[-1].isdigit():
        min_len = int(args[-1]); args = args[:-1]
    out_pkl  = args[-1]
    obj_dirs = args[:-1]   # one or more <name>_objs dirs; lib label inferred from basename
    all_sigs = []
    errors = {}
    counts = {}
    for d in obj_dirs:
        lib = os.path.basename(str(d).rstrip("/")).replace("_objs", "").upper()
        members = sorted(f for f in os.listdir(d) if f.endswith('.obj'))
        nfunc = 0
        for m in members:
            sigs, err = extract_signatures(os.path.join(d,m), lib, m, min_len)
            if err:
                errors[f"{lib}/{m}"] = err
            all_sigs.extend(sigs)
            nfunc += len(sigs)
        counts[lib] = dict(members=len(members), functions=nfunc)
    with open(out_pkl,'wb') as f:
        pickle.dump(all_sigs, f)
    print(json.dumps(dict(counts=counts, total_sigs=len(all_sigs),
                          n_errors=len(errors), errors=errors), indent=2))

if __name__ == '__main__':
    main()
