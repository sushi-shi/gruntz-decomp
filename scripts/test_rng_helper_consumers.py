"""Bounded production-object/original-PE controls for two RNG consumers.

No compilation or game execution. Check the ghost gate/draw protocol and four
two-brick one-based selections, not complete caller equivalence. Brick decoding
stops at the common enum join before embedded jump-table data; switch dispatch,
later tile writes, and unrelated arithmetic are outside this control. Changed
instruction shapes fail closed and need review, not automatic acceptance.
"""

from dataclasses import dataclass
import re
import struct
import unittest

from test_entrance_player_guard import Body, decode, require


GHOST = '?FindIdleGruntInBox@CBattlezMapConfig@@QAEPAVCGrunt@@HHHH@Z'
BRICK = '?BuildCellAttributes@CGruntzMapMgr@@QAEHHH@Z'
RAND = '_rand'
POINT = '__imp__PtInRect@12'
RAND_RVA = 0x11fee0
# Four exact instruction shapes, with threshold, enum and destinations checked
# separately. Wildcards deliberately allow the negative controls to be found.
LAYER = re.compile(
    rb'\xe8....\x99\xb9....\xf7\xf9\x33\xc0\x42\x83\xfa.'
    rb'\x0f\x9e\xc0\x05....\xe9....', re.S)


def imported_name(pe, slot):
    """Resolve the actual IAT slot through original PE import descriptors."""
    opt = struct.unpack_from('<I', pe.data, 0x3c)[0] + 24
    descriptor = struct.unpack_from('<I', pe.data, opt + 104)[0]
    for _ in range(128):
        fields = pe.read(descriptor, 20)
        require(fields is not None, 'unreadable import descriptor')
        original, _, _, name, first = struct.unpack('<IIIII', fields)
        if not any((original, name, first)):
            break
        for index in range(8192):
            payload = pe.read((original or first) + index * 4, 4)
            require(payload is not None, 'unreadable import lookup table')
            value = struct.unpack('<I', payload)[0]
            if not value:
                break
            if first + index * 4 == slot:
                require(not value & 0x80000000, 'unsupported ordinal import')
                text = bytearray()
                for pos in range(256):
                    char = pe.read(value + 2 + pos, 1)
                    require(char is not None, 'unreadable imported name')
                    if char == b'\0':
                        return text.decode('ascii')
                    text.extend(char)
                raise AssertionError('unterminated imported name')
        else:
            raise AssertionError('oversized import lookup table')
        descriptor += 20
    raise AssertionError(f'unresolved original IAT slot {slot:x}')


@dataclass
class Consumer:
    label: str
    data: bytes
    references: tuple = ()
    rva: int = 0
    pe: object = None

    def call_name(self, instruction):
        require(instruction.mnemonic == 'call', f'{self.label}: expected CALL')
        off = instruction.offset
        if self.pe is None:
            refs = [(kind, name) for site, kind, name in self.references
                    if off <= site < instruction.after]
            require(len(refs) == 1, f'{self.label}: missing/ambiguous call relocation')
            kind, name = refs[0]
            expected = 0x14 if self.data[off] == 0xe8 else 6
            require(kind == expected, f'{self.label}: wrong call relocation kind')
            if expected == 6:
                require(self.data[off:off + 2] == b'\xff\x15', 'unsupported indirect CALL')
            return name
        if self.data[off] == 0xe8:
            target = self.rva + instruction.after + struct.unpack_from('<i', self.data, off + 1)[0]
            return RAND if target == RAND_RVA else f'rva:{target:x}'
        require(self.data[off:off + 2] == b'\xff\x15', 'unsupported original indirect CALL')
        slot = struct.unpack_from('<I', self.data, off + 2)[0] - self.pe.image_base
        name = imported_name(self.pe, slot)
        return POINT if name == 'PtInRect' else f'import:{name}'

    def changed(self, offset, payload):
        data = self.data[:offset] + payload + self.data[offset + len(payload):]
        return Consumer(self.label + ' mutated', data, self.references, self.rva, self.pe)


def production(path, symbol):
    from gruntz.delink.coffx import Obj
    from gruntz.walls.pairscan import functions, fn_relocs

    require(path.is_file(), f'missing production object {path}; run gruntz build')
    obj = Obj(path)
    require(symbol in functions(obj), f'missing production owner {symbol}')
    section, start, end = functions(obj)[symbol]
    data = obj.section_payload(section)[start:end].rstrip(b'\xcc').rstrip(b'\x90')
    refs = tuple((off - start, kind, name)
                 for off, name, kind, _ in fn_relocs(obj, section, start, end))
    return Consumer(f'production {symbol}', data, refs)


def pair(instruction, mnemonic, operands=None):
    require(instruction.mnemonic == mnemonic
            and (operands is None or instruction.operands == operands),
            f'unsupported protocol at +{instruction.offset:x}: '
            f'{instruction.mnemonic} {instruction.operands}; expected {mnemonic} {operands}')


def check_ghost(consumer):
    body = Body(consumer.label, consumer.data, {}, consumer.references)
    code = list(body.instructions.values())
    calls = [(j, consumer.call_name(i)) for j, i in enumerate(code) if i.mnemonic == 'call']
    require([name for _, name in calls] == [POINT, RAND, RAND], 'ghost call identity/order changed')
    point, draw, discard = (j for j, _ in calls)
    pair(code[point + 1], 'test', 'eax,eax')
    pair(code[point + 2], 'je')
    skip = body.target(code[point + 2])
    # The complete local ghost gate: member, initialized caller result, test,
    # one conditional draw, conditional clear, then shared result test.
    require(draw == point + 7, 'unsupported ghost gate extent')
    pair(code[draw - 4], 'mov')
    kind = re.fullmatch(r'eax,DWORD PTR \[(esi|edi|ebx|ebp)\+0x258\]', code[draw - 4].operands)
    require(kind, 'ghost kind field/receiver changed')
    unit = kind[1]
    pair(code[draw - 3], 'mov')
    keep = re.fullmatch(r'(esi|edi|ebx|ebp),0x1', code[draw - 3].operands)
    require(keep and keep[1] != unit, 'missing independent caller-owned keep')
    keep = keep[1]
    pair(code[draw - 2], 'cmp', 'eax,0x36')
    pair(code[draw - 1], 'jne')
    for n, (mn, op) in enumerate((('cdq', ''), ('mov', 'ecx,0x64'),
                                  ('idiv', 'ecx'), ('cmp', 'edx,0x5'),
                                  ('jle', None), ('xor', f'{keep},{keep}'),
                                  ('test', f'{keep},{keep}'), ('je', None)), 1):
        pair(code[draw + n], mn, op)
    shared = code[draw + 7].offset
    require(body.target(code[draw - 1]) == shared
            and body.target(code[draw + 5]) == shared, 'wrong ghost draw/clear guard edge')
    require(body.target(code[draw + 8]) == skip, 'ghost rejection bypasses candidate skip')

    drops = [j for j, i in enumerate(code[:point])
             if i.mnemonic == 'mov' and i.operands == f'eax,DWORD PTR [{unit}+0x364]']
    require(len(drops) == 1, 'missing entrance-drop eligibility guard')
    drop = drops[0]
    pair(code[drop - 2], 'test', f'{unit},{unit}')
    pair(code[drop - 1], 'je')
    pair(code[drop + 1], 'test', 'eax,eax')
    pair(code[drop + 2], 'jne')
    require(all(body.target(code[j]) == skip for j in (drop - 1, drop + 2)),
            'null/drop eligibility changed RNG reachability')

    # Distance rejects before powered-up load; powered-up draw is deliberately
    # retained even though its return is unused. Do not prove distance arithmetic.
    pair(code[discard - 5], 'cmp')
    dist = re.fullmatch(r'(esi|edi|ebx|ebp),(esi|edi|ebx|ebp)', code[discard - 5].operands)
    require(dist, 'unsupported closest-candidate comparison')
    pair(code[discard - 4], 'jge')
    require(body.target(code[discard - 4]) == skip, 'closer-candidate guard changed')
    pair(code[discard - 3], 'mov', f'eax,DWORD PTR [{unit}+0x220]')
    pair(code[discard - 2], 'test', 'eax,eax')
    pair(code[discard - 1], 'je')
    require(body.target(code[discard - 1]) == code[discard].after,
            'powered-up guard no longer skips exactly its draw')
    pair(code[discard + 1], 'mov')
    require(re.fullmatch(r'DWORD PTR \[esp\+0x[0-9a-f]+\],' + unit,
                         code[discard + 1].operands), 'candidate pointer commit changed')
    pair(code[discard + 2], 'mov', f'{dist[2]},{dist[1]}')
    require(code[discard + 2].after == skip, 'candidate skip no longer follows commits')
    # Reject hidden direct incoming branches bypassing the recognized draw guards.
    for call in (code[draw], code[discard]):
        require(all(not i.mnemonic.startswith('j') or body.target(i) != call.offset
                    for i in code), 'unexpected direct entry into guarded random draw')
    return {'threshold': code[draw + 4].after - 1,
            'guard': code[draw - 1], 'draw': code[draw]}


def check_bricks(consumer):
    matches = list(LAYER.finditer(consumer.data))
    require(len(matches) == 4, 'unsupported four brick RNG instruction shapes')
    joins = [m.end() + struct.unpack_from('<i', consumer.data, m.end() - 4)[0] for m in matches]
    require(len(set(joins)) == 1, 'brick selections no longer share one enum join')
    join = joins[0]
    require(matches[-1].end() <= join < len(consumer.data) - 2,
            'unsupported brick common join extent')
    # Never decode the trailing in-section tables as instructions. This bounded
    # prefix includes the actual common join but stops before later cell work.
    code = list(decode(consumer.data[:join + 2]).values())
    by_offset = {i.offset: j for j, i in enumerate(code)}
    pair(code[-1], 'mov')
    result = re.fullmatch(r'(esi|edi|ebx|ebp),eax', code[-1].operands)
    require(result, 'unsupported common enum result transfer')
    carrier = result[1]
    body = Body(consumer.label, consumer.data[:join + 2], {}, consumer.references)
    selected = []
    for arm, (match, low) in enumerate(zip(matches, (0x133, 0x139, 0x13f, 0x145))):
        index = by_offset[match.start()]
        ins = code[index:index + 10]
        require(consumer.call_name(ins[0]) == RAND, 'brick RNG referent changed')
        for i, mn, op in zip(ins[1:], ('cdq', 'mov', 'idiv', 'xor', 'inc', 'cmp', 'setle', 'add', 'jmp'),
                              ('', 'ecx,0x64', 'ecx', 'eax,eax', 'edx', 'edx,0x32', 'al',
                               f'eax,0x{low:x}', None)):
            pair(i, mn, op)
        require(body.target(ins[-1]) == join, 'brick enum tail changed')
        if arm < 3:
            pair(code[index - 2], 'cmp')
            require(re.fullmatch(r'edx,DWORD PTR \[esp\+0x[0-9a-f]+\]', code[index - 2].operands),
                    'unsupported color threshold comparison')
            pair(code[index - 1], 'jg')
            next_index = by_offset[matches[arm + 1].start()]
            expected = code[next_index - 2].offset if arm < 2 else matches[3].start()
            require(body.target(code[index - 1]) == expected, 'wrong color-arm guard edge')
        selected.append((index, ins))
    first = selected[0][0]
    pair(code[first - 6], 'cmp')
    require(re.fullmatch(r'edx,DWORD PTR \[esp\+0x[0-9a-f]+\]', code[first - 6].operands),
            'unsupported brown threshold comparison')
    pair(code[first - 5], 'jg')
    require(body.target(code[first - 5]) == code[first - 2].offset, 'wrong brown rejection edge')
    pair(code[first - 4], 'mov', f'{carrier},0x130')
    pair(code[first - 3], 'jmp')
    # Brown skips both layer draw and enum-result copy. Its target is just
    # outside the bounded prefix; inspect the direct operand without traversing.
    require(code[first - 3].operands == hex(join + 2), 'brown unexpectedly enters layer draw')
    return {'threshold': selected[0][1][6].after - 1,
            'draw': selected[0][1][0], 'guard': code[first - 3]}


class RngHelperConsumerTests(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        from gruntz.core.paths import BUILD
        from gruntz.core.pe import Pe
        from gruntz.model import resolve

        pe = Pe()
        names = {r.rva: r.name for r in resolve().functions if r.name}
        require(names.get(RAND_RVA) == RAND, 'original CRT rand identity changed')
        cls.pairs = []
        for symbol, unit, rva, size, checker in (
                (GHOST, 'battlezmapconfig', 0x2ab80, 0x15e, check_ghost),
                (BRICK, 'brickzload', 0x810f0, 0xa80, check_bricks)):
            require(names.get(rva) == symbol, 'original consumer identity changed')
            data = pe.read(rva, size)
            require(data is not None, 'original consumer bytes unavailable')
            cls.pairs.append((production(BUILD / f'objdiff/base/{unit}.obj', symbol),
                              Consumer(f'original {symbol}', data, (), rva, pe), checker))

    def test_actual_production_and_original_protocols(self):
        for production_code, retail_code, checker in self.pairs:
            with self.subTest(owner=production_code.label):
                checker(retail_code)
                checker(production_code)

    def test_wrong_threshold_is_rejected(self):
        for production_code, retail_code, checker in self.pairs:
            for code in (production_code, retail_code):
                with self.subTest(owner=code.label):
                    site = checker(code)['threshold']
                    changed = code.changed(site, bytes([code.data[site] - 1]))
                    with self.assertRaises(AssertionError):
                        checker(changed)

    def test_wrong_original_rng_referent_is_rejected(self):
        for _, code, checker in self.pairs:
            with self.subTest(owner=code.label):
                call = checker(code)['draw']
                # Redirect actual original rel32 bytes to adjacent srand. Only
                # the referent changes; instruction count/size remains constant.
                displacement = 0x11fed0 - (code.rva + call.after)
                changed = code.changed(call.offset + 1, struct.pack('<i', displacement))
                with self.assertRaisesRegex(AssertionError, 'identity|referent'):
                    checker(changed)

    def test_wrong_production_rng_referent_is_rejected(self):
        for code, _, checker in self.pairs:
            with self.subTest(owner=code.label):
                call = checker(code)['draw']
                refs = tuple((site, kind, '_srand' if site == call.offset + 1 else name)
                             for site, kind, name in code.references)
                require(refs != code.references, 'negative control did not change a relocation')
                changed = Consumer(code.label + ' wrong COFF referent', code.data, refs)
                with self.assertRaisesRegex(AssertionError, 'identity|referent'):
                    checker(changed)

    def test_wrong_guard_edge_is_rejected(self):
        for production_code, retail_code, checker in self.pairs:
            for code in (production_code, retail_code):
                with self.subTest(owner=code.label):
                    sites = checker(code)
                    branch, draw = sites['guard'], sites['draw']
                    # Keep opcode/count/referents; make a formerly skipped draw
                    # reachable from nonghost/brown path by redirecting its edge.
                    width = 1 if branch.size == 2 else 4
                    require(branch.size in (2, 5, 6), 'unsupported branch encoding')
                    changed = code.changed(branch.after - width,
                                           (draw.offset - branch.after).to_bytes(width, 'little', signed=True))
                    with self.assertRaises(AssertionError):
                        checker(changed)


if __name__ == '__main__':
    unittest.main()
