"""Isolated VC5 /Ob0 controls for the real map occupancy header.

Only the three integer leaf bodies are checked, using bounded instruction
evaluation over synthetic memory. This is not game execution, a general x86
emulator, or proof of production callers' inlining/row-reload schedules.
Unknown instructions, addresses and control flow fail closed.
"""

import os
from pathlib import Path
import re
import tempfile
import unittest


GET = '?OccupantAt@CGruntzMapMgr@@QBEHII@Z'
RELEASE = '?ReleaseCellOccupancy@CGruntzMapMgr@@QAEXHH@Z'
ACQUIRE = '?AcquireCellOccupancy@CGruntzMapMgr@@QAEXHHH@Z'
MASK = 0xffffffff
BIT = 0x20000000
THIS, ROWS, STACK = 0x10000, 0x20000, 0x100000
WIDTH, HEIGHT = 4, 3


def require(condition, message):
    if not condition:
        raise AssertionError(message)


def cell(x, y):
    return 0x30000 + y * 0x1000 + x * 0x1c


class LeafCheck:
    """Small, explicit instruction subset; never follows calls or external jumps."""

    def __init__(self, instructions, x, y, owner, flags):
        self.code = {off: (mn, operands) for off, mn, operands in instructions}
        offsets = list(self.code)
        self.next = dict(zip(offsets, offsets[1:]))
        self.reg = {name: 0 for name in ('eax', 'ebx', 'ecx', 'edx', 'esi', 'edi', 'ebp', 'esp')}
        self.reg.update(ecx=THIS, esp=STACK)
        self.memory, self.writes, self.reads = {}, [], []
        self.comparison = None
        self.seed(THIS + 8, ROWS)
        self.seed(THIS + 12, WIDTH)
        self.seed(THIS + 16, HEIGHT)
        for row in range(HEIGHT):
            self.seed(ROWS + row * 4, cell(0, row))
            for column in range(WIDTH):
                address = cell(column, row)
                for field in range(0, 0x1c, 4):
                    self.seed(address + field, 0x88776655)
                self.seed(address, flags)
                self.seed(address + 4, 0x100 + row * WIDTH + column)
        for offset, value in ((0, 0xdeadbeef), (4, x), (8, y), (12, owner)):
            self.seed(STACK + offset, value)

    def seed(self, address, value):
        for n in range(4):
            self.memory[address + n] = ((value & MASK) >> (n * 8)) & 0xff

    def address(self, operand):
        require('[' in operand or operand.startswith('ds:'),
                f'unsupported memory operand: {operand}')
        expression = operand.split('[', 1)[1].split(']', 1)[0] if '[' in operand else operand[3:]
        total = 0
        for term in re.findall(r'[+-]?[^+-]+', expression):
            sign = -1 if term.startswith('-') else 1
            factors = term.lstrip('+-').split('*')
            value = 1
            for factor in factors:
                value *= self.reg[factor] if factor in self.reg else int(factor, 0)
            total += sign * value
        return total & MASK

    @staticmethod
    def width(operand):
        require(operand.startswith(('BYTE PTR ', 'DWORD PTR ', 'ds:')),
                f'unsupported memory width: {operand}')
        return 1 if operand.startswith('BYTE PTR ') else 4

    def read(self, address, size):
        require(all(address + n in self.memory for n in range(size)),
                f'unknown memory read 0x{address:x}/{size}')
        self.reads.append((address, size))
        return sum(self.memory[address + n] << (n * 8) for n in range(size))

    def value(self, operand):
        if operand in self.reg:
            return self.reg[operand]
        if '[' in operand or operand.startswith('ds:'):
            return self.read(self.address(operand), self.width(operand))
        return int(operand, 0) & MASK

    def store(self, operand, value):
        value &= MASK
        if operand in self.reg:
            self.reg[operand] = value
            return
        address, size = self.address(operand), self.width(operand)
        require(all(address + n in self.memory for n in range(size)),
                f'unknown memory write 0x{address:x}/{size}')
        for n in range(size):
            self.memory[address + n] = (value >> (n * 8)) & 0xff
        self.writes.append((address, size))

    def run(self, stack_pop):
        offset = min(self.code)
        for _ in range(80):
            require(offset in self.code, f'unsupported jump/exit +0x{offset:x}')
            mnemonic, operands = self.code[offset]
            args = operands.split(',')
            next_offset = self.next.get(offset)
            if mnemonic == 'mov':
                self.store(args[0], self.value(args[1]))
            elif mnemonic == 'lea':
                self.store(args[0], self.address(args[1]))
            elif mnemonic in ('add', 'sub', 'and', 'or', 'xor', 'shl'):
                left, right = self.value(args[0]), self.value(args[1])
                value = {'add': lambda: left + right, 'sub': lambda: left - right,
                         'and': lambda: left & right, 'or': lambda: left | right,
                         'xor': lambda: left ^ right, 'shl': lambda: left << right}[mnemonic]()
                self.store(args[0], value)
                self.comparison = None
            elif mnemonic == 'cmp':
                self.comparison = (self.value(args[0]), self.value(args[1]))
            elif mnemonic in ('jae', 'jb', 'ja', 'jbe', 'je', 'jne', 'jge'):
                require(self.comparison is not None, 'branch without supported comparison')
                left, right = self.comparison
                signed = lambda value: value if value < 0x80000000 else value - 0x100000000
                taken = {'jae': left >= right, 'jb': left < right,
                         'ja': left > right, 'jbe': left <= right,
                         'je': left == right, 'jne': left != right,
                         'jge': signed(left) >= signed(right)}[mnemonic]
                if taken:
                    next_offset = int(operands, 0)
            elif mnemonic == 'jmp':
                next_offset = int(operands, 0)
            elif mnemonic == 'push':
                value = self.value(operands)
                self.reg['esp'] -= 4
                self.seed(self.reg['esp'], value)
            elif mnemonic == 'pop':
                self.store(operands, self.read(self.reg['esp'], 4))
                self.reg['esp'] += 4
            elif mnemonic == 'ret':
                require(int(operands or '0', 0) == stack_pop, 'wrong thiscall argument cleanup')
                require(self.reg['esp'] == STACK, 'unbalanced leaf stack')
                return self.reg['eax']
            elif mnemonic != 'nop':
                raise AssertionError(f'unsupported leaf instruction: {mnemonic} {operands}')
            require(next_offset is not None, 'leaf falls out without RET')
            offset = next_offset
        raise AssertionError('leaf exceeds bounded instruction budget')


def check_mutator(instructions, acquire):
    for x, y in ((0, 0), (2, 1), (WIDTH - 1, HEIGHT - 1)):
        for flags in (0, MASK, 0xd7ad55aa, 0xf7ad55aa):
            owner = 0xa5120345
            check = LeafCheck(instructions, x, y, owner, flags)
            before = dict(check.memory)
            check.run(12 if acquire else 8)
            address = cell(x, y)
            require(len(check.writes) == 2, f'expected two cell writes, got {check.writes}')
            first, second = check.writes
            require(first in ((address, 4), (address + 3, 1)) and second == (address + 4, 4),
                    f'wrong flag/owner write order or layout: {check.writes}')
            expected_flags = (flags | BIT) if acquire else (flags & ~BIT)
            require(check.read(address, 4) == expected_flags, 'wrong occupied-bit mask')
            require(check.read(address + 4, 4) == (owner if acquire else MASK),
                    'wrong packed owner or release sentinel')
            require(all(check.memory[key] == value for key, value in before.items()
                        if not address <= key < address + 8), 'unrelated grid field changed')


def check_getter(instructions):
    for x, y in ((0, 0), (3, 2), (4, 0), (0, 3), (MASK, 0), (0, MASK), (0x80000000, 0)):
        check = LeafCheck(instructions, x, y, 0, 0)
        result = check.run(8)
        valid = x < WIDTH and y < HEIGHT
        require(result == (0x100 + y * WIDTH + x if valid else MASK),
                f'wrong bounded owner result for {x:x},{y:x}: {result:x}')
        require(not check.writes, 'const getter mutated memory')
        if not valid:
            require(not any(ROWS <= address < ROWS + HEIGHT * 4
                            or cell(0, 0) <= address < cell(0, HEIGHT)
                            for address, _ in check.reads),
                    'out-of-bounds getter accessed row/cell memory')


@unittest.skipUnless(os.environ.get('MSVC_DIR'), 'requires pinned nix develop')
class MapOccupancyHeaderTests(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        from gruntz.delink.coffx import Obj
        from gruntz.tool import cl
        from gruntz.walls.pairscan import functions, insns

        with tempfile.TemporaryDirectory(prefix='gruntz-map-occupancy-') as directory:
            root = Path(directory)
            source = root / 'probe.cpp'
            source.write_text('''#include <Gruntz/MapCellInline.h>
#include <stddef.h>
typedef char CellLayout[(sizeof(BrickzCell) == 0x1c
    && offsetof(BrickzCell, m_flags) == 0 && offsetof(BrickzCell, m_occupantId) == 4) ? 1 : -1];
typedef char MapLayout[(offsetof(CMapMgr, m_rows) == 8
    && offsetof(CMapMgr, m_width) == 12 && offsetof(CMapMgr, m_height) == 16) ? 1 : -1];
i32 read_owner(const CGruntzMapMgr* grid, u32 x, u32 y) { return grid->OccupantAt(x, y); }
void release_owner(CGruntzMapMgr* grid, i32 x, i32 y) { grid->ReleaseCellOccupancy(x, y); }
void acquire_owner(CGruntzMapMgr* grid, i32 x, i32 y, i32 owner) {
    grid->AcquireCellOccupancy(x, y, owner);
}
''')
            path = root / 'probe.obj'
            cl.compile(source, path, ['/nologo', '/c', '/O2', '/Ob0', '/MT'])
            obj = Obj(path)
            bodies = functions(obj)
            require(all(symbol in bodies for symbol in (GET, RELEASE, ACQUIRE)),
                    'missing or changed const/u32 getter or signed-i32 mutator signatures')
            cls.code = {symbol: insns(obj, *bodies[symbol]) for symbol in (GET, RELEASE, ACQUIRE)}

    def test_real_header_leaf_contracts(self):
        check_getter(self.code[GET])
        check_mutator(self.code[RELEASE], False)
        check_mutator(self.code[ACQUIRE], True)

    def test_wrong_masks_are_rejected(self):
        for symbol, acquire in ((RELEASE, False), (ACQUIRE, True)):
            operation = 'or' if acquire else 'and'
            changed = False
            damaged = []
            for off, mnemonic, operands in self.code[symbol]:
                if (not changed and mnemonic == operation
                        and operands.rsplit(',', 1)[-1] in
                        (('0x20', '0x20000000') if acquire else ('0xdf', '0xdfffffff'))):
                    destination, value = operands.rsplit(',', 1)
                    operands = destination + ',' + hex(int(value, 0) ^ 0x10)
                    changed = True
                damaged.append((off, mnemonic, operands))
            self.assertTrue(changed, 'unsupported negative-control mask spelling')
            with self.assertRaisesRegex(AssertionError, 'occupied-bit mask'):
                check_mutator(damaged, acquire)

    def test_wrong_owner_store_is_rejected(self):
        damaged = []
        stores = [off for off, mnemonic, operands in self.code[ACQUIRE]
                  if mnemonic == 'mov' and operands.startswith('DWORD PTR [')]
        self.assertTrue(stores, 'unsupported negative-control owner spelling')
        for off, mnemonic, operands in self.code[ACQUIRE]:
            if off == stores[-1]:
                operands = operands.split(',')[0] + ',0x0'
            damaged.append((off, mnemonic, operands))
        with self.assertRaisesRegex(AssertionError, 'packed owner'):
            check_mutator(damaged, True)

    def test_signed_bounds_are_rejected(self):
        self.assertTrue(any(mn == 'jae' for _, mn, _ in self.code[GET]),
                        'unsupported negative-control unsigned-guard spelling')
        damaged = [(off, 'jge' if mn == 'jae' else mn, operands)
                   for off, mn, operands in self.code[GET]]
        with self.assertRaises(AssertionError):
            check_getter(damaged)

    def test_zero_owner_sentinel_is_rejected(self):
        damaged = []
        changed = False
        for off, mnemonic, operands in self.code[GET]:
            if operands == 'eax,0xffffffff' and mnemonic in ('mov', 'or'):
                mnemonic, operands = 'xor', 'eax,eax'
                changed = True
            damaged.append((off, mnemonic, operands))
        self.assertTrue(changed, 'unsupported negative-control sentinel spelling')
        with self.assertRaisesRegex(AssertionError, 'bounded owner result'):
            check_getter(damaged)


if __name__ == '__main__':
    unittest.main()
