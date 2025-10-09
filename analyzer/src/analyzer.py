import pathlib
import collections
from dataclasses import dataclass
from typing import Generator

import capstone


@dataclass
class Instruction:
    address: int = None
    size: int = None
    mnemonic: str = None
    operands: str = None
    registers: dict[str, int] = None


class Analyzer:

    def __init__(self, trace_file_path: str, output_directory_path: str, mode: int):
        self.trace_file_path = pathlib.Path(trace_file_path)
        self.output_directory_path = pathlib.Path(output_directory_path)
        self.mode = mode
        self.trace_file = None
        self.mnemonics_file = None
        self.disassembly_file = None
        self.memory_access_file = None
        self.mnemonics = collections.Counter()


    def __enter__(self):
        self.trace_file = open(self.trace_file_path, 'r', encoding='utf-8')
        self.mnemonics_file = open(self.output_directory_path / 'mnemonics.txt', 'w', encoding='utf-8')
        self.disassembly_file = open(self.output_directory_path / 'disassembly.txt', 'w', encoding='utf-8')
        self.memory_access_file = open(self.output_directory_path / 'memory_access.txt', 'w', encoding='utf-8')
        return self


    def __exit__(self, exc_type, exc_value, traceback):
        self.trace_file.close()
        self.mnemonics_file.close()
        self.disassembly_file.close()
        self.memory_access_file.close()


    def analyze(self) -> None:
        for instruction in self._parse_trace_file():
            self.mnemonics[instruction.mnemonic] += 1

            address = f'0x{instruction.address :x}:'
            disassembly = f'{instruction.mnemonic} {instruction.operands}'
            registers = [f'{name}=0x{value :0{self.mode // 4}X}' for name, value in instruction.registers.items()]
            self.disassembly_file.write(f"{address :<{self.mode // 2}} {disassembly :<50} ; {' '.join(registers)}\n")

            memory_address = self._compute_memory_address(instruction)
            self.memory_access_file.write(f"{instruction.address :0{self.mode // 4}X} -> {memory_address :0{self.mode // 4}X}\n")

        for mnemonic, count in sorted(self.mnemonics.items()):
            self.mnemonics_file.write(f"{mnemonic :<10} {count}\n")


    def _parse_trace_file(self) -> Generator[Instruction]:
        md = capstone.Cs(
            arch=capstone.CS_ARCH_X86,
            mode={
                32: capstone.CS_MODE_32,
                64: capstone.CS_MODE_64,
            }[self.mode],
        )

        for line in self.trace_file:
            address, code, registers = line.split(' | ')
            instruction = Instruction()

            instruction.address = int(address, 16)

            disassembly = next(md.disasm_lite(bytes.fromhex(code), instruction.address, count=1))
            instruction.size = disassembly[1]
            instruction.mnemonic = disassembly[2]
            instruction.operands = disassembly[3]

            instruction.registers = {}
            for register in registers.split(' '):
                name, value = register.split('=')
                instruction.registers[name] = int(value, 16)

            yield instruction


    def _compute_memory_address(self, instruction: Instruction) -> None | int:
        start = instruction.operands.find('[')
        end = instruction.operands.find(']')
        if start == -1 or end == -1 or instruction.mnemonic == 'lea':
            return None
        
        loc = {}
        source = ''
        source += '; '.join(f'{name}={value}' for name, value in instruction.registers.items())
        source += f'; memory_address = {instruction.operands[start + 1:end]}'
        exec(source, locals=loc)
    
        return loc['memory_address'] & int('F' * (self.mode // 4), 16)
