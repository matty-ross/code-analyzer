from dataclasses import dataclass
from typing import Generator

import capstone


@dataclass
class Instruction:
    address: int = None
    disassembly: capstone.CsInsn = None
    registers: dict[str, int] = None


def parse(file_name: str) -> Generator[Instruction, None, None]:
    with open(file_name, 'r', encoding='utf-8') as file:
        md = capstone.Cs(arch=capstone.CS_ARCH_X86, mode=capstone.CS_MODE_32) # TODO: add support for x64   
        
        for line in file:
            address, code, registers = line.split(' | ')
            instruction = Instruction()
    
            instruction.address = int(address, 16)
    
            disassembly = md.disasm(bytes.fromhex(code), instruction.address, count=1)
            instruction.disassembly = next(disassembly)
    
            instruction.registers = {}
            for register in registers.split(' '):
                name, value = register.split('=')
                instruction.registers[name] = int(value, 16)

            yield instruction
