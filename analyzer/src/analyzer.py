import pathlib
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


    def __enter__(self):
        self.trace_file = open(self.trace_file_path, 'r', encoding='utf-8')
        return self


    def __exit__(self, exc_type, exc_value, traceback):
        self.trace_file.close()


    def analyze(self) -> None:
        for instruction in self._parse_trace_file():
            print(instruction.mnemonic, instruction.operands) # TODO: process the instruction


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
