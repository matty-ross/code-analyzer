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


def parse(file_name: str, mode: int) -> Generator[Instruction]:
    md = capstone.Cs(
        arch=capstone.CS_ARCH_X86,
        mode={32: capstone.CS_MODE_32, 64: capstone.CS_MODE_64}[mode],
    )
    
    with open(file_name, 'r', encoding='utf-8') as file:
        for line in file:
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
